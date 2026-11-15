// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace spectral_decay
{
class PeakBandAudioEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PeakBandAudioEditor(PeakBandAudioProcessor&);
    ~PeakBandAudioEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    class Curve final : public juce::Component
    {
    public:
        explicit Curve(PeakBandAudioEditor& owner) : parent(owner) {}
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override;
        void mouseUp(const juce::MouseEvent&) override;
        void mouseDoubleClick(const juce::MouseEvent&) override;
        void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void setValues(float f, float q, float d) { frequency=f; quality=q; decay=d; repaint(); }
    private:
        float valueAtX(float x) const;
        float valueAtY(float y) const;
        void updatePoint(const juce::Point<float>&);
        PeakBandAudioEditor& parent;
        float frequency=1000.0f, quality=1.0f, decay=0.0f;
        bool dragging=false;
    };
    void timerCallback() override;
    void changeCurveValues(float frequency, float decay);
    void changeQ(float q);
    PeakBandAudioProcessor& processor;
    Curve curve;
    juce::Slider frequencySlider, qSlider, decaySlider, toneNoiseSlider;
    juce::Label frequencyLabel, qLabel, decayLabel, toneNoiseLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> frequencyAttachment, qAttachment, decayAttachment, toneNoiseAttachment;
};
}
