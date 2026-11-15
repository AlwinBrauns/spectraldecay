// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace spectral_decay
{
namespace
{
constexpr const char* stateType = "SpectralDecayState";
constexpr int stateVersion = 2;
}

PeakBandAudioProcessor::PeakBandAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, stateType, createParameterLayout())
{
    bandFrequency = parameters.getRawParameterValue("bandFreqHz");
    bandQ = parameters.getRawParameterValue("bandQ");
    bandDecay = parameters.getRawParameterValue("bandDecay");
    toneNoise = parameters.getRawParameterValue("toneNoise");
    jassert(bandFrequency && bandQ && bandDecay && toneNoise);
}

juce::AudioProcessorValueTreeState::ParameterLayout PeakBandAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    juce::NormalisableRange<float> frequency{40.0f, 16000.0f, 0.0f, 0.25f};
    frequency.setSkewForCentre(1000.0f);
    juce::NormalisableRange<float> q{0.25f, 12.0f, 0.0f, 0.55f};
    q.setSkewForCentre(1.5f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bandFreqHz", 1}, "Frequency", frequency, 1000.0f, juce::AudioParameterFloatAttributes().withLabel("Hz")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bandQ", 1}, "Width (Q)", q, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bandDecay", 1}, "Decay", juce::NormalisableRange<float>{-5.0f, 5.0f}, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"toneNoise", 1}, "Tone / Noise", juce::NormalisableRange<float>{0.0f, 1.0f}, 0.5f));
    return layout;
}

void PeakBandAudioProcessor::prepareToPlay(double rate, int)
{
    sampleRate = rate;
    preparedChannels = getTotalNumInputChannels();
    engine.prepare(rate, static_cast<std::size_t>(preparedChannels));
    setLatencySamples(engine.latencySamples());
}
void PeakBandAudioProcessor::releaseResources() { engine.reset(); }
void PeakBandAudioProcessor::reset() { engine.reset(); }
bool PeakBandAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.inputBuses.size() != 1 || layouts.outputBuses.size() != 1) return false;
    const auto input = layouts.getMainInputChannelSet();
    return input == layouts.getMainOutputChannelSet() && (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo());
}
void PeakBandAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    jassert(buffer.getNumChannels() == preparedChannels);
    if (buffer.getNumSamples() == 0)
        return;
    engine.setBandFrequencyHz(bandFrequency->load(std::memory_order_relaxed));
    engine.setBandQ(bandQ->load(std::memory_order_relaxed));
    engine.setBandDecay(bandDecay->load(std::memory_order_relaxed));
    engine.setToneNoise(toneNoise->load(std::memory_order_relaxed));
    engine.process(buffer.getArrayOfWritePointers(), static_cast<std::size_t>(buffer.getNumSamples()));
}
double PeakBandAudioProcessor::getTailLengthSeconds() const
{
    return PeakBandStft::maximumTailSeconds + (sampleRate > 0.0 ? PeakBandStft::fftSize / sampleRate : 1.0);
}
juce::AudioProcessorEditor* PeakBandAudioProcessor::createEditor() { return new PeakBandAudioEditor(*this); }
void PeakBandAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    destination.reset();
    auto state = parameters.copyState();
    state.setProperty("schemaVersion", stateVersion, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destination);
}
void PeakBandAudioProcessor::restoreDefaults() noexcept
{
    for (const auto* id : {"bandFreqHz", "bandQ", "bandDecay", "toneNoise"})
        if (auto* p = parameters.getParameter(id)) p->setValueNotifyingHost(p->getDefaultValue());
}
void PeakBandAudioProcessor::setStateInformation(const void* data, int size)
{
    if (data == nullptr || size <= 0) { restoreDefaults(); return; }
    auto xml = getXmlFromBinary(data, size);
    if (xml == nullptr || !xml->hasTagName(stateType)) { restoreDefaults(); return; }
    auto state = juce::ValueTree::fromXml(*xml);
    if (!state.isValid() || static_cast<int>(state.getProperty("schemaVersion", 0)) != stateVersion)
    { restoreDefaults(); return; }
    constexpr const char* ids[] = {"bandFreqHz", "bandQ", "bandDecay", "toneNoise"};
    constexpr float minimum[] = {40.0f, 0.25f, -5.0f, 0.0f};
    constexpr float maximum[] = {16000.0f, 12.0f, 5.0f, 1.0f};
    for (std::size_t i = 0; i < 4; ++i)
    {
        const auto child = state.getChildWithProperty("id", ids[i]);
        if (!child.isValid() || !child.hasProperty("value")) { restoreDefaults(); return; }
        const float value = static_cast<float>(child["value"]);
        if (!std::isfinite(value) || value < minimum[i] || value > maximum[i])
        { restoreDefaults(); return; }
    }
    parameters.replaceState(state);
}
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new spectral_decay::PeakBandAudioProcessor(); }
