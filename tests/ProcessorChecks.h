// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

inline juce::AudioProcessorParameter& findParameter(juce::AudioProcessor& processor,
                                                    const juce::String& name)
{
    for (auto* parameter : processor.getParameters())
        if (parameter->getName(64) == name)
            return *parameter;
    throw std::runtime_error("Missing parameter: " + name.toStdString());
}

inline void checkTailControls(juce::AudioProcessor& processor)
{
    auto& amount = findParameter(processor, "Tail amount");
    auto& mode = findParameter(processor, "Tail mode");
    auto& low = findParameter(processor, "Low frequency");
    auto& high = findParameter(processor, "High frequency");
    if (std::abs(amount.getValue()) > 1.0e-6f)
        throw std::runtime_error("The tail must default to neutral");

    amount.setValueNotifyingHost(0.75f);
    mode.setValueNotifyingHost(1.0f);
    const float savedLow = low.getValueForText("200");
    const float savedHigh = high.getValueForText("4000");
    low.setValueNotifyingHost(savedLow);
    high.setValueNotifyingHost(savedHigh);
    juce::MemoryBlock state;
    processor.getStateInformation(state);
    if (state.getSize() == 0)
        throw std::runtime_error("Parameter state is empty");
    amount.setValueNotifyingHost(0.0f);
    mode.setValueNotifyingHost(0.0f);
    low.setValueNotifyingHost(low.getValueForText("500"));
    high.setValueNotifyingHost(high.getValueForText("1500"));
    processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    if (std::abs(amount.getValue() - 0.75f) > 0.001f || mode.getValue() < 0.99f
        || std::abs(low.getValue() - savedLow) > 0.001f
        || std::abs(high.getValue() - savedHigh) > 0.001f)
        throw std::runtime_error("Parameter state was not restored");
    low.setValueNotifyingHost(low.getValueForText("500"));
    high.setValueNotifyingHost(high.getValueForText("1500"));

    auto layout = processor.getBusesLayout();
    layout.inputBuses.set(0, juce::AudioChannelSet::mono());
    layout.outputBuses.set(0, juce::AudioChannelSet::mono());
    if (!processor.setBusesLayout(layout))
        throw std::runtime_error("Mono layout rejected");
    constexpr double rate = 48000.0;
    constexpr int blockSize = 257;
    constexpr int signalLength = 14400;
    processor.setRateAndBufferSizeDetails(rate, blockSize);
    processor.prepareToPlay(rate, blockSize);
    const int latency = processor.getLatencySamples();
    const int length = signalLength + latency + 48000;

    auto render = [&](float tailAmount, float tailMode, bool enableDuringSignal = false)
    {
        amount.setValueNotifyingHost(enableDuringSignal ? 0.0f : tailAmount);
        mode.setValueNotifyingHost(tailMode);
        processor.reset();
        juce::AudioBuffer<float> buffer(1, blockSize);
        juce::MidiBuffer midi;
        double energy = 0.0;
        bool enabled = !enableDuringSignal;
        for (int position = 0; position < length;)
        {
            if (!enabled && position >= signalLength / 2)
            {
                amount.setValueNotifyingHost(tailAmount);
                enabled = true;
            }
            const int count = std::min(blockSize, length - position);
            buffer.setSize(1, count, false, false, true);
            for (int n = 0; n < count; ++n)
            {
                const int time = position + n;
                const float sample = time < signalLength
                    ? static_cast<float>(0.2 * std::sin(2.0 * juce::MathConstants<double>::pi
                                                        * 1000.0 * time / rate))
                    : 0.0f;
                buffer.setSample(0, n, sample);
            }
            processor.processBlock(buffer, midi);
            for (int n = 0; n < count; ++n)
            {
                const float sample = buffer.getSample(0, n);
                if (!std::isfinite(sample))
                    throw std::runtime_error("Non-finite tail output");
                if (position + n >= signalLength + latency + 4800)
                    energy += static_cast<double>(sample) * sample;
            }
            position += count;
        }
        return energy;
    };

    const double neutral = render(0.0f, 0.0f);
    for (const float selectedMode : {0.0f, 1.0f})
    {
        const double tail = render(1.0f, selectedMode);
        if (tail <= neutral + 1.0e-8)
            throw std::runtime_error("Tail parameter did not extend the test signal");
        std::cout << "tail_mode=" << selectedMode << ", neutral_energy=" << neutral
                  << ", tail_energy=" << tail << '\n';
    }
    if (render(1.0f, 0.0f, true) <= neutral + 1.0e-8)
        throw std::runtime_error("Changing the tail during playback had no effect");
    amount.setValueNotifyingHost(0.0f);
    mode.setValueNotifyingHost(0.0f);
    processor.releaseResources();
}

inline void checkDecayWithOtherAudio(juce::AudioProcessor& processor)
{
    auto& amount = findParameter(processor, "Tail amount");
    auto& mode = findParameter(processor, "Tail mode");
    auto& low = findParameter(processor, "Low frequency");
    auto& high = findParameter(processor, "High frequency");
    constexpr double rate = 48000.0;
    constexpr int blockSize = 257;
    constexpr int length = 48000;
    auto layout = processor.getBusesLayout();
    layout.inputBuses.set(0, juce::AudioChannelSet::mono());
    layout.outputBuses.set(0, juce::AudioChannelSet::mono());
    if (!processor.setBusesLayout(layout))
        throw std::runtime_error("Mono layout rejected");
    processor.setRateAndBufferSizeDetails(rate, blockSize);
    processor.prepareToPlay(rate, blockSize);
    const int latency = processor.getLatencySamples();

    auto render = [&](float strength, float selectedMode, int lowHz, int highHz,
                      bool switchBandDuringPlayback = false)
    {
        amount.setValueNotifyingHost(strength);
        mode.setValueNotifyingHost(selectedMode);
        low.setValueNotifyingHost(low.getValueForText(juce::String(lowHz)));
        high.setValueNotifyingHost(high.getValueForText(juce::String(highHz)));
        processor.reset();
        juce::AudioBuffer<float> buffer(1, blockSize);
        juce::MidiBuffer midi;
        std::vector<float> output(static_cast<std::size_t>(length + latency));
        bool switched = !switchBandDuringPlayback;
        for (int position = 0; position < length + latency;)
        {
            if (!switched && position >= static_cast<int>(0.1 * rate))
            {
                low.setValueNotifyingHost(low.getValueForText("500"));
                high.setValueNotifyingHost(high.getValueForText("1500"));
                switched = true;
            }
            const int count = std::min(blockSize, length + latency - position);
            buffer.setSize(1, count, false, false, true);
            for (int n = 0; n < count; ++n)
            {
                const int t = position + n;
                const double time = t / rate;
                const double sample = t < length
                    ? 0.3 * std::exp(-time / 0.12)
                          * std::sin(2.0 * juce::MathConstants<double>::pi * 1000.0 * time)
                          + 0.08 * std::sin(2.0 * juce::MathConstants<double>::pi * 8000.0 * time)
                    : 0.0;
                buffer.setSample(0, n, static_cast<float>(sample));
            }
            processor.processBlock(buffer, midi);
            for (int n = 0; n < count; ++n)
            {
                const auto sample = buffer.getSample(0, n);
                if (!std::isfinite(sample))
                    throw std::runtime_error("Non-finite decaying-signal output");
                output[static_cast<std::size_t>(position + n)] = sample;
            }
            position += count;
        }
        return output;
    };

    const auto reference = render(0.0f, 0.0f, 500, 1500);
    for (float selectedMode : {0.0f, 1.0f})
    {
        const auto affected = render(1.0f, selectedMode, 500, 1500);
        const auto half = render(0.5f, selectedMode, 500, 1500);
        const auto elsewhere = render(1.0f, selectedMode, 6000, 10000);
        const auto switched = render(1.0f, selectedMode, 6000, 10000, true);
        double selectedEnergy = 0.0;
        double halfEnergy = 0.0;
        double otherEnergy = 0.0;
        double switchedEnergy = 0.0;
        for (int t = static_cast<int>(0.2 * rate); t < static_cast<int>(0.8 * rate); ++t)
        {
            const auto index = static_cast<std::size_t>(t + latency);
            const double selectedDifference = affected[index] - reference[index];
            const double halfDifference = half[index] - reference[index];
            const double otherDifference = elsewhere[index] - reference[index];
            const double switchedDifference = switched[index] - reference[index];
            selectedEnergy += selectedDifference * selectedDifference;
            halfEnergy += halfDifference * halfDifference;
            otherEnergy += otherDifference * otherDifference;
            switchedEnergy += switchedDifference * switchedDifference;
        }
        if (selectedEnergy <= 1.0e-8 || selectedEnergy <= otherEnergy * 4.0)
            throw std::runtime_error("Frequency-selected decay not detected with other audio playing");
        if (halfEnergy <= 0.0 || halfEnergy >= selectedEnergy)
            throw std::runtime_error("Tail amount does not control the effect strength");
        if (switchedEnergy <= otherEnergy * 4.0)
            throw std::runtime_error("Changing the frequency range during playback had no effect");
        std::cout << "decaying_signal_mode=" << selectedMode
                  << ", selected_difference_energy=" << selectedEnergy
                  << ", outside_difference_energy=" << otherEnergy
                  << ", switched_difference_energy=" << switchedEnergy << '\n';
    }
    amount.setValueNotifyingHost(0.0f);
    mode.setValueNotifyingHost(0.0f);
    low.setValueNotifyingHost(low.getValueForText("500"));
    high.setValueNotifyingHost(high.getValueForText("1500"));
    processor.releaseResources();
}

inline void checkProcessor(juce::AudioProcessor& processor)
{
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
    checkTailControls(processor);
    checkDecayWithOtherAudio(processor);
}
