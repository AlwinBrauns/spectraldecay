// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

inline void checkProcessor(juce::AudioProcessor& processor)
{
    if (processor.hasEditor())
        throw std::runtime_error("The neutral prototype must not expose an editor");

    for (const double rate : {44100.0, 48000.0})
        for (const int blockSize : {1, 64, 512, 257})
            for (const int channels : {1, 2})
            {
                auto layout = processor.getBusesLayout();
                const auto channelSet = channels == 1 ? juce::AudioChannelSet::mono()
                                                       : juce::AudioChannelSet::stereo();
                layout.inputBuses.set(0, channelSet);
                layout.outputBuses.set(0, channelSet);
                if (!processor.setBusesLayout(layout))
                    throw std::runtime_error("Required mono/stereo bus layout rejected");
                processor.setRateAndBufferSizeDetails(rate, blockSize);
                processor.prepareToPlay(rate, blockSize);
                processor.reset();
                const int latency = processor.getLatencySamples();
                if (latency <= 0)
                    throw std::runtime_error("Expected a positive STFT latency");

                const int inputLength = 4097;
                const int length = inputLength + latency + 8192;
                std::vector<std::vector<float>> output(
                    static_cast<std::size_t>(channels),
                    std::vector<float>(static_cast<std::size_t>(length)));
                juce::AudioBuffer<float> buffer(channels, blockSize);
                juce::MidiBuffer midi;
                for (int position = 0; position < length;)
                {
                    const int count = std::min(blockSize, length - position);
                    buffer.setSize(channels, count, false, false, true);
                    buffer.clear();
                    for (int channel = 0; channel < channels; ++channel)
                    {
                        const int impulsePosition = channel == 0 ? 257 : 613;
                        if (position <= impulsePosition && impulsePosition < position + count)
                            buffer.setSample(channel, impulsePosition - position,
                                             channel == 0 ? 1.0f : -0.5f);
                    }
                    processor.processBlock(buffer, midi);
                    for (int channel = 0; channel < channels; ++channel)
                        std::copy_n(buffer.getReadPointer(channel), count,
                                    output[static_cast<std::size_t>(channel)].begin() + position);
                    position += count;
                }

                for (int channel = 0; channel < channels; ++channel)
                {
                    const int impulsePosition = channel == 0 ? 257 : 613;
                    const auto& samples = output[static_cast<std::size_t>(channel)];
                    float maximum = 0.0f;
                    int maximumPosition = -1;
                    int maxima = 0;
                    double residualPeak = 0.0;
                    for (int n = 0; n < length; ++n)
                    {
                        const float sample = samples[static_cast<std::size_t>(n)];
                        if (!std::isfinite(sample))
                            throw std::runtime_error("Non-finite processor output");
                        const float magnitude = std::abs(sample);
                        if (magnitude > maximum)
                        {
                            maximum = magnitude;
                            maximumPosition = n;
                            maxima = 1;
                        }
                        else if (magnitude == maximum)
                            ++maxima;
                        const double reference = n == impulsePosition + latency
                                                     ? (channel == 0 ? 1.0 : -0.5)
                                                     : 0.0;
                        residualPeak = std::max(residualPeak, std::abs(sample - reference));
                    }
                    if (maxima != 1 || maximumPosition - impulsePosition != latency)
                        throw std::runtime_error("Ambiguous impulse or host latency mismatch");
                    std::cout << "fs=" << rate << ", block=" << blockSize
                              << ", channels=" << channels << ", channel=" << channel
                              << ", reported_latency=" << latency
                              << ", measured_latency=" << maximumPosition - impulsePosition
                              << ", residual_peak=" << residualPeak
                              << " (neutrality tolerance OPEN)\n";
                }
                processor.releaseResources();
            }
}
