// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PeakBandStft.h"

#include <pffft/pffft.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>

namespace spectral_decay
{
namespace
{
constexpr double pi = 3.1415926535897932384626433832795;
constexpr double twoPi = 2.0 * pi;
constexpr float wolaScale = 1.0f / (2.0f * static_cast<float>(PeakBandStft::fftSize));
constexpr float epsilon = 1.0e-9f;
constexpr std::uint64_t noSample = std::numeric_limits<std::uint64_t>::max();
constexpr double returnToNeutralFadeSeconds = 0.005;

static_assert(std::atomic<float>::is_always_lock_free,
              "PeakBandStft requires lock-free float atomics");

struct AlignedBuffer
{
    float* allocation = nullptr;
    float* data = nullptr;
    ~AlignedBuffer() { pffft_aligned_free(allocation); }
    void allocate(std::size_t count)
    {
        allocation = static_cast<float*>(pffft_aligned_malloc(count * sizeof(float)));
        if (allocation == nullptr)
            throw std::bad_alloc();
        data = allocation;
    }
};

float bounded(float value, float low, float high, float fallback) noexcept
{
    if (!std::isfinite(value))
        return fallback;
    return std::max(low, std::min(high, value));
}
}

struct PeakBandStft::Impl final
{
    explicit Impl(double rate, std::size_t channelCount) : sampleRate(rate), channels(channelCount)
    {
        setup = pffft_new_setup(static_cast<int>(fftSize), PFFFT_REAL);
        if (setup == nullptr)
            throw std::bad_alloc();
        input.allocate(channels * fftSize);
        output.allocate(channels * fftSize);
        tail.allocate(channels * fftSize);
        frame.allocate(fftSize);
        spectrum.allocate(fftSize);
        work.allocate(fftSize);
        tailSpectrum.allocate(fftSize);
        tailFrame.allocate(fftSize);
        double sum = 0.0;
        double power = 0.0;
        for (std::size_t i = 0; i < fftSize; ++i)
        {
            const double hann = 0.5 - 0.5 * std::cos(twoPi * static_cast<double>(i) / fftSize);
            window[i] = static_cast<float>(std::sqrt(std::max(0.0, hann)));
            sum += window[i];
            power += static_cast<double>(window[i]) * window[i];
        }
        tonalScale = static_cast<float>(static_cast<double>(fftSize) / sum);
        noiseScale = static_cast<float>(std::sqrt(static_cast<double>(fftSize) / power));
        clear();
    }

    ~Impl() { if (setup != nullptr) pffft_destroy_setup(setup); }

    void clear() noexcept
    {
        std::fill_n(input.data, channels * fftSize, 0.0f);
        std::fill_n(output.data, channels * fftSize, 0.0f);
        std::fill_n(tail.data, channels * fftSize, 0.0f);
        magnitudes = {};
        phases = {};
        previousPhases = {};
        previousMagnitudes = {};
        relativePhase = {};
        peakOwner.fill(0);
        stereoRatio = {};
        fast = {};
        reference = {};
        extended = {};
        onset = {};
        lastActivity.fill(noSample);
        phaseAccumulator = {};
        previousRelativePhase = {};
        randomPhase = {};
        randomWalk = {};
        lastFrequency = {};
        generatedTailFrame.fill(0.0f);
        stereoPolarity = 0;
        cursor = 0;
        sinceFrame = 0;
        totalSamples = 0;
        tailDeadline = noSample;
        returnFadeLength = returnFadeRemaining = 0;
        controlsInitialized = false;
        targetFrequency = smoothFrequency = 1000.0f;
        targetQ = smoothQ = 1.0f;
        targetDecay = smoothDecay = 0.0f;
        targetToneNoise = smoothToneNoise = 0.5f;
        tailActive = false;
        for (std::size_t k = 0; k < numBins; ++k)
        {
            randomState[k] = 0x9e3779b9u ^ static_cast<std::uint32_t>(k * 0x85ebca6bu);
            if (randomState[k] == 0) randomState[k] = 1;
            randomPhase[k] = static_cast<float>(twoPi * unitRandom(randomState[k]));
            randomWalk[k] = randomPhase[k];
        }
    }

    static float unitRandom(std::uint32_t& state) noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float>(state) / 4294967296.0f;
    }

    void setTargets(float f, float q, float d, float tn) noexcept
    {
        targetFrequency = bounded(f, 40.0f, 16000.0f, 1000.0f);
        targetQ = bounded(q, 0.25f, 12.0f, 1.0f);
        targetDecay = bounded(d, -5.0f, 5.0f, 0.0f);
        targetToneNoise = bounded(tn, 0.0f, 1.0f, 0.5f);
        if (!controlsInitialized)
        {
            smoothFrequency = targetFrequency;
            smoothQ = targetQ;
            smoothDecay = targetDecay;
            smoothToneNoise = targetToneNoise;
            controlsInitialized = true;
        }
    }

    void smoothControls() noexcept
    {
        const float coefficient = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate * 0.02)));
        auto approach = [coefficient](float& current, float target) noexcept
        {
            current += (target - current) * coefficient;
            if (std::abs(target - current) < 1.0e-6f)
                current = target;
        };
        approach(smoothFrequency, targetFrequency);
        approach(smoothQ, targetQ);
        approach(smoothDecay, targetDecay);
        approach(smoothToneNoise, targetToneNoise);
        if (targetDecay == 0.0f && std::abs(smoothDecay) < 1.0e-6f)
            smoothDecay = 0.0f;
    }

    void processFrame() noexcept
    {
        const std::uint64_t frameEnd = totalSamples - 1;
        const std::uint64_t frameStart = frameEnd >= fftSize - 1 ? frameEnd - (fftSize - 1) : 0;
        const bool active = smoothDecay != 0.0f;
        bool anyPositiveTail = false;
        for (std::size_t c = 0; c < channels; ++c)
        {
            const float* in = input.data + c * fftSize;
            for (std::size_t i = 0; i < fftSize; ++i)
            {
                const std::size_t idx = cursor + i < fftSize ? cursor + i : cursor + i - fftSize;
                frame.data[i] = in[idx] * window[i];
            }
            pffft_transform_ordered(setup, frame.data, spectrum.data, work.data, PFFFT_FORWARD);
            std::copy_n(spectrum.data, fftSize, spectrumFor(c));
            magnitudes[c][0] = std::abs(spectrum.data[0]);
            phases[c][0] = spectrum.data[0] < 0.0f ? static_cast<float>(pi) : 0.0f;
            magnitudes[c][fftSize / 2] = std::abs(spectrum.data[1]);
            phases[c][fftSize / 2] = spectrum.data[1] < 0.0f ? static_cast<float>(pi) : 0.0f;
            for (std::size_t k = 1; k < fftSize / 2; ++k)
            {
                const float re = spectrum.data[2 * k];
                const float im = spectrum.data[2 * k + 1];
                magnitudes[c][k] = std::sqrt(re * re + im * im);
                phases[c][k] = std::atan2(im, re);
            }
        }

        updateStereoPolarity();
        for (std::size_t k = 0; k < numBins; ++k)
        {
            std::size_t owner = k;
            if (k > 0 && k < fftSize / 2)
            {
                const std::size_t first = k > 4 ? k - 4 : 1;
                const std::size_t last = std::min<std::size_t>(fftSize / 2 - 1, k + 4);
                float peak = -1.0f;
                for (std::size_t candidate = first; candidate <= last; ++candidate)
                {
                    const float value = std::max(magnitudes[0][candidate], channels == 2 ? magnitudes[1][candidate] : 0.0f);
                    if (value > peak) { peak = value; owner = candidate; }
                }
            }
            peakOwner[k] = static_cast<std::uint16_t>(owner);
            for (std::size_t c = 0; c < channels; ++c)
                relativePhase[c][k] = std::remainder(phases[c][k] - phases[c][owner], static_cast<float>(twoPi));
        }

        for (std::size_t k = 0; k < numBins; ++k)
        {
            const float frequency = static_cast<float>(static_cast<double>(k) * sampleRate / fftSize);
            const float weight = computeBandWeight(frequency, smoothFrequency, smoothQ);
            const float strength = weight * smoothDecay / 5.0f;
            float linked = 0.0f;
            std::size_t refChannel = 0;
            for (std::size_t c = 0; c < channels; ++c)
                if (magnitudes[c][k] > linked) { linked = magnitudes[c][k]; refChannel = c; }
            const float previousFast = fast[k];
            const float fastRelease = std::exp(-static_cast<float>(hopSize) / (static_cast<float>(sampleRate) * 0.018f));
            const float slowRelease = std::exp(-static_cast<float>(hopSize) / (static_cast<float>(sampleRate) * 2.5f));
            fast[k] = std::max(linked, previousFast * fastRelease);
            if (linked > 1.0e-7f)
            {
                reference[k] = std::max(linked, reference[k] * slowRelease);
                const bool newOnset = linked > 1.2f * previousFast + 1.0e-6f;
                onset[k] = newOnset ? 1.0f : onset[k] * std::exp(-static_cast<float>(hopSize) / (static_cast<float>(sampleRate) * 0.025f));
                // The analysis frame covers [frameStart, frameEnd]. Using its
                // oldest sample is conservative and prevents an FFT-window
                // sized extension beyond the actual input activity.
                lastActivity[k] = frameStart;
            }
            else
            {
                reference[k] *= slowRelease;
                if (reference[k] < 1.0e-8f) reference[k] = 0.0f;
            }
            if (linked <= 1.0e-7f)
                onset[k] *= std::exp(-static_cast<float>(hopSize) / (static_cast<float>(sampleRate) * 0.025f));
            float stereoMagnitude = linked;
            for (std::size_t c = 0; c < channels; ++c)
                if (linked > epsilon) stereoRatio[c][k] = magnitudes[c][k] / linked;
            const float weightStrength = std::abs(strength);
            float shortGain = 1.0f;
            const float ratio = std::max(0.0f, std::min(1.0f, fast[k] / std::max(reference[k], epsilon)));
            if (strength < 0.0f && linked < reference[k])
            {
                const float gamma = 3.5f * weightStrength * (1.0f - onset[k]);
                shortGain = std::max(0.004f, std::pow(std::max(ratio, 0.004f), gamma));
            }
            for (std::size_t c = 0; c < channels; ++c)
            {
                if (strength < 0.0f && k > 0 && k < fftSize / 2)
                {
                    const float gain = 1.0f - std::abs(strength) * (1.0f - shortGain);
                    spectrumFor(c)[2 * k] *= gain;
                    spectrumFor(c)[2 * k + 1] *= gain;
                }
            }

            const auto maximumAge = static_cast<std::uint64_t>(std::llround(sampleRate * maximumTailSeconds));
            const bool expired = lastActivity[k] == noSample || frameEnd > lastActivity[k] + maximumAge;
            if (expired)
            {
                extended[k] = 0.0f;
                lastActivity[k] = noSample;
                continue;
            }
            if (strength > 0.0f)
            {
                const float tau = 0.14f + 0.61f * std::max(0.0f, std::min(1.0f, strength));
                const float highDamping = 1.0f + smoothToneNoise * std::max(0.0f, std::log2(std::max(frequency, 800.0f) / 800.0f));
                const float release = std::exp(-static_cast<float>(hopSize) / (static_cast<float>(sampleRate) * tau / highDamping));
                extended[k] = std::max(linked, extended[k] * release);
            }
            if (strength > 0.0f && extended[k] > linked + 1.0e-8f)
                anyPositiveTail = true;

            const float expected = static_cast<float>(twoPi * static_cast<double>(k) * hopSize / fftSize);
            float omega = expected / static_cast<float>(hopSize);
            if (magnitudes[refChannel][k] > 1.0e-8f)
            {
                if (previousMagnitudes[refChannel][k] > 1.0e-8f)
                {
                    const float delta = phases[refChannel][k] - previousPhases[refChannel][k] - expected;
                    omega += std::remainder(delta, static_cast<float>(twoPi)) / static_cast<float>(hopSize);
                }
                phaseAccumulator[k] = phases[refChannel][k];
                lastFrequency[k] = omega;
            }
            previousRelativePhase[k] = channels == 2 ? std::remainder(phases[1][k] - phases[0][k], static_cast<float>(twoPi)) : 0.0f;
        }

        for (std::size_t c = 0; c < channels; ++c)
        {
            pffft_transform_ordered(setup, spectrumFor(c), frame.data, work.data, PFFFT_BACKWARD);
            float* out = output.data + c * fftSize;
            for (std::size_t i = 0; i < fftSize; ++i)
            {
                const std::size_t idx = cursor + i < fftSize ? cursor + i : cursor + i - fftSize;
                out[idx] += frame.data[i] * window[i] * wolaScale;
            }
        }

        if (anyPositiveTail && active)
            synthesizeTail(frameStart, frameEnd);
        if (anyPositiveTail)
        {
            const auto maximumAge = static_cast<std::uint64_t>(std::llround(sampleRate * maximumTailSeconds));
            for (std::size_t k = 1; k < fftSize / 2; ++k)
                if (lastActivity[k] != noSample && extended[k] > 0.0f)
                    tailDeadline = std::max(tailDeadline == noSample ? 0 : tailDeadline,
                                            lastActivity[k] + maximumAge + fftSize);
        }
        for (std::size_t c = 0; c < channels; ++c)
            for (std::size_t k = 0; k < numBins; ++k)
            {
                previousPhases[c][k] = phases[c][k];
                previousMagnitudes[c][k] = magnitudes[c][k];
            }
        tailActive = anyPositiveTail;
    }

    float* spectrumFor(std::size_t c) noexcept
    {
        return spectra.data() + c * fftSize;
    }

    void synthesizeTail(std::uint64_t frameStart, std::uint64_t frameEnd) noexcept
    {
        const float toneMix = std::cos(static_cast<float>(pi * 0.5) * smoothToneNoise);
        const float noiseMix = std::sin(static_cast<float>(pi * 0.5) * smoothToneNoise);
        const auto maximumAge = static_cast<std::uint64_t>(std::llround(sampleRate * maximumTailSeconds));
        for (std::size_t c = 0; c < channels; ++c)
        {
            std::fill_n(tailSpectrum.data, fftSize, 0.0f);
            for (std::size_t k = 1; k < fftSize / 2; ++k)
            {
                const float frequency = static_cast<float>(static_cast<double>(k) * sampleRate / fftSize);
                const float strength = computeBandWeight(frequency, smoothFrequency, smoothQ) * smoothDecay / 5.0f;
                if (strength <= 0.0f || extended[k] <= 0.0f || lastActivity[k] == noSample)
                    continue;
                const std::uint64_t deadline = lastActivity[k] + maximumAge;
                if (frameEnd > deadline) { extended[k] = 0.0f; continue; }
                const float linked = std::max(magnitudes[0][k], channels == 2 ? magnitudes[1][k] : 0.0f);
                const auto remaining = deadline - frameEnd;
                const float fadeOut = remaining < hopSize
                    ? 0.5f - 0.5f * std::cos(static_cast<float>(pi) *
                                             static_cast<float>(remaining) / static_cast<float>(hopSize))
                    : 1.0f;
                const float deficit = std::max(0.0f, extended[k] - linked) *
                                      std::min(1.0f, strength) * fadeOut;
                if (deficit <= 0.0f) continue;
                const std::size_t owner = peakOwner[k];
                const float channelRatio = c == 0 ? stereoRatio[0][k] : stereoRatio[1][k];
                const float magnitude = deficit * channelRatio;
                const float tonePhase = phaseAccumulator[owner] + relativePhase[c][k];
                const float noisePhase = randomWalk[k] + relativePhase[c][k];
                const float micro = 1.0f + 0.025f * smoothToneNoise * std::sin(randomWalk[k] * 0.7f);
                tailSpectrum.data[2 * k] = magnitude * micro * (toneMix * tonalScale * std::cos(tonePhase) + noiseMix * noiseScale * std::cos(noisePhase));
                tailSpectrum.data[2 * k + 1] = magnitude * micro * (toneMix * tonalScale * std::sin(tonePhase) + noiseMix * noiseScale * std::sin(noisePhase));
            }
            pffft_transform_ordered(setup, tailSpectrum.data, tailFrame.data, work.data, PFFFT_BACKWARD);
            if (c == 0 && channels == 2 && stereoPolarity != 0)
                std::copy_n(tailFrame.data, fftSize, generatedTailFrame.data());
            if (c == 1 && channels == 2 && stereoPolarity != 0)
                for (std::size_t i = 0; i < fftSize; ++i)
                    tailFrame.data[i] = static_cast<float>(stereoPolarity) * generatedTailFrame[i];
            float* outputTail = tail.data + c * fftSize;
            for (std::size_t i = 0; i < fftSize; ++i)
            {
                const std::uint64_t timeline = frameStart + i;
                const std::size_t idx = cursor + i < fftSize ? cursor + i : cursor + i - fftSize;
                if (timeline > frameEnd + maximumAge) continue;
                const float fade = 0.5f - 0.5f * std::cos(static_cast<float>(pi) * std::min(1.0f, static_cast<float>(i + 1) / static_cast<float>(hopSize)));
                outputTail[idx] += tailFrame.data[i] * window[i] * wolaScale * fade;
            }
        }
        for (std::size_t k = 1; k < fftSize / 2; ++k)
        {
            randomWalk[k] = std::remainder(randomWalk[k] + lastFrequency[k] * hopSize + (unitRandom(randomState[k]) - 0.5f) * smoothToneNoise * 0.35f, static_cast<float>(twoPi));
            phaseAccumulator[k] = std::remainder(phaseAccumulator[k] + lastFrequency[k] * hopSize, static_cast<float>(twoPi));
        }
    }

    void updateStereoPolarity() noexcept
    {
        if (channels != 2) { stereoPolarity = 0; return; }
        bool identical = true, opposite = true, silent = true;
        for (std::size_t i = 0; i < fftSize; ++i)
        {
            const std::size_t index = cursor + i < fftSize ? cursor + i : cursor + i - fftSize;
            const float left = input.data[index], right = input.data[fftSize + index];
            identical = identical && left == right;
            opposite = opposite && left == -right;
            silent = silent && left == 0.0f && right == 0.0f;
        }
        if (!silent) stereoPolarity = identical ? 1 : (opposite ? -1 : 0);
    }

    double sampleRate;
    std::size_t channels;
    PFFFT_Setup* setup = nullptr;
    AlignedBuffer input, output, tail, frame, spectrum, work, tailSpectrum, tailFrame;
    alignas(64) std::array<float, fftSize * maxChannels> spectra{};
    std::array<float, fftSize> window{};
    std::array<std::array<float, numBins>, maxChannels> magnitudes{}, phases{}, previousPhases{}, previousMagnitudes{}, relativePhase{}, stereoRatio{};
    std::array<std::uint16_t, numBins> peakOwner{};
    std::array<float, fftSize> generatedTailFrame{};
    std::array<float, numBins> fast{}, reference{}, extended{}, onset{}, phaseAccumulator{}, previousRelativePhase{}, randomPhase{}, randomWalk{}, lastFrequency{};
    std::array<std::uint32_t, numBins> randomState = [] { std::array<std::uint32_t, numBins> a{}; for (std::size_t i=0;i<numBins;++i) a[i]=0x9e3779b9u ^ static_cast<std::uint32_t>(i*0x85ebca6bu); return a; }();
    std::array<std::uint64_t, numBins> lastActivity{};
    std::size_t cursor = 0, sinceFrame = 0;
    std::uint64_t totalSamples = 0, tailDeadline = noSample;
    std::size_t returnFadeLength = 0, returnFadeRemaining = 0;
    float targetFrequency=1000.0f, smoothFrequency=1000.0f, targetQ=1.0f, smoothQ=1.0f;
    float targetDecay=0.0f, smoothDecay=0.0f, targetToneNoise=0.5f, smoothToneNoise=0.5f;
    float tonalScale=1.0f, noiseScale=1.0f;
    bool controlsInitialized=false, tailActive=false;
    int stereoPolarity=0;
};

PeakBandStft::PeakBandStft() = default;
PeakBandStft::~PeakBandStft() = default;

float PeakBandStft::computeBandWeight(float freqHz, float centerHz, float q) noexcept
{
    if (!std::isfinite(freqHz) || !std::isfinite(centerHz) || !std::isfinite(q)
        || freqHz <= 0.0f || centerHz <= 0.0f)
        return 0.0f;
    q = bounded(q, 0.25f, 12.0f, 1.0f);
    const float u = q * std::log2(freqHz / centerHz);
    const float distance = std::abs(u);
    if (distance >= 3.5f) return 0.0f;
    float taper = 1.0f;
    if (distance > 2.8f)
    {
        const float x = (distance - 2.8f) / 0.7f;
        taper = 0.5f + 0.5f * std::cos(static_cast<float>(pi) * x);
    }
    return std::max(0.0f, std::min(1.0f, std::exp(-0.5f * u * u) * taper));
}

void PeakBandStft::prepare(double sampleRate, std::size_t channels)
{
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0 || channels == 0 || channels > maxChannels)
        throw std::invalid_argument("PeakBandStft requires a positive sample rate and one or two channels");
    impl = std::make_unique<Impl>(sampleRate, channels);
}
void PeakBandStft::reset() noexcept { if (impl) impl->clear(); }
void PeakBandStft::setBandFrequencyHz(float hz) noexcept { requestedFrequency.store(bounded(hz,40.0f,16000.0f,1000.0f),std::memory_order_relaxed); }
void PeakBandStft::setBandQ(float q) noexcept { requestedQ.store(bounded(q,0.25f,12.0f,1.0f),std::memory_order_relaxed); }
void PeakBandStft::setBandDecay(float decay) noexcept { requestedDecay.store(bounded(decay,-5.0f,5.0f,0.0f),std::memory_order_relaxed); }
void PeakBandStft::setToneNoise(float toneNoise) noexcept { requestedToneNoise.store(bounded(toneNoise,0.0f,1.0f,0.5f),std::memory_order_relaxed); }
void PeakBandStft::process(float* const* channels, std::size_t numSamples) noexcept
{
    if (numSamples == 0) return;
    assert(impl != nullptr && channels != nullptr);
    for (std::size_t c=0;c<impl->channels;++c) assert(channels[c] != nullptr);
    impl->setTargets(requestedFrequency.load(std::memory_order_relaxed),requestedQ.load(std::memory_order_relaxed),requestedDecay.load(std::memory_order_relaxed),requestedToneNoise.load(std::memory_order_relaxed));
    for (std::size_t n=0;n<numSamples;++n)
    {
        const float previousDecay = impl->smoothDecay;
        impl->smoothControls();
        if (previousDecay != 0.0f && impl->smoothDecay == 0.0f)
        {
            bool queuedTail = false;
            for (std::size_t i = 0; i < impl->channels * fftSize; ++i)
                queuedTail = queuedTail || impl->tail.data[i] != 0.0f;
            impl->returnFadeLength = queuedTail
                ? std::max<std::size_t>(1, static_cast<std::size_t>(std::llround(impl->sampleRate * returnToNeutralFadeSeconds)))
                : 0;
            impl->returnFadeRemaining = impl->returnFadeLength;
        }
        else if (impl->smoothDecay > 0.0f)
        {
            impl->returnFadeRemaining = impl->returnFadeLength = 0;
        }
        float inputSamples[maxChannels]{};
        for (std::size_t c=0;c<impl->channels;++c) inputSamples[c]=channels[c][n];
        for (std::size_t c=0;c<impl->channels;++c)
        {
            const std::size_t ring=impl->cursor;
            float* out=impl->output.data+c*fftSize;
            float* tail=impl->tail.data+c*fftSize;
            float value=out[ring];
            const float tailValue=tail[ring];
            out[ring]=0.0f; tail[ring]=0.0f;
            impl->input.data[c*fftSize+ring]=inputSamples[c];
            if (tailValue != 0.0f)
            {
                float tailGain = 1.0f;
                if (impl->smoothDecay == 0.0f)
                {
                    if (impl->returnFadeRemaining <= 1)
                        tailGain = 0.0f;
                    else
                    {
                        const auto elapsed = impl->returnFadeLength - impl->returnFadeRemaining;
                        tailGain = 0.5f + 0.5f * std::cos(static_cast<float>(pi) *
                            static_cast<float>(elapsed) / static_cast<float>(impl->returnFadeLength - 1));
                    }
                }
                if (impl->tailDeadline != noSample)
                {
                    if (impl->totalSamples >= impl->tailDeadline)
                        tailGain = 0.0f;
                    else
                    {
                        const auto remaining = impl->tailDeadline - impl->totalSamples;
                        if (remaining <= hopSize)
                            tailGain *= 0.5f - 0.5f * std::cos(static_cast<float>(pi) *
                                static_cast<float>(remaining) / static_cast<float>(hopSize));
                    }
                }
                const float tailContribution = tailValue * tailGain;
                value += tailContribution;
                const float ceiling = std::max(tailOutputCeiling, std::abs(value - tailContribution));
                value = std::max(-ceiling, std::min(ceiling, value));
            }
            channels[c][n]=value;
        }
        ++impl->totalSamples;
        if (impl->returnFadeRemaining > 0)
            --impl->returnFadeRemaining;
        if (++impl->cursor==fftSize) impl->cursor=0;
        if (++impl->sinceFrame==hopSize)
        {
            impl->sinceFrame=0;
            impl->processFrame();
        }
    }
}
}
