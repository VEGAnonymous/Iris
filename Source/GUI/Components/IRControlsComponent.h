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
        LabelledControl<Rotary> sendControl;
        juce::AudioProcessorValueTreeState::SliderAttachment sendControlAttachment;

        IRControl(juce::AudioProcessorValueTreeState& apvts, juce::AnimatorUpdater& updater, int i)
            : sendControl("Send", updater, true /* bipolar */),
            sendControlAttachment(apvts, ParamID::irGain(i), sendControl.control) {
        }
    };

    struct SwapControl {
        LabelledControl<RangeSlider> swapRangeSlider;
        RangeSliderAttachment swapRangeAttachment;

        LabelledControl<HoverableToggleButton> swapActiveToggle;
        juce::AudioProcessorValueTreeState::ButtonAttachment swapActiveToggleAttachment;

        SwapControl(juce::AudioProcessorValueTreeState& apvts, juce::AnimatorUpdater& updater, int i)
            : swapRangeSlider("Auto Swap Time", updater, 2.0f /* hitRadius */),
            swapRangeAttachment(apvts, ParamID::irSwapMin(i), ParamID::irSwapMax(i), swapRangeSlider.control),
            swapActiveToggle("Auto Swap", updater),
            swapActiveToggleAttachment(apvts, ParamID::irSwapActive(i), swapActiveToggle.control) {
        }
    };

	MareverbAudioProcessor& audioProcessor;
	juce::AnimatorUpdater& animatorUpdater;
    ValueTooltipWindow& valueTooltip;
    juce::Component& parentComponent;

    HoverableTextButton clearIRButton {animatorUpdater}, randomIRButton {animatorUpdater};

    std::array<std::unique_ptr<IRControl>, MAX_IR_COUNT> irControls;
    EnvelopeControl envelopeControl;

    std::array<std::unique_ptr<SwapControl>, MAX_IR_COUNT> swapControls;

	void prepare();

public:

	IRControlsComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater, ValueTooltipWindow& tooltip, juce::Component& parent);
    ~IRControlsComponent() override = default;

	void paint(juce::Graphics& g) override;
	void resized() override;

    void updateSwapState(int irIndex);

    EnvelopeControl* getEnvelopeControl();

    IRControl* getIRControl(int irIndex);
    SwapControl* getSwapControl(int irIndex);
};