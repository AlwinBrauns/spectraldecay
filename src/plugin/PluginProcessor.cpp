// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"

namespace spectral_decay
{
NeutralAudioProcessor::NeutralAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "SpectralDecayState", createParameterLayout()),
      tailAmount(parameters.getRawParameterValue("tailAmount")),
      tailMode(parameters.getRawParameterValue("tailMode")),
      tailLowHz(parameters.getRawParameterValue("tailLowHz")),
      tailHighHz(parameters.getRawParameterValue("tailHighHz"))
{
    jassert(tailAmount != nullptr && tailMode != nullptr
            && tailLowHz != nullptr && tailHighHz != nullptr);
}

juce::AudioProcessorValueTreeState::ParameterLayout NeutralAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"tailAmount", 1}, "Tail amount",
        juce::NormalisableRange<float>{0.0f, 1.0f}, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"tailMode", 1}, "Tail mode",
        juce::StringArray{"Tonal", "Noise"}, 0));
    juce::NormalisableRange<float> frequencyRange{40.0f, 16000.0f, 1.0f};
    frequencyRange.setSkewForCentre(1000.0f);
    const auto attributes = juce::AudioParameterFloatAttributes().withLabel("Hz");
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"tailLowHz", 1}, "Low frequency",
        frequencyRange, 500.0f, attributes));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"tailHighHz", 1}, "High frequency",
        frequencyRange, 1500.0f, attributes));
    return layout;
}

juce::AudioProcessorEditor* NeutralAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
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
    engine.setTail(tailAmount->load(std::memory_order_relaxed),
                   tailMode->load(std::memory_order_relaxed) < 0.5f
                       ? NeutralStft::TailMode::tonal
                       : NeutralStft::TailMode::noise,
                   tailLowHz->load(std::memory_order_relaxed),
                   tailHighHz->load(std::memory_order_relaxed));
    engine.process(buffer.getArrayOfWritePointers(),
                   static_cast<std::size_t>(buffer.getNumSamples()));
}

double NeutralAudioProcessor::getTailLengthSeconds() const
{
    // Include the experimental tail and the reconstruction window.
    const auto rate = preparedSampleRate > 0.0 ? preparedSampleRate : getSampleRate();
    return 2.0 + (rate > 0.0
                      ? (engine.latencySamples() + NeutralStft::fftSize) / rate
                      : 1.0);
}

void NeutralAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    if (const auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destination);
    else
    {
        destination.reset();
        juce::Logger::writeToLog("Spectral Decay: could not serialize plugin state");
    }
}

void NeutralAudioProcessor::setStateInformation(const void* data, int size)
{
    // Earlier neutral builds did not save state.
    if (size == 0)
        return;
    if (data != nullptr && size > 0)
    {
        if (const auto xml = getXmlFromBinary(data, size))
        {
            if (xml->hasTagName(parameters.state.getType()))
            {
                auto state = juce::ValueTree::fromXml(*xml);
                for (const auto* id : {"tailLowHz", "tailHighHz"})
                {
                    if (!state.getChildWithProperty("id", id).isValid())
                    {
                        auto* parameter = parameters.getParameter(id);
                        juce::ValueTree child("PARAM");
                        child.setProperty("id", id, nullptr);
                        child.setProperty("value",
                                          parameter->convertFrom0to1(parameter->getDefaultValue()),
                                          nullptr);
                        state.appendChild(child, nullptr);
                    }
                }
                parameters.replaceState(state);
                return;
            }
        }
    }
    juce::Logger::writeToLog("Spectral Decay: invalid plugin state");
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new spectral_decay::NeutralAudioProcessor();
}
