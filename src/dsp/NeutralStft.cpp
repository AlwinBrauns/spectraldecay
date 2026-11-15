// SPDX-License-Identifier: AGPL-3.0-or-later
#include "NeutralStft.h"

#include <pffft/pffft.h>

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
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
    explicit Impl(std::size_t channelCount) : channels(channelCount)
    {
        setup.create();
        inputRing.allocate(channels * fftLength);
        outputRing.allocate(channels * fftLength);
        frame.allocate(fftLength);
        spectrum.allocate(fftLength);
        work.allocate(fftLength);

        for (std::size_t i = 0; i < fftLength; ++i)
        {
            const double hann = 0.5 - 0.5 * std::cos(twoPi * static_cast<double>(i) /
                                                    static_cast<double>(fftLength));
            window[i] = static_cast<float>(std::sqrt(hann < 0.0 ? 0.0 : hann));
        }
        clearState();
    }

    void clearState() noexcept
    {
        for (std::size_t i = 0; i < channels * fftLength; ++i)
        {
            inputRing.get()[i] = 0.0f;
            outputRing.get()[i] = 0.0f;
        }
        current = 0;
        samplesSinceFrame = 0;
    }

    void processFrame() noexcept
    {
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
    }

    const std::size_t channels;
    Setup setup;
    AlignedFloats inputRing;
    AlignedFloats outputRing;
    AlignedFloats frame;
    AlignedFloats spectrum;
    AlignedFloats work;
    alignas(requiredAlignment) std::array<float, fftLength> window{};
    std::size_t current = 0;
    std::size_t samplesSinceFrame = 0;
};

NeutralStft::NeutralStft() = default;
NeutralStft::~NeutralStft() = default;

void NeutralStft::prepare(double sampleRate, std::size_t channels)
{
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0)
        throw std::invalid_argument("NeutralStft requires a finite, positive sample rate");
    if (channels != 1 && channels != 2)
        throw std::invalid_argument("NeutralStft supports one or two channels");

    auto prepared = std::make_unique<Impl>(channels);
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

    for (std::size_t sample = 0; sample < numSamples; ++sample)
    {
        float inputs[2] = {};
        for (std::size_t channel = 0; channel < impl->channels; ++channel)
            inputs[channel] = channelData[channel][sample];

        for (std::size_t channel = 0; channel < impl->channels; ++channel)
        {
            float* const channelInput = impl->inputRing.get() + channel * fftLength;
            float* const channelOutput = impl->outputRing.get() + channel * fftLength;
            const float output = channelOutput[impl->current];
            channelOutput[impl->current] = 0.0f;
            channelInput[impl->current] = inputs[channel];
            channelData[channel][sample] = output;
        }

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
