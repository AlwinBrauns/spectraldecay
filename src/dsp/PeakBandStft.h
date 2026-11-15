// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <atomic>
#include <cstddef>
#include <memory>

namespace spectral_decay
{
class PeakBandStft final
{
public:
    static constexpr std::size_t maxChannels = 2;
    static constexpr std::size_t fftSize = 2048;
    static constexpr std::size_t hopSize = 512;
    static constexpr std::size_t numBins = fftSize / 2 + 1;
    static constexpr int latency = static_cast<int>(fftSize);
    static constexpr float tailOutputCeiling = 0.89125094f;
    static constexpr double maximumTailSeconds = 2.0;

    PeakBandStft();
    ~PeakBandStft();
    PeakBandStft(const PeakBandStft&) = delete;
    PeakBandStft& operator=(const PeakBandStft&) = delete;

    void prepare(double sampleRate, std::size_t channels);
    void reset() noexcept;
    void setBandFrequencyHz(float hz) noexcept;
    void setBandQ(float q) noexcept;
    void setBandDecay(float decay) noexcept;
    void setToneNoise(float toneNoise) noexcept;
    void process(float* const* channels, std::size_t numSamples) noexcept;
    [[nodiscard]] int latencySamples() const noexcept { return latency; }
    [[nodiscard]] static float computeBandWeight(float freqHz,
                                                 float centerHz,
                                                 float q) noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    std::atomic<float> requestedFrequency { 1000.0f };
    std::atomic<float> requestedQ { 1.0f };
    std::atomic<float> requestedDecay { 0.0f };
    std::atomic<float> requestedToneNoise { 0.5f };
};
}
