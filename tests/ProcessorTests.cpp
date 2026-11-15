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
