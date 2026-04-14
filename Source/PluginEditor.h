#pragma once

#include "IRSlotButton.h"
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
    std::vector<ControlDef> controls { {
        { &globalMixControl,       ControlGroup::GLOBAL },
        { &decayControl,           ControlGroup::GLOBAL },
        { &lowCutControl,          ControlGroup::GLOBAL },
        { &highCutControl,         ControlGroup::GLOBAL },
        { &weightingModeControl,   ControlGroup::INTERACTION },
        { &strengthControl,        ControlGroup::INTERACTION },
        { &spreadControl,          ControlGroup::INTERACTION },
        { &positionPatternControl, ControlGroup::POSITION },
        { &positionRateControl,    ControlGroup::POSITION },
        { &positionModAControl,    ControlGroup::POSITION },
        { &positionModBControl,    ControlGroup::POSITION },
        { &fieldPatternControl,    ControlGroup::FIELD },
        { &fieldRateControl,       ControlGroup::FIELD },
        { &fieldModAControl,       ControlGroup::FIELD },
        { &fieldModBControl,       ControlGroup::FIELD },
    } };

    // Sliders
    struct Rotary : juce::Slider {
        Rotary() : juce::Slider (
            juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
            juce::Slider::TextEntryBoxPosition::NoTextBox
        ) {}
    };

    Rotary 
        globalMixControl, decayControl, lowCutControl, highCutControl,
        strengthControl, spreadControl,
        positionRateControl, positionModAControl, positionModBControl,
        fieldRateControl, fieldModAControl, fieldModBControl;

    juce::AudioProcessorValueTreeState::SliderAttachment 
        globalMixControlAttachment, decayControlAttachment, lowCutControlAttachment, highCutControlAttachment,
        strengthControlAttachment, spreadControlAttachment,
        positionRateControlAttachment, positionModAControlAttachment, positionModBControlAttachment,
        fieldRateControlAttachment, fieldModAControlAttachment, fieldModBControlAttachment;

    // Buttons
    juce::TextButton weightingModeControl;
    juce::AudioProcessorValueTreeState::ButtonAttachment weightingModeControlAttachment;

    juce::TextButton positionTabButton, fieldTabButton;

    juce::TextButton clearAllButton, randomAllButton;

    std::array<std::unique_ptr<IRSlotButton>, MAX_IR_COUNT> irSlotButtons;

    // ComboBoxes
    juce::ComboBox positionPatternControl, fieldPatternControl;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment positionPatternControlAttachment, fieldPatternControlAttachment;

    // Components
    PolarMapComponent polarMapComponent;

    void initComponents();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MareverbAudioProcessorEditor)

public:
    MareverbAudioProcessorEditor(MareverbAudioProcessor&);
    ~MareverbAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    std::vector<ControlDef> getControls();
    std::vector<ControlDef> getControlsGroup(ControlGroup group);
};