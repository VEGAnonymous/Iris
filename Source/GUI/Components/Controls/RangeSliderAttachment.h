#pragma once

#include "RangeSlider.h"

#include <JuceHeader.h>

class RangeSliderAttachment {
private:
	juce::RangedAudioParameter& minParam; 
	juce::RangedAudioParameter& maxParam;
	juce::ParameterAttachment minAttachment, maxAttachment;
	RangeSlider& rangeSlider;

	void updateMin(float value);
	void updateMax(float value);

public:
	RangeSliderAttachment(juce::AudioProcessorValueTreeState& apvts,
		const juce::String minParamID, const juce::String maxParamID, RangeSlider& rangeSlider);

	~RangeSliderAttachment() = default;
};