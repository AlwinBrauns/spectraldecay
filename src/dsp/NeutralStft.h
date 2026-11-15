// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace spectral_decay
{
class NeutralStft final
{
public:
    enum class TailMode { tonal, noise };

    static constexpr std::size_t fftSize = 2048;
    static constexpr std::size_t hopSize = 512;

    NeutralStft();
    ~NeutralStft();
    NeutralStft(const NeutralStft&) = delete;
    NeutralStft& operator=(const NeutralStft&) = delete;

    // Non-realtime: validates the rate/layout and prepares all FFT storage.
    void prepare(double sampleRate, std::size_t channels);
    void reset() noexcept;

    // Lock-free setting exchange. A change is observed at the next non-empty
    // process call and smoothed by the audio path; amount is clamped to 0..1.
    void setTail(float amount, TailMode mode) noexcept;

    // Prepared channel count; non-null, writable planar buffers for numSamples.
    // A zero-sample call does not inspect the pointers or advance the stream.
    void process(float* const* channels, std::size_t numSamples) noexcept;
    [[nodiscard]] int latencySamples() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    std::atomic<std::uint32_t> requestedTailSettings { 0 };
};
}
