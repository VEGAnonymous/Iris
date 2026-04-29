#pragma once

#include "GUI/Theme/Theme.h"

#include <JuceHeader.h>

class ScrollBarLookAndFeel : public juce::LookAndFeel_V4 {
private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScrollBarLookAndFeel)

public:
	ScrollBarLookAndFeel();
	~ScrollBarLookAndFeel() override = default;

	void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height,
		bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;
};