#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MareverbAudioProcessorEditor  : public juce::AudioProcessorEditor,
    juce::AudioProcessorValueTreeState::Listener, juce::Timer {
public:

    MareverbAudioProcessorEditor (MareverbAudioProcessor&);
    ~MareverbAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MareverbAudioProcessor& audioProcessor;

    // Listeners and callbacks
    juce::Atomic<bool> motionPatternChanged { false };

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    // Components
    struct Rotary : juce::Slider {
        Rotary() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
            juce::Slider::TextEntryBoxPosition::TextBoxBelow) {}
    };

    Rotary globalMixControl, decayControl,
        motionRateControl, motionModAControl, motionModBControl;
    juce::AudioProcessorValueTreeState::SliderAttachment globalMixControlAttachment, decayControlAttachment, 
        motionRateControlAttachment, motionModAControlAttachment, motionModBControlAttachment;

    juce::ComboBox motionPatternControl;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment motionPatternControlAttachment;

    std::vector<juce::Component*> getComponents();
    
    // State
    PolarCoordinate currentPosition {0.0f, 0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MareverbAudioProcessorEditor)
};
