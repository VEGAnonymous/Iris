#include "GUI/Theme/Theme.h"
#include "GUI/Theme/LookAndFeel/ScrollBarLookAndFeel.h"

/* WOW! It's fucking nothing! */

ScrollBarLookAndFeel::ScrollBarLookAndFeel() {}

void ScrollBarLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& /*scrollbar*/, int x, int y, int width, int height,
	bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool /*isMouseDown*/) {

    Bounds thumbBounds;
    if (isScrollbarVertical) thumbBounds = { x, thumbStartPosition, width, thumbSize };
    else thumbBounds = { thumbStartPosition, y, thumbSize, height };

    juce::Colour thumbColor = isMouseOver ? Theme::Colors::highlightHover : Theme::Colors::highlight;
    g.setColour(thumbColor.withAlpha(0.6f));
    g.fillRoundedRectangle(thumbBounds.reduced(2).toFloat(), 4.0f);
}