// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginEditor.h"
#include "dsp/PeakBandStft.h"
#include <cmath>

namespace spectral_decay
{
namespace
{
constexpr float leftPad=48.0f, rightPad=12.0f, topPad=14.0f, bottomPad=23.0f;
const juce::Colour background{0xff171a20}, grid{0xff39414d}, accent{0xff50c8b0}, text{0xffdce4ec};
}
PeakBandAudioEditor::PeakBandAudioEditor(PeakBandAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p), curve(*this)
{
    setSize(580,380);
    addAndMakeVisible(curve);
    auto setupSlider=[this](juce::Slider& slider, juce::Label& label, const juce::String& name)
    {
        addAndMakeVisible(slider); addAndMakeVisible(label); label.setText(name,juce::dontSendNotification); label.setJustificationType(juce::Justification::centred);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,78,18);
    };
    setupSlider(frequencySlider,frequencyLabel,"Frequency"); setupSlider(qSlider,qLabel,"Width (Q)"); setupSlider(decaySlider,decayLabel,"Decay"); setupSlider(toneNoiseSlider,toneNoiseLabel,"Tone / Noise");
    auto& state=processor.getParameters();
    frequencyAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,"bandFreqHz",frequencySlider);
    qAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,"bandQ",qSlider);
    decayAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,"bandDecay",decaySlider);
    toneNoiseAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,"toneNoise",toneNoiseSlider);
    startTimerHz(30);
}
void PeakBandAudioEditor::paint(juce::Graphics& g)
{
    g.fillAll(background); g.setColour(text); g.setFont(18.0f); g.drawText("Spectral Decay — Prototype 2",12,4,getWidth()-24,28,juce::Justification::centredLeft);
}
void PeakBandAudioEditor::resized()
{
    curve.setBounds(12,34,getWidth()-24,220);
    const int width=(getWidth()-24)/4; const int y=270;
    juce::Component* sliders[]={&frequencySlider,&qSlider,&decaySlider,&toneNoiseSlider};
    juce::Component* labels[]={&frequencyLabel,&qLabel,&decayLabel,&toneNoiseLabel};
    for(int i=0;i<4;++i){labels[i]->setBounds(12+i*width,y,width,20);sliders[i]->setBounds(12+i*width,y+18,width,88);}
}
void PeakBandAudioEditor::timerCallback()
{
    auto* f=processor.getParameters().getRawParameterValue("bandFreqHz"); auto* q=processor.getParameters().getRawParameterValue("bandQ"); auto* d=processor.getParameters().getRawParameterValue("bandDecay");
    curve.setValues(f->load(std::memory_order_relaxed),q->load(std::memory_order_relaxed),d->load(std::memory_order_relaxed));
}
void PeakBandAudioEditor::changeCurveValues(float f,float d)
{
    auto* fp=processor.getParameters().getParameter("bandFreqHz"); auto* dp=processor.getParameters().getParameter("bandDecay");
    fp->setValueNotifyingHost(fp->convertTo0to1(f)); dp->setValueNotifyingHost(dp->convertTo0to1(d));
}
void PeakBandAudioEditor::changeQ(float q)
{
    auto* p=processor.getParameters().getParameter("bandQ");p->setValueNotifyingHost(p->convertTo0to1(q));
}
float PeakBandAudioEditor::Curve::valueAtX(float x) const
{
    const float w=std::max(1.0f,getWidth()-leftPad-rightPad); const float t=juce::jlimit(0.0f,1.0f,(x-leftPad)/w);
    return 40.0f*std::pow(16000.0f/40.0f,t);
}
float PeakBandAudioEditor::Curve::valueAtY(float y) const
{
    const float h=std::max(1.0f,getHeight()-topPad-bottomPad); return juce::jlimit(-5.0f,5.0f,5.0f*(1.0f-2.0f*(y-topPad)/h));
}
void PeakBandAudioEditor::Curve::paint(juce::Graphics& g)
{
    auto r=getLocalBounds().toFloat(); g.setColour(juce::Colour(0xff202630)); g.fillRoundedRectangle(r,6.0f);
    const float w=getWidth()-leftPad-rightPad,h=getHeight()-topPad-bottomPad,zero=topPad+h*0.5f;
    const float hzMarks[]={50,100,200,500,1000,2000,5000,10000};
    g.setFont(10.0f); g.setColour(grid);
    for(float hz:hzMarks){float x=leftPad+w*std::log(hz/40.0f)/std::log(16000.0f/40.0f);g.drawVerticalLine(static_cast<int>(x),topPad,topPad+h);g.setColour(text);g.drawText(hz>=1000?juce::String(hz/1000,1)+"k":juce::String(static_cast<int>(hz)),static_cast<int>(x)-16,getHeight()-18,32,14,juce::Justification::centred);g.setColour(grid);}
    for(int i=0;i<=10;++i){float y=topPad+h*i/10.0f;g.drawHorizontalLine(static_cast<int>(y),leftPad,leftPad+w);}
    g.setColour(juce::Colour(0xff8793a1));g.drawHorizontalLine(static_cast<int>(zero),leftPad,leftPad+w);
    juce::Path path,fill; bool started=false;
    for(int i=0;i<=160;++i){float x=leftPad+w*i/160.0f;float hz=valueAtX(x);float val=decay*PeakBandStft::computeBandWeight(hz,frequency,quality);float y=topPad+h*(5.0f-val)/10.0f;if(!started){path.startNewSubPath(x,y);fill.startNewSubPath(x,zero);fill.lineTo(x,y);started=true;}else{path.lineTo(x,y);fill.lineTo(x,y);}}
    fill.lineTo(leftPad+w,zero);fill.closeSubPath();g.setColour(accent.withAlpha(0.2f));g.fillPath(fill);g.setColour(accent);g.strokePath(path,juce::PathStrokeType(2.0f));
    float px=leftPad+w*std::log(frequency/40.0f)/std::log(16000.0f/40.0f),py=topPad+h*(5.0f-decay)/10.0f;g.setColour(juce::Colours::white);g.fillEllipse(px-5,py-5,10,10);
}
void PeakBandAudioEditor::Curve::mouseDown(const juce::MouseEvent& e)
{
    float px=leftPad+(getWidth()-leftPad-rightPad)*std::log(frequency/40.0f)/std::log(16000.0f/40.0f);float py=topPad+(getHeight()-topPad-bottomPad)*(5.0f-decay)/10.0f;
    if(e.position.getDistanceFrom({px,py})<18.0f){dragging=true;parent.processor.getParameters().getParameter("bandFreqHz")->beginChangeGesture();parent.processor.getParameters().getParameter("bandDecay")->beginChangeGesture();}
}
void PeakBandAudioEditor::Curve::updatePoint(const juce::Point<float>& p){frequency=valueAtX(p.x);decay=valueAtY(p.y);parent.changeCurveValues(frequency,decay);repaint();}
void PeakBandAudioEditor::Curve::mouseDrag(const juce::MouseEvent& e){if(dragging)updatePoint(e.position);}
void PeakBandAudioEditor::Curve::mouseUp(const juce::MouseEvent&){if(dragging){parent.processor.getParameters().getParameter("bandFreqHz")->endChangeGesture();parent.processor.getParameters().getParameter("bandDecay")->endChangeGesture();dragging=false;}}
void PeakBandAudioEditor::Curve::mouseDoubleClick(const juce::MouseEvent&){auto* p=parent.processor.getParameters().getParameter("bandDecay");p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(0.0f));p->endChangeGesture();}
void PeakBandAudioEditor::Curve::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (std::abs(wheel.deltaY) <= 1.0e-6f)
        return;

    auto* qParam = parent.processor.getParameters().getParameter("bandQ");
    const float currentQ = qParam->convertFrom0to1(qParam->getValue());
    const float newQ = juce::jlimit(0.25f, 12.0f,
                                    currentQ * std::pow(1.08f, wheel.deltaY));
    qParam->beginChangeGesture();
    qParam->setValueNotifyingHost(qParam->convertTo0to1(newQ));
    qParam->endChangeGesture();
    quality = newQ;
}
}
