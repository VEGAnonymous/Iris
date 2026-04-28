#pragma once

#include "Core/Defines.h"
#include "GUI/API/ValueTooltipWindow.h"
#include "GUI/Components/Controls/EnvelopeControl.h"
#include "GUI/Components/Controls/HoverableTextButton.h"
#include "GUI/Components/Controls/HoverableToggleButton.h"
#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Components/Controls/RangeSlider.h"
#include "GUI/Components/Controls/RangeSliderAttachment.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>
#include <array>

class IRControlsComponent : public juce::Component, juce::AudioProcessorValueTreeState::Listener, juce::Timer {
private:
    struct SwapControl {
        LabelledControl<RangeSlider> swapRangeSlider;
        RangeSliderAttachment swapRangeAttachment;

        LabelledControl<HoverableToggleButton> swapActiveToggle;
        juce::AudioProcessorValueTreeState::ButtonAttachment swapActiveToggleAttachment;

        SwapControl(juce::AudioProcessorValueTreeState& apvts, juce::AnimatorUpdater& updater, int i)
            : swapRangeSlider("Auto Swap Time", updater, 2.0f),
            swapRangeAttachment(apvts, ParamID::irSwapMin(i), ParamID::irSwapMax(i), swapRangeSlider.control),
            swapActiveToggle("Auto Swap", updater),
            swapActiveToggleAttachment(apvts, ParamID::irSwapActive(i), swapActiveToggle.control) {
        }
    };

	MareverbAudioProcessor& audioProcessor;
	juce::AnimatorUpdater& animatorUpdater;
    ValueTooltipWindow valueTooltip;

    HoverableTextButton clearIRButton {animatorUpdater}, randomIRButton {animatorUpdater};
    EnvelopeControl envelopeControl;
    std::array<std::unique_ptr<SwapControl>, MAX_IR_COUNT> swapControls;

	void prepare();

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

public:

	IRControlsComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater);
	~IRControlsComponent() override;

	void paint(juce::Graphics& g) override;
	void resized() override;

    EnvelopeControl* getEnvelopeControl();
    SwapControl* getSwapControl(int irIndex);
};