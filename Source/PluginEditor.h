#pragma once

#include "GUI/API/ValueTooltipWindow.h"
#include "GUI/Components/IRHeaderComponent.h"
#include "GUI/Components/PolarMapComponent.h"
#include "GUI/Components/SettingsComponent.h"
#include "GUI/Components/WindowOverlayComponent.h"
#include "GUI/Components/Base/WaveformComponent.h"
#include "GUI/Components/Controls/HoverableTextButton.h"
#include "GUI/Components/Controls/HoverableToggleButton.h"
#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Components/Controls/RangeSlider.h"
#include "GUI/Components/Controls/RangeSliderAttachment.h"
#include "GUI/Components/Controls/Rotary.h"
#include "GUI/Panels/TopBarPanel.h"
#include "GUI/Panels/IRSelectorPanel.h"
#include "GUI/Panels/SelectedIRPanel.h"
#include "GUI/Theme/LookAndFeel/ButtonLookAndFeel.h"
#include "GUI/Theme/LookAndFeel/ComboBoxLookAndFeel.h"
#include "GUI/Theme/LookAndFeel/RotaryLookAndFeel.h"

#include "GUI/Theme/LookAndFeel/MareverbLookAndFeel.h"

#include "PluginProcessor.h"

#include <JuceHeader.h>

struct ControlDef {
    juce::String paramID = "";
    juce::Component* component;
    ControlGroup group;

    juce::Slider* slider = nullptr;
};

class MareverbAudioProcessorEditor  : public juce::AudioProcessorEditor,
    juce::AudioProcessorValueTreeState::Listener, juce::Timer {
private:
    MareverbAudioProcessor& audioProcessor;

    // Theme
    MareverbLookAndFeel mainLookAndFeel;

    // API
    juce::AnimatorUpdater animatorUpdater;
    ValueTooltipWindow valueTooltip;

    // Listeners and callbacks
    std::atomic<bool> positionPathChanged { false };

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    // GUI state
    void updateIRSlot(bool animate = false);
    void syncPosition();
    void syncField();

    // Controls
    std::vector<ControlDef> controls { { 
       // ParamID                   Component                ControlGroup               Slider (opt)                  
        { ParamID::globalMix,       &globalMixControl,       ControlGroup::GLOBAL,      &globalMixControl.control    },
        { ParamID::decay,           &decayControl,           ControlGroup::GLOBAL,      &decayControl.control        },
        { ParamID::lowCut,          &lowCutControl,          ControlGroup::GLOBAL,      &lowCutControl .control      },
        { ParamID::highCut,         &highCutControl,         ControlGroup::GLOBAL,      &highCutControl.control      },
        { ParamID::weightingMode,   &weightingModeControl,   ControlGroup::INTERACTION, nullptr                      },
        { ParamID::strength,        &strengthControl,        ControlGroup::INTERACTION, &strengthControl.control     },
        { ParamID::spread,          &spreadControl,          ControlGroup::INTERACTION, &spreadControl.control       },
        { ParamID::positionPattern, &positionPatternControl, ControlGroup::POSITION,    nullptr                      },
        { ParamID::positionRate,    &positionRateControl,    ControlGroup::POSITION,    &positionRateControl.control },
        { ParamID::positionModA,    &positionModAControl,    ControlGroup::POSITION,    &positionModAControl.control },
        { ParamID::positionModB,    &positionModBControl,    ControlGroup::POSITION,    &positionModBControl.control },
        { ParamID::fieldPattern,    &fieldPatternControl,    ControlGroup::FIELD,       nullptr                      },
        { ParamID::fieldRate,       &fieldRateControl,       ControlGroup::FIELD,       &fieldRateControl.control    },
        { ParamID::fieldModA,       &fieldModAControl,       ControlGroup::FIELD,       &fieldModAControl.control    },
        { ParamID::fieldModB,       &fieldModBControl,       ControlGroup::FIELD,       &fieldModBControl.control    },
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
        globalMixControlAttachment, 
        decayControlAttachment, 
        lowCutControlAttachment,
        highCutControlAttachment,
        strengthControlAttachment, 
        spreadControlAttachment,
        positionRateControlAttachment, 
        positionModAControlAttachment, 
        positionModBControlAttachment,
        fieldRateControlAttachment, 
        fieldModAControlAttachment, 
        fieldModBControlAttachment;

    // Buttons
    // TODO: Replace with toggle switch
    LabelledControl<HoverableTextButton> weightingModeControl { getParameterName(audioProcessor.apvts, ParamID::weightingMode), animatorUpdater };
    juce::AudioProcessorValueTreeState::ButtonAttachment weightingModeControlAttachment;

    HoverableTextButton positionTabButton {animatorUpdater}, fieldTabButton {animatorUpdater};

    // ComboBoxes
    juce::ComboBox positionPatternControl, fieldPatternControl;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment positionPatternControlAttachment, fieldPatternControlAttachment;

    // Panels
    TopBarPanel topBarPanel;
    IRSelectorPanel irSelectorPanel;
    SelectedIRPanel selectedIRPanel;

    // Components
    PolarMapComponent polarMapComponent;

    std::unique_ptr<SettingsComponent> settingsModal;

    void initPositionFieldControls();;
    void initComponents();

    // Layout
    void layoutLeftPanel(Bounds bounds);
    void layoutRightPanel(Bounds bounds);

    void layoutPositionFieldControls(Bounds bounds);
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