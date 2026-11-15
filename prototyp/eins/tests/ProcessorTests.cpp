// SPDX-License-Identifier: AGPL-3.0-or-later
#include "plugin/PluginProcessor.h"
#include "ProcessorChecks.h"

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI initialiseJuce;
        spectral_decay::NeutralAudioProcessor processor;
        if (!processor.hasEditor())
            throw std::runtime_error("The prototype must expose an editor");
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
            if (editor == nullptr)
                throw std::runtime_error("The prototype editor could not be created");
        }
        {
            auto& amount = findParameter(processor, "Tail amount");
            auto& mode = findParameter(processor, "Tail mode");
            auto& low = findParameter(processor, "Low frequency");
            auto& high = findParameter(processor, "High frequency");
            low.setValueNotifyingHost(low.getValueForText("3000"));
            high.setValueNotifyingHost(high.getValueForText("6000"));
            juce::ValueTree oldState("SpectralDecayState");
            for (const auto& entry : {std::pair<const char*, float>{"tailAmount", 0.75f},
                                      {"tailMode", 1.0f}})
            {
                juce::ValueTree parameter("PARAM");
                parameter.setProperty("id", entry.first, nullptr);
                parameter.setProperty("value", entry.second, nullptr);
                oldState.appendChild(parameter, nullptr);
            }
            juce::MemoryBlock state;
            juce::AudioProcessor::copyXmlToBinary(*oldState.createXml(), state);
            processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            if (std::abs(amount.getValue() - 0.75f) > 0.001f || mode.getValue() < 0.99f
                || std::abs(low.getValue() - low.getValueForText("500")) > 0.001f
                || std::abs(high.getValue() - high.getValueForText("1500")) > 0.001f)
                throw std::runtime_error("Old state did not restore values and frequency defaults");
            amount.setValueNotifyingHost(0.0f);
            mode.setValueNotifyingHost(0.0f);
        }
        checkProcessor(processor);
        auto unsupported = processor.getBusesLayout();
        unsupported.inputBuses.set(0, juce::AudioChannelSet::mono());
        unsupported.outputBuses.set(0, juce::AudioChannelSet::stereo());
        if (processor.checkBusesLayoutSupported(unsupported))
            throw std::runtime_error("Mismatched bus layout must be rejected");
        std::cout << "Processor routing, finite output and exact latency checks passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Processor test failed: " << error.what() << '\n';
        return 1;
    }
}
