#pragma once

#include "GUI/API/ValueTooltipWindow.h"
#include "GUI/Components/DirectoryManagerComponent.h"
#include "GUI/Components/SettingsComponent.h"

#include "GUI/Panels/GlobalControlsPanel.h"
#include "GUI/Panels/InteractionControlsPanel.h"
#include "GUI/Panels/PolarMapPanel.h"
#include "GUI/Panels/PositionFieldControlsPanel.h"
#include "GUI/Panels/TopBarPanel.h"
#include "GUI/Panels/IRSelectorPanel.h"
#include "GUI/Panels/SelectedIRPanel.h"

#include "GUI/Theme/LookAndFeel/MareverbLookAndFeel.h"

#include "PluginProcessor.h"

#include <JuceHeader.h>

struct ControlDef {
    juce::String paramID = "";
    juce::Component* component;

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
    juce::TooltipWindow tooltipWindow;
    ValueTooltipWindow valueTooltip;
    
    // ControlDefs
    std::vector<ControlDef> controls { { 
       // ParamID                   Component                Slider (opt)                  
        { ParamID::globalMix,       &globalMixControl,       &globalMixControl.control    },
        { ParamID::decay,           &decayControl,           &decayControl.control        },
        { ParamID::lowCut,          &lowCutControl,          &lowCutControl .control      },
        { ParamID::highCut,         &highCutControl,         &highCutControl.control      },
        { ParamID::weightingMode,   &weightingModeControl,   nullptr                      },
        { ParamID::strength,        &strengthControl,        &strengthControl.control     },
        { ParamID::spread,          &spreadControl,          &spreadControl.control       },
        { ParamID::positionPattern, &positionPatternControl, nullptr                      },
        { ParamID::positionRate,    &positionRateControl,    &positionRateControl.control },
        { ParamID::positionModA,    &positionModAControl,    &positionModAControl.control },
        { ParamID::positionModB,    &positionModBControl,    &positionModBControl.control },
        { ParamID::fieldPattern,    &fieldPatternControl,    nullptr                      },
        { ParamID::fieldRate,       &fieldRateControl,       &fieldRateControl.control    },
        { ParamID::fieldModA,       &fieldModAControl,       &fieldModAControl.control    },
        { ParamID::fieldModB,       &fieldModBControl,       &fieldModBControl.control    },
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

    // ComboBoxes
    LabelledControl<juce::ComboBox> 
        weightingModeControl   { getParameterName(audioProcessor.apvts, ParamID::weightingMode) },
        positionPatternControl { getParameterName(audioProcessor.apvts, ParamID::positionPattern) },
        fieldPatternControl    { getParameterName(audioProcessor.apvts, ParamID::fieldPattern) };
    juce::AudioProcessorValueTreeState::ComboBoxAttachment 
        weightingModeControlAttachment, 
        positionPatternControlAttachment, 
        fieldPatternControlAttachment;

    // Panels
    PolarMapPanel polarMapPanel;
    PositionFieldControlsPanel positionFieldControlsPanel;

    TopBarPanel topBarPanel;
    IRSelectorPanel irSelectorPanel;
    SelectedIRPanel selectedIRPanel;
    InteractionControlsPanel interactionControlsPanel;
    GlobalControlsPanel globalControlsPanel;

    std::unique_ptr<SettingsComponent> settingsModal;
    std::unique_ptr<DirectoryManagerComponent> directoryManagerModal;

    // Listeners and callbacks
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    // GUI state
    void updateIRSlot(bool animate = false);
    void syncPosition();
    void syncField();
    void syncIRs(bool animate = true);
    void syncIRControls();
    void syncSwap();
    void syncIndicatorStyles();
    void syncSettings();

    // Initialization
    void prepare();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MareverbAudioProcessorEditor)

public:
    MareverbAudioProcessorEditor(MareverbAudioProcessor&);
    ~MareverbAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;

    void initState();
};