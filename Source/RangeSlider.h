#pragma once

#include "RangeSelectorComponent.h"

#include <JuceHeader.h>

class RangeSlider : public RangeSelectorComponent {
private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RangeSlider)

protected:
	void beginGesture() override;
	void updateGesture() override;
	void endGesture() override;

public:
	std::function<void()> onDragStart;
	std::function<void(float min, float max)> onRangeChanged;
	std::function<void()> onDragEnd;

	RangeSlider(juce::AnimatorUpdater& updater, float hitRadius = 4.0f);
	~RangeSlider() override = default;

	void paint(juce::Graphics& g);
};