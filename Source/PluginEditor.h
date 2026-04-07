#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MareverbAudioProcessorEditor  : public juce::AudioProcessorEditor,
    juce::AudioProcessorParameter::Listener, juce::Timer {
public:

    MareverbAudioProcessorEditor (MareverbAudioProcessor&);
    ~MareverbAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MareverbAudioProcessor& audioProcessor;

    // Listeners and callbacks
    juce::Atomic<bool> parametersChanged{ false };

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
    void timerCallback() override;

    // Components
    struct Rotary : juce::Slider {
        Rotary() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
            juce::Slider::TextEntryBoxPosition::TextBoxBelow) {}
    };

    Rotary globalMixControl, decayControl,
        motionRateControl, motionModAControl, motionModBControl;
    Attachment globalMixControlAttachment, decayControlAttachment, 
        motionRateControlAttachment, motionModAControlAttachment, motionModBControlAttachment;

    std::vector<juce::Component*> getComponents();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MareverbAudioProcessorEditor)
};
