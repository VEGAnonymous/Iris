#pragma once

#include "RangeSlider.h"

#include <JuceHeader.h>

class RangeSliderAttachment {
private:
	juce::RangedAudioParameter& minParam; 
	juce::RangedAudioParameter& maxParam;
	juce::ParameterAttachment minAttachment, maxAttachment;
	RangeSlider& rangeSlider;

	void updateMin(float value) { rangeSlider.setRange(minParam.convertTo0to1(value), rangeSlider.getEnd()); }
	void updateMax(float value) { rangeSlider.setRange(rangeSlider.getStart(), maxParam.convertTo0to1(value)); }

public:
	RangeSliderAttachment(juce::AudioProcessorValueTreeState& apvts, 
		const juce::String minParamID, const juce::String maxParamID, RangeSlider& rangeSlider) :
		minParam(*apvts.getParameter(minParamID)), maxParam(*apvts.getParameter(maxParamID)), rangeSlider(rangeSlider),
		minAttachment(minParam, [this](float value) { updateMin(value); }),
		maxAttachment(maxParam, [this](float value) { updateMax(value); }) {

		// Set the value in the APVTS
		rangeSlider.onDragStart = [this]() {
			minAttachment.beginGesture();
			maxAttachment.beginGesture();
		};
		rangeSlider.onRangeChanged = [this](float min, float max) {
			minAttachment.setValueAsPartOfGesture(minParam.convertFrom0to1(min));
			maxAttachment.setValueAsPartOfGesture(maxParam.convertFrom0to1(max));
		};
		rangeSlider.onDragEnd = [this]() {
			minAttachment.endGesture();
			maxAttachment.endGesture();
		};

		minAttachment.sendInitialUpdate();
		maxAttachment.sendInitialUpdate();
	}

	~RangeSliderAttachment() = default;
};