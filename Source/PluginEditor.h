#pragma once

#include "ButtonLookAndFeel.h"
#include "HoverableTextButton.h"
#include "IRHeaderComponent.h"
#include "IRSlotButton.h"
#include "LabelledControl.h"
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

    ButtonLookAndFeel buttonLookAndFeel;
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
        { &globalMixControl,       ControlGroup::GLOBAL, [&]() { globalMixControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { &decayControl,           ControlGroup::GLOBAL, [&]() { decayControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { &lowCutControl,          ControlGroup::GLOBAL, [&]() { lowCutControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { &highCutControl,         ControlGroup::GLOBAL, [&]() { highCutControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { &weightingModeControl,   ControlGroup::INTERACTION, [&]() { weightingModeControl.control.setLookAndFeel(&buttonLookAndFeel); } },
        { &strengthControl,        ControlGroup::INTERACTION, [&]() { strengthControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { &spreadControl,          ControlGroup::INTERACTION, [&]() { spreadControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { &positionPatternControl, ControlGroup::POSITION, []() {} },
        { &positionRateControl,    ControlGroup::POSITION, [&]() { positionRateControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { &positionModAControl,    ControlGroup::POSITION, [&]() { positionModAControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { &positionModBControl,    ControlGroup::POSITION, [&]() { positionModBControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { &fieldPatternControl,    ControlGroup::FIELD, []() {} },
        { &fieldRateControl,       ControlGroup::FIELD, [&]() { fieldRateControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { &fieldModAControl,       ControlGroup::FIELD, [&]() { fieldModAControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { &fieldModBControl,       ControlGroup::FIELD, [&]() { fieldModBControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
    } };

    // Sliders
    inline juce::String getParameterName(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID) {
        if (auto* parameter = apvts.getParameter(paramID)) return parameter->getName(32);
        else return paramID;
    }

    LabelledControl<Rotary>
        globalMixControl { getParameterName(audioProcessor.apvts, ParamID::globalMix), animatorUpdater },
        decayControl { getParameterName(audioProcessor.apvts, ParamID::decay), animatorUpdater },
        lowCutControl { getParameterName(audioProcessor.apvts, ParamID::lowCut), animatorUpdater },
        highCutControl { getParameterName(audioProcessor.apvts, ParamID::highCut), animatorUpdater },
        strengthControl { getParameterName(audioProcessor.apvts, ParamID::strength), animatorUpdater },
        spreadControl { getParameterName(audioProcessor.apvts, ParamID::spread), animatorUpdater },
        positionRateControl { getParameterName(audioProcessor.apvts, ParamID::positionRate), animatorUpdater, true },
        positionModAControl { getParameterName(audioProcessor.apvts, ParamID::positionModA), animatorUpdater },
        positionModBControl { getParameterName(audioProcessor.apvts, ParamID::positionModB), animatorUpdater },
        fieldRateControl { getParameterName(audioProcessor.apvts, ParamID::fieldRate), animatorUpdater, true },
        fieldModAControl { getParameterName(audioProcessor.apvts, ParamID::fieldModA), animatorUpdater },
        fieldModBControl { getParameterName(audioProcessor.apvts, ParamID::fieldModB), animatorUpdater };

    SliderAttachment
        globalMixControlAttachment, decayControlAttachment, lowCutControlAttachment, highCutControlAttachment,
        strengthControlAttachment, spreadControlAttachment,
        positionRateControlAttachment, positionModAControlAttachment, positionModBControlAttachment,
        fieldRateControlAttachment, fieldModAControlAttachment, fieldModBControlAttachment;

    struct SwapControl {
        LabelledControl<Rotary> swapMinControl, swapMaxControl;
        SliderAttachment swapMinControlAttachment, swapMaxControlAttachment;

        SwapControl(juce::AudioProcessorValueTreeState& apvts, juce::AnimatorUpdater& updater, int i)
            : swapMinControl("Auto Min", updater), swapMaxControl("Auto Max", updater),
            swapMinControlAttachment(apvts, ParamID::irSwapMin(i), swapMinControl.control),
            swapMaxControlAttachment(apvts, ParamID::irSwapMax(i), swapMaxControl.control)
        {}
    };

    std::array<std::unique_ptr<SwapControl>, MAX_IR_COUNT> swapControls;

    // Buttons
    LabelledControl<HoverableTextButton> weightingModeControl { getParameterName(audioProcessor.apvts, ParamID::weightingMode), animatorUpdater };
    juce::AudioProcessorValueTreeState::ButtonAttachment weightingModeControlAttachment;

    HoverableTextButton positionTabButton {animatorUpdater}, fieldTabButton {animatorUpdater};

    HoverableTextButton clearAllButton {animatorUpdater}, randomAllButton {animatorUpdater};

    std::array<std::unique_ptr<IRSlotButton>, MAX_IR_COUNT> irSlotButtons;

    HoverableTextButton loadIRButton {animatorUpdater}, clearIRButton {animatorUpdater}, randomIRButton {animatorUpdater};

    HoverableTextButton settingsButton {animatorUpdater};

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