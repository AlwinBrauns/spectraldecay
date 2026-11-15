// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

inline juce::AudioProcessorParameter& findParameter(juce::AudioProcessor& p,const juce::String& name)
{
    for(auto* item:p.getParameters()) if(item->getName(64)==name) return *item;
    throw std::runtime_error("Missing parameter: "+name.toStdString());
}
inline void setText(juce::AudioProcessorParameter& p,const juce::String& text){p.setValueNotifyingHost(p.getValueForText(text));}
inline juce::MemoryBlock wrapVst3ComponentState(const void* data,int size)
{
    juce::XmlElement root("VST3PluginState");auto* component=root.createNewChildElement("IComponent");juce::MemoryBlock bytes(data,static_cast<std::size_t>(std::max(0,size)));component->addTextElement(bytes.toBase64Encoding());juce::MemoryBlock wrapped;juce::AudioProcessor::copyXmlToBinary(root,wrapped);return wrapped;
}
inline void checkState(juce::AudioProcessor& processor,bool vst3Wrapped=false)
{
    auto& frequency=findParameter(processor,"Frequency");auto& q=findParameter(processor,"Width (Q)");auto& decay=findParameter(processor,"Decay");auto& morph=findParameter(processor,"Tone / Noise");
    const std::array<juce::AudioProcessorParameter*,4> params{&frequency,&q,&decay,&morph};
    const std::array<float,4> defaults{frequency.getDefaultValue(),q.getDefaultValue(),decay.getDefaultValue(),morph.getDefaultValue()};
    setText(frequency,"3210");setText(q,"3.2");setText(decay,"-2.25");setText(morph,"0.73");
    std::array<float,4> saved{};for(std::size_t i=0;i<params.size();++i)saved[i]=params[i]->getValue();
    juce::MemoryBlock state;processor.getStateInformation(state);if(state.getSize()==0)throw std::runtime_error("Serialized state is empty");
    for(std::size_t i=0;i<params.size();++i)params[i]->setValueNotifyingHost(defaults[i]);
    processor.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    for(std::size_t i=0;i<params.size();++i)if(std::abs(params[i]->getValue()-saved[i])>1.0e-5f)throw std::runtime_error("Parameter state did not restore at index "+std::to_string(i)+" value="+std::to_string(params[i]->getValue())+" expected="+std::to_string(saved[i]));

    juce::ValueTree old("SpectralDecayState");
    for(const auto& item:{std::pair<const char*,float>{"tailAmount",1.0f},{"tailMode",1.0f}}){juce::ValueTree child("PARAM");child.setProperty("id",item.first,nullptr);child.setProperty("value",item.second,nullptr);old.appendChild(child,nullptr);}
    juce::MemoryBlock oldBlob;juce::AudioProcessor::copyXmlToBinary(*old.createXml(),oldBlob);auto oldHostBlob=vst3Wrapped?wrapVst3ComponentState(oldBlob.getData(),static_cast<int>(oldBlob.getSize())):oldBlob;processor.setStateInformation(oldHostBlob.getData(),static_cast<int>(oldHostBlob.getSize()));
    auto verifyDefaults=[&](const char* stage){for(std::size_t i=0;i<params.size();++i)if(std::abs(params[i]->getValue()-defaults[i])>1.0e-6f)throw std::runtime_error(std::string("Invalid state fallback failed at ")+stage+" parameter "+std::to_string(i)+" value="+std::to_string(params[i]->getValue())+" expected="+std::to_string(defaults[i]));};verifyDefaults("old P1");
    if(vst3Wrapped){auto empty=wrapVst3ComponentState(nullptr,0);processor.setStateInformation(empty.getData(),static_cast<int>(empty.getSize()));}else processor.setStateInformation(nullptr,0);verifyDefaults("empty");const unsigned char corrupt[]={0xff,0x01,0x7f};if(vst3Wrapped){auto bad=wrapVst3ComponentState(corrupt,3);processor.setStateInformation(bad.getData(),static_cast<int>(bad.getSize()));}else processor.setStateInformation(corrupt,3);verifyDefaults("corrupt");
    std::cout<<"state_roundtrip=pass; old_empty_corrupt_fallback=pass\n";
}
inline void checkProcessor(juce::AudioProcessor& processor,bool vst3Wrapped=false)
{
    checkState(processor,vst3Wrapped);
    for(double fs:{44100.0,48000.0})for(int block:{1,64,257,512})for(int channels:{1,2})
    {
        auto layout=processor.getBusesLayout();auto set=channels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo();layout.inputBuses.set(0,set);layout.outputBuses.set(0,set);if(!processor.setBusesLayout(layout))throw std::runtime_error("Mono/stereo layout rejected");
        processor.setRateAndBufferSizeDetails(fs,block);processor.prepareToPlay(fs,block);if(processor.getTailLengthSeconds()<2.0)throw std::runtime_error("Reported tail does not cover the extension limit");if(processor.getLatencySamples()!=2048)throw std::runtime_error("Reported latency is not 2048 samples");processor.reset();
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> zeroBuffer(channels, 0);
        processor.processBlock(zeroBuffer, midi);
        std::cout << "zero_block=pass";
        if (!vst3Wrapped)
        {
            const int oversizedSamples = block + std::max(1, block / 2);
            juce::AudioBuffer<float> oversized(channels, oversizedSamples);
            oversized.clear();
            for (int c = 0; c < channels; ++c)
                for (int i = 0; i < oversizedSamples; ++i)
                    oversized.setSample(c, i, 0.1f);
            processor.processBlock(oversized, midi);
            for (int c = 0; c < channels; ++c)
                for (int i = 0; i < oversizedSamples; ++i)
                    if (!std::isfinite(oversized.getSample(c, i)))
                        throw std::runtime_error("Oversized block produced nonfinite output");
            std::cout << "; oversized_block=" << oversizedSamples << ", prepared_max=" << block;
        }
        std::cout << "\n";
        const int length=2048+8192+67;juce::AudioBuffer<float> buffer(channels,block);std::vector<std::vector<float>> output(channels,std::vector<float>(length));int peakIndex=-1;float peak=0;
        for(int pos=0;pos<length;){const int n=std::min(block,length-pos);buffer.setSize(channels,n,false,false,true);buffer.clear();if(pos<=67&&67<pos+n)for(int c=0;c<channels;++c)buffer.setSample(c,67-pos,c==0?0.25f:-0.125f);processor.processBlock(buffer,midi);for(int c=0;c<channels;++c)for(int i=0;i<n;++i){float x=buffer.getSample(c,i);if(!std::isfinite(x))throw std::runtime_error("Nonfinite processor output");output[c][pos+i]=x;if(c==0&&std::abs(x)>peak){peak=std::abs(x);peakIndex=pos+i;}}pos+=n;}
        if(peakIndex-67!=2048)throw std::runtime_error("Measured impulse latency differs from reported latency");
        std::cout<<"fs="<<fs<<",block="<<block<<",channels="<<channels<<",latency="<<processor.getLatencySamples()<<",measured="<<peakIndex-67<<"\n";processor.releaseResources();
    }
}
