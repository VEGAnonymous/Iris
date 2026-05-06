#pragma once

#include "Core/Defines.h"
#include "GUI/API/ValueTooltipWindow.h"
#include "GUI/Components/Controls/EnvelopeControl.h"
#include "GUI/Components/Controls/HoverableTextButton.h"
#include "GUI/Components/Controls/HoverableToggleButton.h"
#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Components/Controls/RangeSlider.h"
#include "GUI/Components/Controls/RangeSliderAttachment.h"
#include "GUI/Components/Controls/Rotary.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>
#include <array>

class IRControlsComponent : public juce::Component {
private:
    struct IRControl {
        LabelledControl<Rotary> gainControl;

        IRControl(juce::AnimatorUpdater& updater) : gainControl("Gain", updater, true /* bipolar */) {}
    };

    struct SwapControl {
        LabelledControl<RangeSlider> swapRangeSlider;
        LabelledControl<HoverableToggleButton> swapActiveToggle;

        SwapControl(juce::AnimatorUpdater& updater) : swapRangeSlider("Auto Swap Time", updater, 2.0f /* hitRadius */),
            swapActiveToggle("Auto Swap", updater) {}
    };

	MareverbAudioProcessor& audioProcessor;
	juce::AnimatorUpdater& animatorUpdater;
    ValueTooltipWindow& valueTooltip;
    juce::Component& parentComponent;

    HoverableTextButton clearIRButton {animatorUpdater}, randomIRButton {animatorUpdater};

    std::array<std::unique_ptr<IRControl>, MAX_IR_COUNT> irControls;
    EnvelopeControl envelopeControl;

    std::array<std::unique_ptr<SwapControl>, MAX_IR_COUNT> swapControls;

    void initIRButtons();
    void initIRControls();
    void initEnvelopeControl();
    void initSwapControls();

	void prepare();

public:

	IRControlsComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater, ValueTooltipWindow& tooltip, juce::Component& parent);
    ~IRControlsComponent() override = default;

	void paint(juce::Graphics& g) override;
	void resized() override;

    void updateSwapState(int irIndex, bool nActiveState);

    EnvelopeControl* getEnvelopeControl();

    IRControl* getIRControl(int irIndex);
    SwapControl* getSwapControl(int irIndex);
};