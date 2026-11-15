// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"

namespace spectral_decay
{
NeutralAudioProcessor::NeutralAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void NeutralAudioProcessor::prepareToPlay(double sampleRate, int)
{
    const auto channels = getTotalNumInputChannels();
    engine.prepare(sampleRate, static_cast<std::size_t>(channels));
    preparedChannels = channels;
    preparedSampleRate = sampleRate;
    setLatencySamples(engine.latencySamples());
}

void NeutralAudioProcessor::releaseResources()
{
    engine.reset();
}

void NeutralAudioProcessor::reset()
{
    engine.reset();
}

bool NeutralAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.inputBuses.size() != 1 || layouts.outputBuses.size() != 1)
        return false;
    const auto input = layouts.getMainInputChannelSet();
    return input == layouts.getMainOutputChannelSet()
           && (input == juce::AudioChannelSet::mono()
               || input == juce::AudioChannelSet::stereo());
}

void NeutralAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    jassert(buffer.getNumChannels() == preparedChannels);
    midi.clear();
    engine.process(buffer.getArrayOfWritePointers(),
                   static_cast<std::size_t>(buffer.getNumSamples()));
}

double NeutralAudioProcessor::getTailLengthSeconds() const
{
    // Include a reconstruction window for possible floating-point residuals.
    return preparedSampleRate > 0.0
               ? (engine.latencySamples() + NeutralStft::fftSize) / preparedSampleRate
               : 0.0;
}

void NeutralAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    destination.reset();
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new spectral_decay::NeutralAudioProcessor();
}
