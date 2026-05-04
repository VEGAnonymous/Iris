#pragma once

#include <JuceHeader.h>

class TooltipWindowLookAndFeel : public juce::LookAndFeel_V4 {
public:
	TooltipWindowLookAndFeel();
	~TooltipWindowLookAndFeel() override = default;

	void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override;
};