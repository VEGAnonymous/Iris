#pragma once

#include "Core/Defines.h"
#include "GUI/API/ValueTooltipWindow.h"
#include "GUI/Components/IRHeaderComponent.h"
#include "GUI/Components/IRDisplayComponent.h"
#include "GUI/Components/IRControlsComponent.h"
#include "GUI/Components/Controls/EnvelopeControl.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>

class SelectedIRPanel : public juce::Component {
private:
    MareverbAudioProcessor& audioProcessor;
    juce::AnimatorUpdater& animatorUpdater;

    IRHeaderComponent irHeaderComponent;
    IRDisplayComponent irDisplayComponent;
    IRControlsComponent irControlsComponent;

    void prepare();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelectedIRPanel)

public:
    SelectedIRPanel(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater, ValueTooltipWindow& tooltip, juce::Component& parent);
    ~SelectedIRPanel() override = default;

    void resized() override;

    void updateIRSlot(int selectedIR, bool animate);
    void updateIndicatorStyle();

    IRControlsComponent* getIRControlsComponent();
};