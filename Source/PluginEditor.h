#pragma once

#include "PluginProcessor.h"
#include "PolarMapComponent.h"

#include <JuceHeader.h>

class MareverbAudioProcessorEditor  : public juce::AudioProcessorEditor,
    juce::AudioProcessorValueTreeState::Listener, juce::Timer {
private:
    MareverbAudioProcessor& audioProcessor;

    // Listeners and callbacks
    std::atomic<bool> positionPathChanged { false };

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    // Parameters
    const std::vector<juce::String> paramIDs = {
        ParamID::globalMix, ParamID::decay, ParamID::lowCut, ParamID::highCut,
        ParamID::weightingMode, ParamID::strength, ParamID::spread,
        ParamID::positionPattern, ParamID::positionRate, ParamID::positionModA, ParamID::positionModB,
        ParamID::fieldPattern, ParamID::fieldRate, ParamID::fieldModA, ParamID::fieldModB
    };

    // Controls
    struct Rotary : juce::Slider {
        Rotary() : juce::Slider (
            juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
            juce::Slider::TextEntryBoxPosition::TextBoxBelow
        ) {}
    };

    Rotary globalMixControl, decayControl, lowCutControl, highCutControl,
        strengthControl, spreadControl,
        positionRateControl, positionModAControl, positionModBControl,
        fieldRateControl, fieldModAControl, fieldModBControl;

    juce::AudioProcessorValueTreeState::SliderAttachment 
        globalMixControlAttachment, decayControlAttachment, lowCutControlAttachment, highCutControlAttachment,
        strengthControlAttachment, spreadControlAttachment,
        positionRateControlAttachment, positionModAControlAttachment, positionModBControlAttachment,
        fieldRateControlAttachment, fieldModAControlAttachment, fieldModBControlAttachment;

    juce::ToggleButton weightingModeControl; // TODO: Change this? Need a two-choice toggle control, not a tickbox
    juce::AudioProcessorValueTreeState::ButtonAttachment weightingModeControlAttachment;

    juce::ComboBox positionPatternControl, fieldPatternControl;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment positionPatternControlAttachment, fieldPatternControlAttachment;

    // TODO: Add control for fieldSelect
    // Need a custom component (perhaps based off juce::Label)
    // Text box that you can drag up and down to change the value (not Slider)

    std::vector<juce::Component*> getControls();
    
    // Components
    PolarMapComponent polarMapComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MareverbAudioProcessorEditor)

public:
    MareverbAudioProcessorEditor(MareverbAudioProcessor&);
    ~MareverbAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
};
