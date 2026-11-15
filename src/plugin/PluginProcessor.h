// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "dsp/PeakBandStft.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

namespace spectral_decay
{
class PeakBandAudioProcessor final : public juce::AudioProcessor
{
public:
    PeakBandAudioProcessor();
    void prepareToPlay(double sampleRate, int maximumBlockSize) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    const juce::String getName() const override { return "Spectral Decay Neutral"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Prototype 2"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }
    const juce::AudioProcessorValueTreeState& getParameters() const noexcept { return parameters; }
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void restoreDefaults() noexcept;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* bandFrequency = nullptr;
    std::atomic<float>* bandQ = nullptr;
    std::atomic<float>* bandDecay = nullptr;
    std::atomic<float>* toneNoise = nullptr;
    PeakBandStft engine;
    double sampleRate = 0.0;
    int preparedChannels = 0;
};
}
