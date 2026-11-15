// SPDX-License-Identifier: AGPL-3.0-or-later
#include "plugin/PluginProcessor.h"
#include "ProcessorChecks.h"
#include <memory>
#include <cmath>
namespace
{
float actualValue(juce::RangedAudioParameter& parameter)
{
    return parameter.convertFrom0to1(parameter.getValue());
}
void testEditorInteractions(spectral_decay::PeakBandAudioProcessor& processor)
{
    auto* qParam = processor.getParameters().getParameter("bandQ");
    auto* decayParam = processor.getParameters().getParameter("bandDecay");
    setText(*qParam, "4.0");
    setText(*decayParam, "1.0");
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    if (!editor)
        throw std::runtime_error("Custom editor could not be opened");
    auto* curve = editor->getChildComponent(0);
    if (curve == nullptr)
        throw std::runtime_error("Curve component missing from custom editor");
    const auto now = juce::Time::getCurrentTime();
    const auto source = juce::Desktop::getInstance().getMainMouseSource();
    auto eventAt = [&](juce::Point<float> position, int clicks = 1, bool dragged = false)
    {
        return juce::MouseEvent(source, position, juce::ModifierKeys::noModifiers, 1.0f,
                                0.0f, 0.0f, 0.0f, 0.0f, curve, curve, now,
                                position, now, clicks, dragged);
    };
    auto wheelEvent = eventAt({100.0f, 100.0f});
    const float qBefore = actualValue(*qParam);
    const juce::MouseWheelDetails positiveWheel{0.0f, 1.0f, false, true, false};
    curve->mouseWheelMove(wheelEvent, positiveWheel);
    const float qAfterFirst = actualValue(*qParam);
    curve->mouseWheelMove(wheelEvent, positiveWheel);
    const float qAfterSecond = actualValue(*qParam);
    if (!(qAfterFirst > qBefore && qAfterSecond > qAfterFirst))
        throw std::runtime_error("Positive mouse wheel did not increment the live bandQ value");
    const juce::MouseWheelDetails horizontalWheel{1.0f, 0.0f, false, true, false};
    const float beforeZeroWheel = actualValue(*qParam);
    curve->mouseWheelMove(wheelEvent, horizontalWheel);
    if (actualValue(*qParam) != beforeZeroWheel)
        throw std::runtime_error("Zero vertical wheel delta changed bandQ");

    const float width = static_cast<float>(curve->getWidth());
    const float height = static_cast<float>(curve->getHeight());
    const float nodeX = 48.0f + (width - 60.0f) * std::log(1000.0f / 40.0f) / std::log(16000.0f / 40.0f);
    const float nodeY = 14.0f + (height - 37.0f) * 0.5f;
    const auto down = eventAt({nodeX, nodeY});
    curve->mouseDown(down);
    const auto drag = eventAt({nodeX + 18.0f, nodeY - 24.0f}, 1, true);
    curve->mouseDrag(drag);
    curve->mouseUp(drag);
    if (std::abs(actualValue(*processor.getParameters().getParameter("bandDecay")) - 1.0f) < 1.0e-4f)
        throw std::runtime_error("Dragging the curve node did not change bandDecay");
    const auto doubleClick = eventAt({nodeX, nodeY}, 2);
    curve->mouseDoubleClick(doubleClick);
    if (std::abs(actualValue(*processor.getParameters().getParameter("bandDecay"))) > 1.0e-6f)
        throw std::runtime_error("Double-clicking the curve did not reset bandDecay");
    std::cout << "editor_wheel_drag_double_click=pass; bandQ=" << qAfterSecond << "\n";
}
}
int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI init; spectral_decay::PeakBandAudioProcessor processor;
        if(!processor.hasEditor())throw std::runtime_error("Missing custom editor");
        auto& f=findParameter(processor,"Frequency");auto& q=findParameter(processor,"Width (Q)");auto& d=findParameter(processor,"Decay");auto& m=findParameter(processor,"Tone / Noise");
        const float before[4]{f.getValue(),q.getValue(),d.getValue(),m.getValue()};
        for(int i=0;i<4;++i){std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());if(!editor)throw std::runtime_error("Editor construction failed");}
        if(f.getValue()!=before[0]||q.getValue()!=before[1]||d.getValue()!=before[2]||m.getValue()!=before[3])throw std::runtime_error("Opening editor changed parameter state");
        testEditorInteractions(processor);
        checkProcessor(processor);std::cout<<"Processor and repeated editor construction passed.\n";return 0;
    }
    catch(const std::exception& e){std::cerr<<"Processor test failed: "<<e.what()<<'\n';return 1;}
}
