// SPDX-License-Identifier: AGPL-3.0-or-later
#include "NeutralStft.h"

#include <pffft/pffft.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>

namespace spectral_decay
{
namespace
{
constexpr std::size_t fftLength = NeutralStft::fftSize;
constexpr std::size_t hopLength = NeutralStft::hopSize;
constexpr std::size_t requiredAlignment = 64;
constexpr double twoPi = 6.283185307179586476925286766559;
constexpr float wolaScale = 1.0f / (2.0f * static_cast<float>(fftLength));
constexpr std::size_t spectrumBinCount = fftLength / 2 + 1;
constexpr std::uint32_t amountBits = 0x00ffffffu;
constexpr std::uint32_t noiseModeBit = 0x80000000u;
constexpr float activityThreshold = 0.001f; // -60 dBFS peak threshold
constexpr float tailOutputCeiling = 0.89125094f; // -1 dBFS
constexpr double controlSmoothingSeconds = 0.020;
constexpr double silenceHoldSeconds = 0.010;
constexpr double tailTimeConstantSeconds = 0.250;
constexpr double bandLowHz = 500.0;
constexpr double bandHighHz = 1500.0;
constexpr double bandEdgeWidthHz = 100.0;
constexpr double pi = twoPi * 0.5;

static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
              "Tail settings require a lock-free 32-bit atomic");

class AlignedFloats final
{
public:
    AlignedFloats() = default;
    ~AlignedFloats() { pffft_aligned_free(allocation); }

    AlignedFloats(const AlignedFloats&) = delete;
    AlignedFloats& operator=(const AlignedFloats&) = delete;

    AlignedFloats(AlignedFloats&& other) noexcept
        : allocation(other.allocation), data(other.data)
    {
        other.allocation = nullptr;
        other.data = nullptr;
    }

    AlignedFloats& operator=(AlignedFloats&& other) noexcept
    {
        if (this != &other)
        {
            pffft_aligned_free(allocation);
            allocation = other.allocation;
            data = other.data;
            other.allocation = nullptr;
            other.data = nullptr;
        }
        return *this;
    }

    void allocate(std::size_t count)
    {
        constexpr std::size_t alignmentPadding = requiredAlignment - 1;
        void* const newAllocation = pffft_aligned_malloc(count * sizeof(float) + alignmentPadding);
        if (newAllocation == nullptr)
            throw std::bad_alloc();

        const auto address = reinterpret_cast<std::uintptr_t>(newAllocation);
        const auto alignedAddress = (address + alignmentPadding) & ~static_cast<std::uintptr_t>(alignmentPadding);

        pffft_aligned_free(allocation);
        allocation = newAllocation;
        data = reinterpret_cast<float*>(alignedAddress);
    }

    float* get() noexcept { return data; }
    const float* get() const noexcept { return data; }

private:
    void* allocation = nullptr;
    float* data = nullptr;
};

class Setup final
{
public:
    Setup() = default;
    ~Setup()
    {
        if (value != nullptr)
            pffft_destroy_setup(value);
    }

    Setup(const Setup&) = delete;
    Setup& operator=(const Setup&) = delete;

    void create()
    {
        value = pffft_new_setup(static_cast<int>(fftLength), PFFFT_REAL);
        if (value == nullptr)
            throw std::bad_alloc();
    }

    const PFFFT_Setup* get() const noexcept { return value; }

private:
    PFFFT_Setup* value = nullptr;
};
} // namespace

struct NeutralStft::Impl final
{
    using ChannelBins = std::array<std::array<float, spectrumBinCount>, 2>;

    explicit Impl(std::size_t channelCount, double rate)
        : channels(channelCount), sampleRate(rate)
    {
        setup.create();
        inputRing.allocate(channels * fftLength);
        outputRing.allocate(channels * fftLength);
        tailOutputRing.allocate(channels * fftLength);
        frame.allocate(fftLength);
        spectrum.allocate(fftLength);
        work.allocate(fftLength);
        tailFrame.allocate(fftLength);

        double windowSum = 0.0;
        double windowPowerSum = 0.0;
        for (std::size_t i = 0; i < fftLength; ++i)
        {
            const double hann = 0.5 - 0.5 * std::cos(twoPi * static_cast<double>(i) /
                                                    static_cast<double>(fftLength));
            window[i] = static_cast<float>(std::sqrt(hann < 0.0 ? 0.0 : hann));
            windowSum += window[i];
            windowPowerSum += static_cast<double>(window[i]) * window[i];
        }

        tonalSpectrumScale = static_cast<float>(static_cast<double>(fftLength) / windowSum);
        noiseSpectrumScale = static_cast<float>(
            std::sqrt(static_cast<double>(fftLength) / windowPowerSum));

        for (std::size_t bin = 1; bin < fftLength / 2; ++bin)
        {
            const double frequency = static_cast<double>(bin) * sampleRate /
                                     static_cast<double>(fftLength);
            double weight = 0.0;
            if (frequency >= bandLowHz && frequency <= bandHighHz)
            {
                if (frequency < bandLowHz + bandEdgeWidthHz)
                {
                    const double position = (frequency - bandLowHz) / bandEdgeWidthHz;
                    weight = 0.5 - 0.5 * std::cos(pi * position);
                }
                else if (frequency > bandHighHz - bandEdgeWidthHz)
                {
                    const double position = (bandHighHz - frequency) / bandEdgeWidthHz;
                    weight = 0.5 - 0.5 * std::cos(pi * position);
                }
                else
                {
                    weight = 1.0;
                }
            }

            bandMask[bin] = static_cast<float>(weight);
            if (weight > 0.0)
            {
                firstBandBin = std::min(firstBandBin, bin);
                lastBandBin = bin;
            }
        }

        clearState();
    }

    void clearState() noexcept
    {
        for (std::size_t i = 0; i < channels * fftLength; ++i)
        {
            inputRing.get()[i] = 0.0f;
            outputRing.get()[i] = 0.0f;
            tailOutputRing.get()[i] = 0.0f;
        }
        current = 0;
        samplesSinceFrame = 0;
        totalSamples = 0;
        lastExcitationSample = 0;
        capturedExcitationSample = noSample;
        hasExcitation = false;
        tailActive = false;
        tailStartSample = 0.0;
        tailDeadlineSample = 0.0;
        toneOmega = 0.0;
        smoothedAmount = 0.0f;
        smoothedNoiseMix = 0.0f;
        currentMagnitude = {};
        previousMagnitude = {};
        currentReal = {};
        currentImaginary = {};
        currentPhase = {};
        previousPhase = {};
        heldMagnitude = {};
        toneRelativePhase = {};
        noiseRelativePhase = {};
        tonePhaseAtTailStart = {};
        generatedTailReal = {};
        generatedTailImaginary = {};
        stereoPolarity = 0;
        for (std::size_t bin = 0; bin < spectrumBinCount; ++bin)
        {
            noiseRandomState[bin] =
                0x9e3779b9u ^ (static_cast<std::uint32_t>(bin) * 0x85ebca6bu);
            if (noiseRandomState[bin] == 0)
                noiseRandomState[bin] = 1;
        }
        noiseFramePhase = {};
    }

    void processFrame() noexcept
    {
        const bool spectrumTracking =
            targetAmount > 0.0f || smoothedAmount > 0.0f || tailActive;

        for (std::size_t channel = 0; channel < channels; ++channel)
        {
            float* const channelInput = inputRing.get() + channel * fftLength;
            float* const channelOutput = outputRing.get() + channel * fftLength;

            // current is both the oldest input slot and the next output slot.
            // Initially, the unfilled part of this frame is zero-padded
            // negative-time input, so the first real samples get all four
            // overlapping windows.
            for (std::size_t i = 0; i < fftLength; ++i)
            {
                const std::size_t ringIndex = current + i < fftLength
                                                  ? current + i
                                                  : current + i - fftLength;
                frame.get()[i] = channelInput[ringIndex] * window[i];
            }

            pffft_transform_ordered(setup.get(), frame.get(), spectrum.get(), work.get(),
                                    PFFFT_FORWARD);

            if (spectrumTracking)
            {
                for (std::size_t bin = firstBandBin;
                     bin <= lastBandBin && bin < spectrumBinCount; ++bin)
                {
                    if (bandMask[bin] <= 0.0f)
                        continue;

                    const float real = spectrum.get()[2 * bin];
                    const float imaginary = spectrum.get()[2 * bin + 1];
                    currentReal[channel][bin] = real;
                    currentImaginary[channel][bin] = imaginary;
                    currentMagnitude[channel][bin] =
                        std::sqrt(real * real + imaginary * imaginary);
                    currentPhase[channel][bin] = std::atan2(imaginary, real);
                }
            }

            pffft_transform_ordered(setup.get(), spectrum.get(), frame.get(), work.get(),
                                    PFFFT_BACKWARD);

            // PFFFT's inverse contributes fftLength; the four periodic-Hann
            // products sum to 2. Thus each frame uses 1 / (fftLength * 2).
            for (std::size_t i = 0; i < fftLength; ++i)
            {
                const std::size_t ringIndex = current + i < fftLength
                                                  ? current + i
                                                  : current + i - fftLength;
                channelOutput[ringIndex] += frame.get()[i] * window[i] * wolaScale;
            }
        }

        const std::uint64_t latestSample = totalSamples - 1;
        const double frameStart = static_cast<double>(latestSample) -
                                  static_cast<double>(fftLength - 1);
        if (spectrumTracking)
            updateTailState(latestSample, frameStart);

        if (tailActive && smoothedAmount > 0.0f)
            renderTailFrame(frameStart);

        if (spectrumTracking)
        {
            for (std::size_t channel = 0; channel < channels; ++channel)
            {
                for (std::size_t bin = firstBandBin;
                     bin <= lastBandBin && bin < spectrumBinCount; ++bin)
                {
                    previousMagnitude[channel][bin] = currentMagnitude[channel][bin];
                    previousPhase[channel][bin] = currentPhase[channel][bin];
                }
            }
        }
    }

    void setTargets(std::uint32_t packedSettings) noexcept
    {
        targetAmount = static_cast<float>(packedSettings & amountBits) /
                       static_cast<float>(amountBits);
        targetNoiseMix = (packedSettings & noiseModeBit) != 0 ? 1.0f : 0.0f;
    }

    void advanceControls() noexcept
    {
        const float amountStep = static_cast<float>(
            1.0 / std::max(1.0, sampleRate * controlSmoothingSeconds));
        if (smoothedAmount < targetAmount)
            smoothedAmount = std::min(targetAmount, smoothedAmount + amountStep);
        else if (smoothedAmount > targetAmount)
            smoothedAmount = std::max(targetAmount, smoothedAmount - amountStep);

        const float modeStep = amountStep;
        if (smoothedNoiseMix < targetNoiseMix)
            smoothedNoiseMix = std::min(targetNoiseMix, smoothedNoiseMix + modeStep);
        else if (smoothedNoiseMix > targetNoiseMix)
            smoothedNoiseMix = std::max(targetNoiseMix, smoothedNoiseMix - modeStep);
    }

    double estimateTonalOmegaFromInput(std::size_t channel, double frameStart) const noexcept
    {
        std::size_t firstActive = fftLength;
        std::size_t lastActive = 0;
        const float* const channelInput = inputRing.get() + channel * fftLength;
        for (std::size_t i = 0; i < fftLength; ++i)
        {
            const double sampleTime = frameStart + static_cast<double>(i);
            if (sampleTime < 0.0 ||
                sampleTime > static_cast<double>(lastExcitationSample))
                continue;

            const std::size_t ringIndex = current + i < fftLength
                                              ? current + i
                                              : current + i - fftLength;
            if (std::fabs(channelInput[ringIndex]) > activityThreshold)
            {
                firstActive = std::min(firstActive, i);
                lastActive = i;
            }
        }

        if (firstActive >= lastActive)
            return 0.0;

        double firstPositiveCrossing = 0.0;
        double lastPositiveCrossing = 0.0;
        std::size_t crossingCount = 0;
        for (std::size_t i = firstActive + 1; i <= lastActive; ++i)
        {
            const std::size_t previousIndex = current + i - 1 < fftLength
                                                  ? current + i - 1
                                                  : current + i - 1 - fftLength;
            const std::size_t currentIndex = current + i < fftLength
                                                ? current + i
                                                : current + i - fftLength;
            const float previous = channelInput[previousIndex];
            const float next = channelInput[currentIndex];
            if (previous <= 0.0f && next > 0.0f)
            {
                const double crossing = static_cast<double>(i - 1) -
                                         static_cast<double>(previous) /
                                             static_cast<double>(next - previous);
                if (crossingCount == 0)
                    firstPositiveCrossing = crossing;
                lastPositiveCrossing = crossing;
                ++crossingCount;
            }
        }

        if (crossingCount < 3 || lastPositiveCrossing <= firstPositiveCrossing)
            return 0.0;

        const double frequency =
            static_cast<double>(crossingCount - 1) * sampleRate /
            (lastPositiveCrossing - firstPositiveCrossing);
        if (!std::isfinite(frequency) || frequency < bandLowHz || frequency > bandHighHz)
            return 0.0;

        return twoPi * frequency / sampleRate;
    }

    void updateTailState(std::uint64_t latestSample, double frameStart) noexcept
    {
        if (!hasExcitation || firstBandBin >= spectrumBinCount)
            return;

        const bool quietAfterExcitation = lastExcitationSample < latestSample;
        if (tailActive &&
            static_cast<double>(lastExcitationSample) >= tailStartSample)
            tailActive = false;

        if (tailActive && static_cast<double>(latestSample) > tailDeadlineSample)
            tailActive = false;

        const double quietDuration = quietAfterExcitation
                                         ? static_cast<double>(latestSample -
                                                               lastExcitationSample)
                                         : 0.0;
        if (!quietAfterExcitation ||
            quietDuration < sampleRate * silenceHoldSeconds ||
            lastExcitationSample == capturedExcitationSample)
            return;

        std::size_t peak = firstBandBin;
        double peakMagnitude = 0.0;
        for (std::size_t bin = firstBandBin; bin <= lastBandBin; ++bin)
        {
            double linkedMagnitude = 0.0;
            for (std::size_t channel = 0; channel < channels; ++channel)
                linkedMagnitude += static_cast<double>(currentMagnitude[channel][bin]) *
                                   bandMask[bin];

            if (linkedMagnitude > peakMagnitude)
            {
                peakMagnitude = linkedMagnitude;
                peak = bin;
            }
        }

        if (peakMagnitude <= 1.0e-12)
            return;

        std::size_t phaseReferenceChannel = 0;
        for (std::size_t channel = 1; channel < channels; ++channel)
        {
            if (currentMagnitude[channel][peak] >
                currentMagnitude[phaseReferenceChannel][peak])
                phaseReferenceChannel = channel;
        }

        const double expectedAdvance = twoPi * static_cast<double>(peak) *
                                       static_cast<double>(hopLength) /
                                       static_cast<double>(fftLength);
        toneOmega = twoPi * static_cast<double>(peak) / static_cast<double>(fftLength);
        if (previousMagnitude[phaseReferenceChannel][peak] > 1.0e-9f &&
            currentMagnitude[phaseReferenceChannel][peak] > 1.0e-9f)
        {
            const double phaseDifference =
                static_cast<double>(currentPhase[phaseReferenceChannel][peak]) -
                static_cast<double>(previousPhase[phaseReferenceChannel][peak]) -
                expectedAdvance;
            const double frequencyCorrection =
                std::remainder(phaseDifference, twoPi) / static_cast<double>(hopLength);
            // A stable component cannot be more than half a bin from its
            // selected peak. Reject transient phase jumps outside that range.
            if (std::abs(frequencyCorrection) <= pi / static_cast<double>(fftLength))
                toneOmega += frequencyCorrection;
        }

        if (targetNoiseMix < 0.5f)
        {
            // A short burst may not provide two stationary STFT frames; use
            // its recent input crossings to refine the held peak frequency.
            const double inputOmega =
                estimateTonalOmegaFromInput(phaseReferenceChannel, frameStart);
            if (inputOmega > 0.0)
                toneOmega = inputOmega;
        }

        const double lowestOmega = twoPi * bandLowHz / sampleRate;
        const double highestOmega =
            twoPi * std::min(bandHighHz, sampleRate * 0.5) / sampleRate;
        toneOmega = std::max(lowestOmega, std::min(highestOmega, toneOmega));
        tonePeakBin = peak;
        tailStartSample = static_cast<double>(lastExcitationSample) + 1.0;
        tailDeadlineSample = static_cast<double>(lastExcitationSample) +
                             2.0 * sampleRate;

        for (std::size_t channel = 0; channel < channels; ++channel)
        {
            tonePhaseAtTailStart[channel] =
                static_cast<double>(currentPhase[channel][peak]) +
                toneOmega * (tailStartSample - frameStart);
        }

        stereoPolarity = 0;
        if (channels == 2)
        {
            bool identical = true;
            bool opposite = true;
            for (std::size_t bin = firstBandBin; bin <= lastBandBin; ++bin)
            {
                if (bandMask[bin] <= 0.0f)
                    continue;
                identical = identical &&
                            currentReal[1][bin] == currentReal[0][bin] &&
                            currentImaginary[1][bin] == currentImaginary[0][bin];
                opposite = opposite &&
                           currentReal[1][bin] == -currentReal[0][bin] &&
                           currentImaginary[1][bin] == -currentImaginary[0][bin];
            }
            if (identical)
                stereoPolarity = 1;
            else if (opposite)
                stereoPolarity = -1;
        }

        for (std::size_t bin = firstBandBin; bin <= lastBandBin; ++bin)
        {
            std::size_t binReferenceChannel = 0;
            for (std::size_t channel = 1; channel < channels; ++channel)
            {
                if (currentMagnitude[channel][bin] >
                    currentMagnitude[binReferenceChannel][bin])
                    binReferenceChannel = channel;
            }

            for (std::size_t channel = 0; channel < channels; ++channel)
            {
                heldMagnitude[channel][bin] = currentMagnitude[channel][bin] * bandMask[bin];
                toneRelativePhase[channel][bin] =
                    currentPhase[channel][bin] - currentPhase[channel][peak];
                noiseRelativePhase[channel][bin] =
                    currentPhase[channel][bin] -
                    currentPhase[binReferenceChannel][bin];
            }
        }

        capturedExcitationSample = lastExcitationSample;
        tailActive = true;
    }

    void renderTailFrame(double frameStart) noexcept
    {
        for (std::size_t bin = firstBandBin; bin <= lastBandBin; ++bin)
        {
            std::uint32_t& randomState = noiseRandomState[bin];
            randomState ^= randomState << 13;
            randomState ^= randomState >> 17;
            randomState ^= randomState << 5;
            noiseFramePhase[bin] =
                static_cast<float>((static_cast<double>(randomState) / 4294967296.0) *
                                       twoPi -
                                   pi);
        }

        const double phaseAtFrameStart = tonePhaseAtTailStart[0] +
                                         toneOmega * (frameStart - tailStartSample);

        for (std::size_t channel = 0; channel < channels; ++channel)
        {
            for (std::size_t i = 0; i < fftLength; ++i)
                spectrum.get()[i] = 0.0f;

            if (channel == 1 && stereoPolarity != 0)
            {
                for (std::size_t bin = firstBandBin; bin <= lastBandBin; ++bin)
                {
                    spectrum.get()[2 * bin] =
                        static_cast<float>(stereoPolarity) * generatedTailReal[bin];
                    spectrum.get()[2 * bin + 1] =
                        static_cast<float>(stereoPolarity) * generatedTailImaginary[bin];
                }
            }
            else
            {
                const double channelPhaseAtFrameStart =
                    phaseAtFrameStart + (tonePhaseAtTailStart[channel] -
                                         tonePhaseAtTailStart[0]);
                for (std::size_t bin = firstBandBin; bin <= lastBandBin; ++bin)
                {
                    const double tonalPhase = channelPhaseAtFrameStart +
                                              toneRelativePhase[channel][bin];
                    const double noisePhase = static_cast<double>(noiseFramePhase[bin]) +
                                              noiseRelativePhase[channel][bin];
                    double tonalMagnitude =
                        static_cast<double>(heldMagnitude[channel][bin]) * tonalSpectrumScale;
                    // Keep the peak and immediate neighbors phase-locked;
                    // distant bins belong to other partials/noise.
                    const std::size_t peakDistance =
                        bin > tonePeakBin ? bin - tonePeakBin : tonePeakBin - bin;
                    if (peakDistance > 1)
                        tonalMagnitude = 0.0;
                    const double noiseMagnitude =
                        static_cast<double>(heldMagnitude[channel][bin]) * noiseSpectrumScale;
                    const double real =
                        tonalMagnitude * std::cos(tonalPhase) * (1.0 - smoothedNoiseMix) +
                        noiseMagnitude * std::cos(noisePhase) * smoothedNoiseMix;
                    const double imaginary =
                        tonalMagnitude * std::sin(tonalPhase) * (1.0 - smoothedNoiseMix) +
                        noiseMagnitude * std::sin(noisePhase) * smoothedNoiseMix;

                    spectrum.get()[2 * bin] = static_cast<float>(real * smoothedAmount);
                    spectrum.get()[2 * bin + 1] = static_cast<float>(imaginary * smoothedAmount);
                    if (channel == 0 && channels == 2 && stereoPolarity != 0)
                    {
                        generatedTailReal[bin] = spectrum.get()[2 * bin];
                        generatedTailImaginary[bin] = spectrum.get()[2 * bin + 1];
                    }
                }
            }

            pffft_transform_ordered(setup.get(), spectrum.get(), tailFrame.get(), work.get(),
                                    PFFFT_BACKWARD);

            float* const channelTailOutput = tailOutputRing.get() + channel * fftLength;
            for (std::size_t i = 0; i < fftLength; ++i)
            {
                const double sampleTime = frameStart + static_cast<double>(i);
                if (sampleTime < tailStartSample || sampleTime > tailDeadlineSample)
                    continue;

                // Gate after the IFFT and before WOLA: no frame/window support
                // can add tail energy beyond this latency-aligned sample time.
                const double tailAge = sampleTime - tailStartSample;
                const double deadlineDistance = tailDeadlineSample - sampleTime;
                const double fadeInPosition = std::min(
                    1.0, tailAge / static_cast<double>(hopLength));
                const double fadeIn = 0.5 - 0.5 * std::cos(pi * fadeInPosition);
                double fadeOut = 1.0;
                if (deadlineDistance < static_cast<double>(hopLength))
                {
                    const double fadeOutPosition = std::max(
                        0.0, deadlineDistance / static_cast<double>(hopLength));
                    fadeOut = 0.5 - 0.5 * std::cos(pi * fadeOutPosition);
                }
                const double decay = std::exp(
                    -tailAge / (sampleRate * tailTimeConstantSeconds));
                const float envelope = static_cast<float>(decay * fadeIn * fadeOut);

                const std::size_t ringIndex = current + i < fftLength
                                                  ? current + i
                                                  : current + i - fftLength;
                channelTailOutput[ringIndex] +=
                    tailFrame.get()[i] * envelope * window[i] * wolaScale;
            }
        }
    }

    const std::size_t channels;
    const double sampleRate;
    Setup setup;
    AlignedFloats inputRing;
    AlignedFloats outputRing;
    AlignedFloats tailOutputRing;
    AlignedFloats frame;
    AlignedFloats spectrum;
    AlignedFloats work;
    AlignedFloats tailFrame;
    alignas(requiredAlignment) std::array<float, fftLength> window{};
    std::array<float, spectrumBinCount> bandMask {};
    ChannelBins currentMagnitude {};
    ChannelBins previousMagnitude {};
    ChannelBins currentReal {};
    ChannelBins currentImaginary {};
    ChannelBins currentPhase {};
    ChannelBins previousPhase {};
    ChannelBins heldMagnitude {};
    ChannelBins toneRelativePhase {};
    ChannelBins noiseRelativePhase {};
    std::array<double, 2> tonePhaseAtTailStart {};
    std::array<float, spectrumBinCount> generatedTailReal {};
    std::array<float, spectrumBinCount> generatedTailImaginary {};
    std::array<std::uint32_t, spectrumBinCount> noiseRandomState {};
    std::array<float, spectrumBinCount> noiseFramePhase {};
    std::size_t firstBandBin = spectrumBinCount;
    std::size_t lastBandBin = 0;
    std::size_t tonePeakBin = 0;
    double tonalSpectrumScale = 1.0;
    double noiseSpectrumScale = 1.0;
    double toneOmega = 0.0;
    float targetAmount = 0.0f;
    float targetNoiseMix = 0.0f;
    float smoothedAmount = 0.0f;
    float smoothedNoiseMix = 0.0f;
    std::size_t current = 0;
    std::size_t samplesSinceFrame = 0;
    std::uint64_t totalSamples = 0;
    std::uint64_t lastExcitationSample = 0;
    std::uint64_t capturedExcitationSample = noSample;
    double tailStartSample = 0.0;
    double tailDeadlineSample = 0.0;
    int stereoPolarity = 0;
    bool hasExcitation = false;
    bool tailActive = false;

    static constexpr std::uint64_t noSample =
        std::numeric_limits<std::uint64_t>::max();
};

NeutralStft::NeutralStft() = default;
NeutralStft::~NeutralStft() = default;

void NeutralStft::setTail(float amount, TailMode mode) noexcept
{
    if (!std::isfinite(amount))
        amount = 0.0f;
    amount = std::max(0.0f, std::min(1.0f, amount));
    const auto quantizedAmount = static_cast<std::uint32_t>(
        static_cast<double>(amount) * static_cast<double>(amountBits) + 0.5);
    const std::uint32_t packedSettings =
        quantizedAmount | (mode == TailMode::noise ? noiseModeBit : 0u);
    requestedTailSettings.store(packedSettings, std::memory_order_release);
}

void NeutralStft::prepare(double sampleRate, std::size_t channels)
{
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0)
        throw std::invalid_argument("NeutralStft requires a finite, positive sample rate");
    if (channels != 1 && channels != 2)
        throw std::invalid_argument("NeutralStft supports one or two channels");

    auto prepared = std::make_unique<Impl>(channels, sampleRate);
    impl = std::move(prepared);
}

void NeutralStft::reset() noexcept
{
    if (impl != nullptr)
        impl->clearState();
}

void NeutralStft::process(float* const* channelData, std::size_t numSamples) noexcept
{
    if (numSamples == 0)
        return;

    assert(impl != nullptr);
    assert(channelData != nullptr);

    for (std::size_t channel = 0; channel < impl->channels; ++channel)
        assert(channelData[channel] != nullptr);

    impl->setTargets(requestedTailSettings.load(std::memory_order_acquire));

    for (std::size_t sample = 0; sample < numSamples; ++sample)
    {
        impl->advanceControls();
        float inputs[2] = {};
        bool activeInput = false;
        for (std::size_t channel = 0; channel < impl->channels; ++channel)
        {
            inputs[channel] = channelData[channel][sample];
            activeInput = activeInput ||
                          std::fabs(inputs[channel]) > activityThreshold;
        }

        if (activeInput)
        {
            impl->hasExcitation = true;
            impl->lastExcitationSample = impl->totalSamples;
        }

        for (std::size_t channel = 0; channel < impl->channels; ++channel)
        {
            float* const channelInput = impl->inputRing.get() + channel * fftLength;
            float* const channelOutput = impl->outputRing.get() + channel * fftLength;
            const std::size_t ringIndex = impl->current;
            const float dryOutput = channelOutput[ringIndex];
            const float tailOutput =
                impl->tailOutputRing.get()[channel * fftLength + ringIndex];
            channelOutput[impl->current] = 0.0f;
            impl->tailOutputRing.get()[channel * fftLength + ringIndex] = 0.0f;
            channelInput[impl->current] = inputs[channel];

            float output = dryOutput;
            if (tailOutput != 0.0f)
            {
                output += tailOutput;
                // The tail may reach -1 dBFS, but may not raise an already
                // hotter dry sample. This guard is bypassed on the neutral path.
                const float limit = std::max(tailOutputCeiling, std::fabs(dryOutput));
                output = std::max(-limit, std::min(limit, output));
            }
            channelData[channel][sample] = output;
        }

        ++impl->totalSamples;
        if (++impl->current == fftLength)
            impl->current = 0;

        if (++impl->samplesSinceFrame == hopLength)
        {
            impl->samplesSinceFrame = 0;
            // A frame ending at input sample n is scheduled at output n + 1.
            // Its sample i is therefore emitted at n + 1 + i = inputTime + N.
            impl->processFrame();
        }
    }
}

int NeutralStft::latencySamples() const noexcept
{
    return static_cast<int>(fftLength);
}
} // namespace spectral_decay
