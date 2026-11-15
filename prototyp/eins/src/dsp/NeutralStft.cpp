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
// Reversible prototype value: slower release than the reported 120-ms input
// decay so a measurable, bounded spectral surplus is produced.
constexpr double tailTimeConstantSeconds = 0.400;
constexpr double minimumBandHz = 40.0;
constexpr double maximumBandHz = 16000.0;
constexpr double bandEdgeWidthHz = 100.0;
constexpr double maximumTailSeconds = 2.0;
constexpr double pi = twoPi * 0.5;

static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
              "Tail settings require a lock-free 32-bit atomic");

std::uint16_t quantizeBandHz(float hz) noexcept
{
    return static_cast<std::uint16_t>(static_cast<std::uint32_t>(hz * 4.0f + 0.5f));
}

double decodeBandHz(std::uint16_t value) noexcept
{
    return static_cast<double>(value) * 0.25;
}

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

        updateBandMask();
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
        smoothedAmount = 0.0f;
        smoothedNoiseMix = 0.0f;
        smoothedLowHz = 500.0;
        smoothedHighHz = 1500.0;
        bandInitialized = false;
        currentMagnitude = {};
        previousMagnitude = {};
        currentPhase = {};
        previousPhase = {};
        tonePhase = {};
        stereoRatio = {};
        noiseRelativePhase = {};
        generatedTailFrame = {};
        linkedEnvelope = {};
        binOmega = {};
        tonePhaseValid = {};
        binLastInputSample.fill(noSample);
        binTailStartSample.fill(noSample);
        tailStartSample = noSample;
        tailDeadlineSample = noSample;
        tailActive = false;
        stereoPolarity = 0;
        for (std::size_t bin = 0; bin < spectrumBinCount; ++bin)
        {
            stereoRatio[0][bin] = 1.0f;
            stereoRatio[1][bin] = 1.0f;
        }
        for (std::size_t bin = 0; bin < spectrumBinCount; ++bin)
        {
            noiseRandomState[bin] =
                0x9e3779b9u ^ (static_cast<std::uint32_t>(bin) * 0x85ebca6bu);
            if (noiseRandomState[bin] == 0)
                noiseRandomState[bin] = 1;
        }
        noiseFramePhase = {};
    }

    void clearTrackingState() noexcept
    {
        currentMagnitude = {};
        previousMagnitude = {};
        currentPhase = {};
        previousPhase = {};
        tonePhase = {};
        stereoRatio = {};
        noiseRelativePhase = {};
        linkedEnvelope = {};
        binOmega = {};
        tonePhaseValid = {};
        binLastInputSample.fill(noSample);
        binTailStartSample.fill(noSample);
        tailStartSample = noSample;
        tailDeadlineSample = noSample;
        tailActive = false;
        for (std::size_t bin = 0; bin < spectrumBinCount; ++bin)
        {
            stereoRatio[0][bin] = 1.0f;
            stereoRatio[1][bin] = 1.0f;
        }
    }

    void updateBandMask() noexcept
    {
        bandMask.fill(0.0f);
        firstBandBin = spectrumBinCount;
        lastBandBin = 0;

        const double nyquist = sampleRate * 0.5;
        const double low = std::min(smoothedLowHz, nyquist);
        const double high = std::min(smoothedHighHz, nyquist);
        if (high <= low)
            return;

        const double edgeWidth = std::min(bandEdgeWidthHz, (high - low) * 0.5);
        for (std::size_t bin = 1; bin < fftLength / 2; ++bin)
        {
            const double frequency = static_cast<double>(bin) * sampleRate /
                                     static_cast<double>(fftLength);
            if (frequency < low || frequency > high)
                continue;

            double weight = 1.0;
            if (frequency < low + edgeWidth)
            {
                const double position = (frequency - low) / edgeWidth;
                weight = 0.5 - 0.5 * std::cos(pi * position);
            }
            else if (frequency > high - edgeWidth)
            {
                const double position = (high - frequency) / edgeWidth;
                weight = 0.5 - 0.5 * std::cos(pi * position);
            }

            bandMask[bin] = static_cast<float>(weight);
            if (weight > 0.0)
            {
                firstBandBin = std::min(firstBandBin, bin);
                lastBandBin = bin;
            }
        }
    }

    void updateStereoPolarityFromInputWindow() noexcept
    {
        if (channels != 2)
        {
            stereoPolarity = 0;
            return;
        }

        bool fullySilent = true;
        bool identical = true;
        bool opposite = true;
        const float* const left = inputRing.get();
        const float* const right = inputRing.get() + fftLength;
        for (std::size_t i = 0; i < fftLength; ++i)
        {
            const std::size_t ringIndex = current + i < fftLength
                                              ? current + i
                                              : current + i - fftLength;
            const float leftSample = left[ringIndex];
            const float rightSample = right[ringIndex];
            fullySilent = fullySilent && leftSample == 0.0f && rightSample == 0.0f;
            identical = identical && leftSample == rightSample;
            opposite = opposite && rightSample == -leftSample;
        }

        // Silence carries no new mapping evidence; retain the last proven
        // relationship. Any non-silent mixed window disables linked copying.
        if (fullySilent)
            return;
        stereoPolarity = identical ? 1 : (opposite ? -1 : 0);
    }

    void processFrame() noexcept
    {
        updateBandMask();
        updateStereoPolarityFromInputWindow();
        const bool spectrumTracking = targetAmount > 0.0f || smoothedAmount > 0.0f;
        if (!spectrumTracking)
            clearTrackingState();
        else
            currentMagnitude = {};

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
                     bin <= lastBandBin && bin < fftLength / 2; ++bin)
                {
                    if (bandMask[bin] <= 0.0f)
                        continue;

                    const float real = spectrum.get()[2 * bin];
                    const float imaginary = spectrum.get()[2 * bin + 1];
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
        {
            const std::uint64_t frameInputTime =
                latestSample >= fftLength - 1 ? latestSample - (fftLength - 1) : 0;
            updateTailState(latestSample, frameInputTime);
        }
        if (tailActive && smoothedAmount > 0.0f)
            renderTailFrame(frameStart);

        if (spectrumTracking)
        {
            for (std::size_t channel = 0; channel < channels; ++channel)
            {
                for (std::size_t bin = 1; bin < fftLength / 2; ++bin)
                {
                    previousMagnitude[channel][bin] = currentMagnitude[channel][bin];
                    previousPhase[channel][bin] = currentPhase[channel][bin];
                }
            }
        }
    }

    void setTargets(std::uint32_t packedSettings,
                    std::uint32_t packedBandSettings) noexcept
    {
        targetAmount = static_cast<float>(packedSettings & amountBits) /
                       static_cast<float>(amountBits);
        targetNoiseMix = (packedSettings & noiseModeBit) != 0 ? 1.0f : 0.0f;

        targetLowHz = decodeBandHz(
            static_cast<std::uint16_t>(packedBandSettings & 0xffffu));
        targetHighHz = decodeBandHz(
            static_cast<std::uint16_t>(packedBandSettings >> 16));
        targetLowHz = std::max(minimumBandHz, std::min(maximumBandHz, targetLowHz));
        targetHighHz = std::max(minimumBandHz, std::min(maximumBandHz, targetHighHz));
        if (targetLowHz > targetHighHz)
            std::swap(targetLowHz, targetHighHz);
        if (!bandInitialized)
        {
            // prepare/reset starts with no usable smoothed value. Apply the
            // first requested band directly; only subsequent edits ramp.
            smoothedLowHz = targetLowHz;
            smoothedHighHz = targetHighHz;
            bandInitialized = true;
        }
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

        const double bandStep = 1.0 /
                                std::max(1.0, sampleRate * controlSmoothingSeconds);
        smoothedLowHz += (targetLowHz - smoothedLowHz) * bandStep;
        smoothedHighHz += (targetHighHz - smoothedHighHz) * bandStep;
        if (std::abs(targetLowHz - smoothedLowHz) < 0.01)
            smoothedLowHz = targetLowHz;
        if (std::abs(targetHighHz - smoothedHighHz) < 0.01)
            smoothedHighHz = targetHighHz;
    }

    void updateTailState(std::uint64_t latestSample,
                         std::uint64_t frameInputTime) noexcept
    {
        // The per-bin envelope is stereo-linked to the louder channel. It
        // attacks immediately and releases with the prototype time constant;
        // synthesis later adds only envelope-minus-input magnitude.
        const double release = std::exp(
            -static_cast<double>(hopLength) /
            (sampleRate * tailTimeConstantSeconds));
        const double activityScale =
            static_cast<double>(tonalSpectrumScale) * 2.0 /
            static_cast<double>(fftLength);
        const auto maximumAge = static_cast<std::uint64_t>(
            sampleRate * maximumTailSeconds);

        bool anyTail = false;
        std::uint64_t earliestTail = noSample;
        std::uint64_t latestDeadline = 0;

        for (std::size_t bin = 1; bin < fftLength / 2; ++bin)
        {
            const float mask = bandMask[bin];
            float linkedInput = 0.0f;
            std::size_t referenceChannel = 0;
            for (std::size_t channel = 0; channel < channels; ++channel)
            {
                if (currentMagnitude[channel][bin] > linkedInput)
                {
                    linkedInput = currentMagnitude[channel][bin];
                    referenceChannel = channel;
                }
            }

            const double inputLevel = static_cast<double>(linkedInput) * activityScale;
            const bool significantInput = mask > 0.0f && inputLevel > activityThreshold;
            if (significantInput)
            {
                // The spectrum covers [latestSample-N+1, latestSample].
                // Use its oldest represented time as a conservative in-band
                // excitation timestamp so the two-second bound cannot drift
                // beyond the actual end of a burst by one FFT window.
                binLastInputSample[bin] = frameInputTime;
                for (std::size_t channel = 0; channel < channels; ++channel)
                {
                    stereoRatio[channel][bin] =
                        linkedInput > 1.0e-12f
                            ? currentMagnitude[channel][bin] / linkedInput
                            : 0.0f;
                }
            }

            if (mask > 0.0f && significantInput)
            {
                const double expectedAdvance = twoPi * static_cast<double>(bin) *
                                               static_cast<double>(hopLength) /
                                               static_cast<double>(fftLength);
                double omega = twoPi * static_cast<double>(bin) /
                               static_cast<double>(fftLength);
                if (previousMagnitude[referenceChannel][bin] > 1.0e-9f &&
                    currentMagnitude[referenceChannel][bin] > 1.0e-9f)
                {
                    const double phaseDifference =
                        static_cast<double>(currentPhase[referenceChannel][bin]) -
                        static_cast<double>(previousPhase[referenceChannel][bin]) -
                        expectedAdvance;
                    const double correction =
                        std::remainder(phaseDifference, twoPi) /
                        static_cast<double>(hopLength);
                    if (std::abs(correction) <= pi / static_cast<double>(fftLength))
                        omega += correction;
                }
                binOmega[bin] = omega;
                for (std::size_t channel = 0; channel < channels; ++channel)
                {
                    if (currentMagnitude[channel][bin] > 1.0e-12f)
                    {
                        tonePhase[channel][bin] = currentPhase[channel][bin];
                        tonePhaseValid[channel][bin] = true;
                    }
                }
                if (channels == 2 &&
                    currentMagnitude[0][bin] > 1.0e-9f &&
                    currentMagnitude[1][bin] > 1.0e-9f)
                {
                    noiseRelativePhase[1][bin] = static_cast<float>(std::remainder(
                        static_cast<double>(currentPhase[1][bin]) -
                            static_cast<double>(currentPhase[0][bin]),
                        twoPi));
                }
            }
            else if (binOmega[bin] != 0.0)
            {
                for (std::size_t channel = 0; channel < channels; ++channel)
                {
                    if (tonePhaseValid[channel][bin])
                        tonePhase[channel][bin] = static_cast<float>(std::remainder(
                            static_cast<double>(tonePhase[channel][bin]) +
                                binOmega[bin] * static_cast<double>(hopLength),
                            twoPi));
                }
            }

            const double envelope = std::max(
                static_cast<double>(linkedInput),
                static_cast<double>(linkedEnvelope[bin]) * release);
            linkedEnvelope[bin] = static_cast<float>(envelope);

            const bool expired =
                binLastInputSample[bin] == noSample ||
                latestSample > binLastInputSample[bin] + maximumAge;
            if (expired)
            {
                linkedEnvelope[bin] = 0.0f;
                binTailStartSample[bin] = noSample;
                continue;
            }

            bool surplus = false;
            for (std::size_t channel = 0; channel < channels; ++channel)
            {
                const double targetMagnitude =
                    envelope * static_cast<double>(stereoRatio[channel][bin]);
                if (targetMagnitude >
                    static_cast<double>(currentMagnitude[channel][bin]) + 1.0e-8)
                    surplus = true;
            }

            if (mask > 0.0f && surplus)
            {
                if (binTailStartSample[bin] == noSample)
                    binTailStartSample[bin] = latestSample;
                anyTail = true;
                earliestTail = std::min(earliestTail, binTailStartSample[bin]);
                latestDeadline = std::max(
                    latestDeadline, binLastInputSample[bin] + maximumAge);
            }
            else
            {
                binTailStartSample[bin] = noSample;
            }
        }

        tailActive = anyTail;
        if (anyTail)
        {
            tailStartSample = earliestTail;
            tailDeadlineSample = latestDeadline;
        }
        else
        {
            tailStartSample = noSample;
            tailDeadlineSample = noSample;
        }
    }

    void renderTailFrame(double frameStart) noexcept
    {
        // The noise phase sequence is deterministic after reset. In stereo,
        // channel 1 reuses the measured phase difference from channel 0.
        for (std::size_t bin = firstBandBin;
             bin <= lastBandBin && bin < fftLength / 2; ++bin)
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

        const auto latestSample = totalSamples - 1;
        const auto maximumAge = static_cast<std::uint64_t>(
            sampleRate * maximumTailSeconds);

        for (std::size_t channel = 0; channel < channels; ++channel)
        {
            if (channel == 1 && channels == 2 && stereoPolarity != 0)
            {
                for (std::size_t i = 0; i < fftLength; ++i)
                    tailFrame.get()[i] =
                        static_cast<float>(stereoPolarity) * generatedTailFrame[i];
            }
            else
            {
                for (std::size_t i = 0; i < fftLength; ++i)
                    spectrum.get()[i] = 0.0f;

                for (std::size_t bin = firstBandBin;
                     bin <= lastBandBin && bin < fftLength / 2; ++bin)
                {
                    if (bandMask[bin] <= 0.0f ||
                        binTailStartSample[bin] == noSample ||
                        binLastInputSample[bin] == noSample ||
                        latestSample > binLastInputSample[bin] + maximumAge)
                        continue;

                    const double targetMagnitude =
                        static_cast<double>(linkedEnvelope[bin]) *
                        static_cast<double>(stereoRatio[channel][bin]);
                    const double extraMagnitude = std::max(
                        0.0, targetMagnitude -
                                 static_cast<double>(currentMagnitude[channel][bin]));
                    if (extraMagnitude <= 0.0)
                        continue;

                    const double age = static_cast<double>(
                        latestSample - binTailStartSample[bin]);
                    const double fadeIn = 0.5 - 0.5 * std::cos(
                        pi * std::min(1.0, age / static_cast<double>(hopLength)));
                    const auto deadline = binLastInputSample[bin] + maximumAge;
                    const double remaining = static_cast<double>(deadline - latestSample);
                    const double fadeOut = remaining < static_cast<double>(hopLength)
                        ? 0.5 - 0.5 * std::cos(
                              pi * std::max(0.0, remaining /
                                                      static_cast<double>(hopLength)))
                        : 1.0;
                    const double magnitude = extraMagnitude * bandMask[bin] *
                                             fadeIn * fadeOut * smoothedAmount;
                    if (magnitude <= 0.0)
                        continue;

                    const double tonalPhase = tonePhaseValid[channel][bin]
                        ? static_cast<double>(tonePhase[channel][bin])
                        : static_cast<double>(currentPhase[channel][bin]);
                    const double noisePhase =
                        static_cast<double>(noiseFramePhase[bin]) +
                        static_cast<double>(noiseRelativePhase[channel][bin]);
                    const double tonalMagnitude = magnitude * tonalSpectrumScale;
                    const double noiseMagnitude = magnitude * noiseSpectrumScale;
                    const double real =
                        (1.0 - smoothedNoiseMix) * tonalMagnitude * std::cos(tonalPhase) +
                        smoothedNoiseMix * noiseMagnitude * std::cos(noisePhase);
                    const double imaginary =
                        (1.0 - smoothedNoiseMix) * tonalMagnitude * std::sin(tonalPhase) +
                        smoothedNoiseMix * noiseMagnitude * std::sin(noisePhase);
                    spectrum.get()[2 * bin] = static_cast<float>(real);
                    spectrum.get()[2 * bin + 1] = static_cast<float>(imaginary);
                }

                pffft_transform_ordered(setup.get(), spectrum.get(), tailFrame.get(), work.get(),
                                        PFFFT_BACKWARD);
                if (channel == 0 && channels == 2 && stereoPolarity != 0)
                {
                    for (std::size_t i = 0; i < fftLength; ++i)
                        generatedTailFrame[i] = tailFrame.get()[i];
                }
            }

            float* const channelTailOutput = tailOutputRing.get() + channel * fftLength;
            for (std::size_t i = 0; i < fftLength; ++i)
            {
                const double sampleTime = frameStart + static_cast<double>(i);
                if (sampleTime < static_cast<double>(tailStartSample) ||
                    sampleTime > static_cast<double>(tailDeadlineSample))
                    continue;

                const double age =
                    sampleTime - static_cast<double>(tailStartSample);
                const std::size_t ringIndex = current + i < fftLength
                                                  ? current + i
                                                  : current + i - fftLength;
                channelTailOutput[ringIndex] += static_cast<float>(
                    tailFrame.get()[i] * window[i] * wolaScale *
                    (0.5 - 0.5 * std::cos(
                               pi * std::min(1.0, age /
                                                     static_cast<double>(hopLength)))));
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
    ChannelBins currentPhase {};
    ChannelBins previousPhase {};
    ChannelBins tonePhase {};
    ChannelBins stereoRatio {};
    ChannelBins noiseRelativePhase {};
    alignas(requiredAlignment) std::array<float, fftLength> generatedTailFrame {};
    std::array<std::array<bool, spectrumBinCount>, 2> tonePhaseValid {};
    std::array<float, spectrumBinCount> linkedEnvelope {};
    std::array<double, spectrumBinCount> binOmega {};
    std::array<std::uint64_t, spectrumBinCount> binLastInputSample {};
    std::array<std::uint64_t, spectrumBinCount> binTailStartSample {};
    std::array<std::uint32_t, spectrumBinCount> noiseRandomState {};
    std::array<float, spectrumBinCount> noiseFramePhase {};
    std::size_t firstBandBin = spectrumBinCount;
    std::size_t lastBandBin = 0;
    double tonalSpectrumScale = 1.0;
    double noiseSpectrumScale = 1.0;
    float targetAmount = 0.0f;
    float targetNoiseMix = 0.0f;
    float smoothedAmount = 0.0f;
    float smoothedNoiseMix = 0.0f;
    double targetLowHz = 500.0;
    double targetHighHz = 1500.0;
    double smoothedLowHz = 500.0;
    double smoothedHighHz = 1500.0;
    bool bandInitialized = false;
    std::size_t current = 0;
    std::size_t samplesSinceFrame = 0;
    std::uint64_t totalSamples = 0;
    std::uint64_t tailStartSample = noSample;
    std::uint64_t tailDeadlineSample = noSample;
    int stereoPolarity = 0;
    bool tailActive = false;

    static constexpr std::uint64_t noSample =
        std::numeric_limits<std::uint64_t>::max();
};

NeutralStft::NeutralStft() = default;
NeutralStft::~NeutralStft() = default;

void NeutralStft::setTail(float amount,
                          TailMode mode,
                          float lowHz,
                          float highHz) noexcept
{
    if (!std::isfinite(amount))
        amount = 0.0f;
    amount = std::max(0.0f, std::min(1.0f, amount));
    if (!std::isfinite(lowHz))
        lowHz = 500.0f;
    if (!std::isfinite(highHz))
        highHz = 1500.0f;
    lowHz = std::max(static_cast<float>(minimumBandHz),
                     std::min(static_cast<float>(maximumBandHz), lowHz));
    highHz = std::max(static_cast<float>(minimumBandHz),
                      std::min(static_cast<float>(maximumBandHz), highHz));
    if (lowHz > highHz)
        std::swap(lowHz, highHz);

    const auto quantizedAmount = static_cast<std::uint32_t>(
        static_cast<double>(amount) * static_cast<double>(amountBits) + 0.5);
    const std::uint32_t packedSettings =
        quantizedAmount | (mode == TailMode::noise ? noiseModeBit : 0u);
    const std::uint32_t packedBandSettings =
        static_cast<std::uint32_t>(quantizeBandHz(lowHz)) |
        (static_cast<std::uint32_t>(quantizeBandHz(highHz)) << 16);
    requestedBandSettings.store(packedBandSettings, std::memory_order_release);
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

    impl->setTargets(requestedTailSettings.load(std::memory_order_acquire),
                     requestedBandSettings.load(std::memory_order_acquire));

    for (std::size_t sample = 0; sample < numSamples; ++sample)
    {
        impl->advanceControls();
        float inputs[2] = {};
        for (std::size_t channel = 0; channel < impl->channels; ++channel)
            inputs[channel] = channelData[channel][sample];

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
