#pragma once

#include "IRHeaderComponent.h"
#include "IRSlotButton.h"
#include "PluginProcessor.h"
#include "PolarMapComponent.h"
#include "Rotary.h"
#include "RotaryLookAndFeel.h"
#include "SettingsComponent.h"
#include "WaveformComponent.h"

#include <JuceHeader.h>

class MareverbAudioProcessorEditor  : public juce::AudioProcessorEditor,
    juce::AudioProcessorValueTreeState::Listener, juce::Timer {
private:
    MareverbAudioProcessor& audioProcessor;

    // Look and feel
    juce::AnimatorUpdater animatorUpdater;

    RotaryLookAndFeel rotaryLookAndFeel;

    // Listeners and callbacks
    std::atomic<bool> positionPathChanged { false };
    std::atomic<bool> selectedIRChanged { false };

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
        { &globalMixControl,       ControlGroup::GLOBAL, [&]() { globalMixControl.setLookAndFeel(&rotaryLookAndFeel); } },
        { &decayControl,           ControlGroup::GLOBAL, [&]() { decayControl.setLookAndFeel(&rotaryLookAndFeel); } },
        { &lowCutControl,          ControlGroup::GLOBAL, [&]() { lowCutControl.setLookAndFeel(&rotaryLookAndFeel); } },
        { &highCutControl,         ControlGroup::GLOBAL, [&]() { highCutControl.setLookAndFeel(&rotaryLookAndFeel); } },
        { &weightingModeControl,   ControlGroup::INTERACTION, []() {} },
        { &strengthControl,        ControlGroup::INTERACTION, [&]() { strengthControl.setLookAndFeel(&rotaryLookAndFeel); } },
        { &spreadControl,          ControlGroup::INTERACTION, [&]() { spreadControl.setLookAndFeel(&rotaryLookAndFeel); } },
        { &positionPatternControl, ControlGroup::POSITION, []() {} },
        { &positionRateControl,    ControlGroup::POSITION, [&]() { positionRateControl.setLookAndFeel(&rotaryLookAndFeel); } },
        { &positionModAControl,    ControlGroup::POSITION, [&]() { positionModAControl.setLookAndFeel(&rotaryLookAndFeel); } },
        { &positionModBControl,    ControlGroup::POSITION, [&]() { positionModBControl.setLookAndFeel(&rotaryLookAndFeel); } },
        { &fieldPatternControl,    ControlGroup::FIELD, []() {} },
        { &fieldRateControl,       ControlGroup::FIELD, [&]() { fieldRateControl.setLookAndFeel(&rotaryLookAndFeel); } },
        { &fieldModAControl,       ControlGroup::FIELD, [&]() { fieldModAControl.setLookAndFeel(&rotaryLookAndFeel); } },
        { &fieldModBControl,       ControlGroup::FIELD, [&]() { fieldModBControl.setLookAndFeel(&rotaryLookAndFeel); } },
    } };

    // Sliders

    Rotary 
        globalMixControl {animatorUpdater}, decayControl {animatorUpdater}, lowCutControl {animatorUpdater}, highCutControl {animatorUpdater},
        strengthControl {animatorUpdater}, spreadControl {animatorUpdater},
        positionRateControl {animatorUpdater, true}, positionModAControl {animatorUpdater}, positionModBControl {animatorUpdater},
        fieldRateControl {animatorUpdater, true}, fieldModAControl {animatorUpdater}, fieldModBControl {animatorUpdater};

    SliderAttachment 
        globalMixControlAttachment, decayControlAttachment, lowCutControlAttachment, highCutControlAttachment,
        strengthControlAttachment, spreadControlAttachment,
        positionRateControlAttachment, positionModAControlAttachment, positionModBControlAttachment,
        fieldRateControlAttachment, fieldModAControlAttachment, fieldModBControlAttachment;

    struct SwapControl {
        Rotary swapMinControl, swapMaxControl;
        SliderAttachment swapMinControlAttachment, swapMaxControlAttachment;

        SwapControl(juce::AudioProcessorValueTreeState& apvts, juce::AnimatorUpdater& updater, int i)
            : swapMinControl(updater), swapMaxControl(updater),
            swapMinControlAttachment(apvts, ParamID::irSwapMin(i), swapMinControl),
            swapMaxControlAttachment(apvts, ParamID::irSwapMax(i), swapMaxControl)
        {}
    };

    std::array<std::unique_ptr<SwapControl>, MAX_IR_COUNT> swapControls;

    // Buttons
    juce::TextButton weightingModeControl;
    juce::AudioProcessorValueTreeState::ButtonAttachment weightingModeControlAttachment;

    juce::TextButton positionTabButton, fieldTabButton;

    juce::TextButton clearAllButton, randomAllButton;

    std::array<std::unique_ptr<IRSlotButton>, MAX_IR_COUNT> irSlotButtons;

    juce::TextButton loadIRButton, randomIRButton;

    juce::TextButton settingsButton;

    // ComboBoxes
    juce::ComboBox positionPatternControl, fieldPatternControl;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment positionPatternControlAttachment, fieldPatternControlAttachment;

    // Components
    PolarMapComponent polarMapComponent;
    IRHeaderComponent irHeaderComponent;
    WaveformComponent irWaveformComponent;
    std::unique_ptr<SettingsComponent> settingsModal;

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