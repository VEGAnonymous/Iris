#pragma once

#include "Core/Defines.h"
#include "GUI/Components/Controls/IRSlotButton.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>
#include <array>

class IRSelectorPanel : public juce::Component {
private:
    MareverbAudioProcessor& audioProcessor;
    juce::AnimatorUpdater& animatorUpdater;

	std::array<std::unique_ptr<IRSlotButton>, MAX_IR_COUNT> irSlotButtons;

    void prepare();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRSelectorPanel)

public:

    IRSelectorPanel(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater);
    ~IRSelectorPanel() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    void setIndicatorStyle();

    IRSlotButton* getIRSlotButton(int irIndex);
};