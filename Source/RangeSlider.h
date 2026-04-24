#pragma once

#include "RangeSelectorComponent.h"

#include <JuceHeader.h>

class RangeSlider : public RangeSelectorComponent {
private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RangeSlider)

protected:
	void fireCallback() override;

public:
	std::function<void(float min, float max)> onRangeChanged;

	RangeSlider(juce::AnimatorUpdater& updater, float hitRadius = 4.0f);
	~RangeSlider() override = default;

	void paint(juce::Graphics& g);
};