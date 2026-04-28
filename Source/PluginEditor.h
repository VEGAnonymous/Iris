#pragma once

#include "GUI/API/ValueTooltipWindow.h"
#include "GUI/Components/IRHeaderComponent.h"
#include "GUI/Components/PolarMapComponent.h"
#include "GUI/Components/SettingsComponent.h"
#include "GUI/Components/WindowOverlayComponent.h"
#include "GUI/Components/Base/WaveformComponent.h"
#include "GUI/Components/Controls/EnvelopeControl.h"
#include "GUI/Components/Controls/HoverableTextButton.h"
#include "GUI/Components/Controls/HoverableToggleButton.h"
#include "GUI/Components/Controls/IRSlotButton.h"
#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Components/Controls/RangeSlider.h"
#include "GUI/Components/Controls/RangeSliderAttachment.h"
#include "GUI/Components/Controls/Rotary.h"
#include "GUI/Theme/LookAndFeel/ButtonLookAndFeel.h"
#include "GUI/Theme/LookAndFeel/ComboBoxLookAndFeel.h"
#include "GUI/Theme/LookAndFeel/RotaryLookAndFeel.h"

#include "PluginProcessor.h"

#include <JuceHeader.h>

class MareverbAudioProcessorEditor  : public juce::AudioProcessorEditor,
    juce::AudioProcessorValueTreeState::Listener, juce::Timer {
private:
    MareverbAudioProcessor& audioProcessor;

    // Theme
    juce::AnimatorUpdater animatorUpdater;

    ButtonLookAndFeel buttonLookAndFeel;
    ComboBoxLookAndFeel comboBoxLookAndFeel;
    RotaryLookAndFeel rotaryLookAndFeel;

    // Listeners and callbacks
    std::atomic<bool> positionPathChanged { false };
    std::atomic<bool> selectedIRChanged { false };

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    // GUI state
    void updateIRSlot(bool animate = false);
    void syncPosition();
    void syncField();

    // Controls
    std::vector<ControlDef> controls { { 
       // ParamID                   Component                ControlGroup               Slider (opt)                  applyLookAndFeel 
        { ParamID::globalMix,       &globalMixControl,       ControlGroup::GLOBAL,      &globalMixControl.control,    [&]() { globalMixControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { ParamID::decay,           &decayControl,           ControlGroup::GLOBAL,      &decayControl.control,        [&]() { decayControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { ParamID::lowCut,          &lowCutControl,          ControlGroup::GLOBAL,      &lowCutControl .control,      [&]() { lowCutControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { ParamID::highCut,         &highCutControl,         ControlGroup::GLOBAL,      &highCutControl.control,      [&]() { highCutControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { ParamID::weightingMode,   &weightingModeControl,   ControlGroup::INTERACTION, nullptr,                      [&]() { weightingModeControl.control.setLookAndFeel(&buttonLookAndFeel); } },
        { ParamID::strength,        &strengthControl,        ControlGroup::INTERACTION, &strengthControl.control,     [&]() { strengthControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { ParamID::spread,          &spreadControl,          ControlGroup::INTERACTION, &spreadControl.control,       [&]() { spreadControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { ParamID::positionPattern, &positionPatternControl, ControlGroup::POSITION,    nullptr,                      [&]() { positionPatternControl.setLookAndFeel(&comboBoxLookAndFeel); } },
        { ParamID::positionRate,    &positionRateControl,    ControlGroup::POSITION,    &positionRateControl.control, [&]() { positionRateControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { ParamID::positionModA,    &positionModAControl,    ControlGroup::POSITION,    &positionModAControl.control, [&]() { positionModAControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { ParamID::positionModB,    &positionModBControl,    ControlGroup::POSITION,    &positionModBControl.control, [&]() { positionModBControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { ParamID::fieldPattern,    &fieldPatternControl,    ControlGroup::FIELD,       nullptr,                      [&]() { fieldPatternControl.setLookAndFeel(&comboBoxLookAndFeel); } },
        { ParamID::fieldRate,       &fieldRateControl,       ControlGroup::FIELD,       &fieldRateControl.control,    [&]() { fieldRateControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { ParamID::fieldModA,       &fieldModAControl,       ControlGroup::FIELD,       &fieldModAControl.control,    [&]() { fieldModAControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
        { ParamID::fieldModB,       &fieldModBControl,       ControlGroup::FIELD,       &fieldModBControl.control,    [&]() { fieldModBControl.control.setLookAndFeel(&rotaryLookAndFeel); } },
    } };

    inline juce::String getParameterName(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID) {
        if (auto* parameter = apvts.getParameter(paramID)) return parameter->getName(32);
        else return paramID;
    }

    // Knobs
    LabelledControl<Rotary>
        globalMixControl    { getParameterName(audioProcessor.apvts, ParamID::globalMix),    animatorUpdater },
        decayControl        { getParameterName(audioProcessor.apvts, ParamID::decay),        animatorUpdater },
        lowCutControl       { getParameterName(audioProcessor.apvts, ParamID::lowCut),       animatorUpdater },
        highCutControl      { getParameterName(audioProcessor.apvts, ParamID::highCut),      animatorUpdater },
        strengthControl     { getParameterName(audioProcessor.apvts, ParamID::strength),     animatorUpdater },
        spreadControl       { getParameterName(audioProcessor.apvts, ParamID::spread),       animatorUpdater },
        positionRateControl { getParameterName(audioProcessor.apvts, ParamID::positionRate), animatorUpdater, true /* bipolar */},
        positionModAControl { getParameterName(audioProcessor.apvts, ParamID::positionModA), animatorUpdater },
        positionModBControl { getParameterName(audioProcessor.apvts, ParamID::positionModB), animatorUpdater },
        fieldRateControl    { getParameterName(audioProcessor.apvts, ParamID::fieldRate),    animatorUpdater, true /* bipolar */},
        fieldModAControl    { getParameterName(audioProcessor.apvts, ParamID::fieldModA),    animatorUpdater },
        fieldModBControl    { getParameterName(audioProcessor.apvts, ParamID::fieldModB),    animatorUpdater };

    SliderAttachment
        globalMixControlAttachment, decayControlAttachment, lowCutControlAttachment, highCutControlAttachment,
        strengthControlAttachment, spreadControlAttachment,
        positionRateControlAttachment, positionModAControlAttachment, positionModBControlAttachment,
        fieldRateControlAttachment, fieldModAControlAttachment, fieldModBControlAttachment;

    // Swap controls
    struct SwapControl {
        LabelledControl<RangeSlider> swapRangeSlider;
        RangeSliderAttachment swapRangeAttachment;

        LabelledControl<HoverableToggleButton> swapActiveToggle;
        juce::AudioProcessorValueTreeState::ButtonAttachment swapActiveToggleAttachment;

        SwapControl(juce::AudioProcessorValueTreeState& apvts, juce::AnimatorUpdater& updater, int i)
            : swapRangeSlider("Auto Swap Time", updater, 2.0f),
              swapRangeAttachment(apvts, ParamID::irSwapMin(i), ParamID::irSwapMax(i), swapRangeSlider.control),
              swapActiveToggle("Auto Swap", updater),
              swapActiveToggleAttachment(apvts, ParamID::irSwapActive(i), swapActiveToggle.control) {}
    };

    std::array<std::unique_ptr<SwapControl>, MAX_IR_COUNT> swapControls;

    // Buttons
    // TODO: Replace with toggle switch
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

    // Tooltips
    ValueTooltipWindow valueTooltip;

    // Components
    PolarMapComponent polarMapComponent;
    IRHeaderComponent irHeaderComponent;
    WaveformComponent irWaveformComponent;
    WindowOverlayComponent windowOverlayComponent;
    EnvelopeControl envelopeControl;

    std::unique_ptr<SettingsComponent> settingsModal;

    void initPositionFieldControls();
    void initTopBar();
    void initIRSlotButtons();
    void initSelectedIR();

    void initComponents();

    // Layout
    void layoutLeftPanel(Bounds bounds);
    void layoutRightPanel(Bounds bounds);

    void layoutPositionFieldControls(Bounds bounds);

    void layoutTopBar(Bounds bounds);
    void layoutIRSelectorGrid(Bounds bounds);
    void layoutSelectedIR(Bounds bounds);
    void layoutIRControls(Bounds bounds);
    void layoutInteractionControls(Bounds bounds);
    void layoutGlobalControls(Bounds bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MareverbAudioProcessorEditor)

public:
    MareverbAudioProcessorEditor(MareverbAudioProcessor&);
    ~MareverbAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    std::vector<ControlDef> getControls();
    std::vector<ControlDef> getControlsGroup(ControlGroup group);
};