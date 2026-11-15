// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "dsp/NeutralStft.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace spectral_decay
{
class NeutralAudioProcessor final : public juce::AudioProcessor
{
public:
    NeutralAudioProcessor();
    void prepareToPlay(double sampleRate, int maximumBlockSize) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    const juce::String getName() const override { return "Spectral Decay Neutral"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Neutral"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destination) override;
    void setStateInformation(const void*, int) override {}

private:
    NeutralStft engine;
    double preparedSampleRate = 0.0;
    int preparedChannels = 0;
};
}
