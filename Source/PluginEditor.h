#pragma once

#include "PluginProcessor.h"
#include "PolarMapComponent.h"

#include <JuceHeader.h>

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
    juce::Atomic<bool> positionPatternChanged { false };

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    // Components
    struct Rotary : juce::Slider {
        Rotary() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
            juce::Slider::TextEntryBoxPosition::TextBoxBelow) {}
    };

    Rotary globalMixControl, decayControl,
        strengthControl, spreadControl,
        positionRateControl, positionModAControl, positionModBControl,
        fieldRateControl, fieldModAControl, fieldModBControl;
    juce::AudioProcessorValueTreeState::SliderAttachment globalMixControlAttachment, decayControlAttachment, 
        strengthControlAttachment, spreadControlAttachment,
        positionRateControlAttachment, positionModAControlAttachment, positionModBControlAttachment,
        fieldRateControlAttachment, fieldModAControlAttachment, fieldModBControlAttachment;

    juce::ToggleButton weightingModeControl; // TODO: Change this? Need a two-choice toggle control, but not necessarily like a tickbox
    juce::AudioProcessorValueTreeState::ButtonAttachment weightingModeControlAttachment;
    juce::ComboBox positionPatternControl, fieldPatternControl;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment positionPatternControlAttachment, fieldPatternControlAttachment;

    // TODO: Add control for fieldSelect
    // Need a custom component (perhaps based off juce::Label): a text box that you can drag up and down to change the value (not Slider)

    std::vector<juce::Component*> getComponents();
    
    // State
    PolarMapComponent polarMapComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MareverbAudioProcessorEditor)
};
