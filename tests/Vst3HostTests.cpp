// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ProcessorChecks.h"

int main(int argc, char** argv)
{
    try
    {
        if (argc != 2)
            throw std::runtime_error("Usage: spectral_decay_vst3_host_tests <plugin.vst3>");
        juce::ScopedJuceInitialiser_GUI initialiseJuce;
        juce::VST3PluginFormatHeadless format;
        juce::OwnedArray<juce::PluginDescription> descriptions;
        format.findAllTypesForFile(descriptions, juce::File(argv[1]).getFullPathName());
        if (descriptions.size() != 1)
            throw std::runtime_error("Expected exactly one VST3 audio effect in the module");
        juce::String error;
        auto instance = format.createInstanceFromDescription(*descriptions[0], 48000.0, 512, error);
        if (!instance)
            throw std::runtime_error("VST3 instantiation failed: " + error.toStdString());
        checkProcessor(*instance);
        std::cout << "Actual VST3 module loaded; audio and host latency checks passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "VST3 host test failed: " << error.what() << '\n';
        return 1;
    }
}
