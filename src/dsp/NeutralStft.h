// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <cstddef>
#include <memory>

namespace spectral_decay
{
class NeutralStft final
{
public:
    static constexpr std::size_t fftSize = 2048;
    static constexpr std::size_t hopSize = 512;

    NeutralStft();
    ~NeutralStft();
    NeutralStft(const NeutralStft&) = delete;
    NeutralStft& operator=(const NeutralStft&) = delete;

    // Non-realtime: validates the rate/layout and prepares all FFT storage.
    void prepare(double sampleRate, std::size_t channels);
    void reset() noexcept;

    // Prepared channel count; non-null, writable planar buffers for numSamples.
    // A zero-sample call does not inspect the pointers or advance the stream.
    void process(float* const* channels, std::size_t numSamples) noexcept;
    [[nodiscard]] int latencySamples() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
