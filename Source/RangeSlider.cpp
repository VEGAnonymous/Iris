#include "RangeSlider.h"

/* PROTECTED */

void RangeSlider::fireCallback() { /*DBG("Range slider callback fired with start: " << start << ", end: " << end); */ if (onRangeChanged) onRangeChanged(start, end); }

/* PUBLIC */

RangeSlider::RangeSlider(juce::AnimatorUpdater& updater, float hitRadius) : RangeSelectorComponent(updater, hitRadius, true) {}

void RangeSlider::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();
    const float startX = inverseMap(start);
    const float endX = inverseMap(end);
    const float trackY = bounds.getCentreY();
    float startAlpha = hoverStart.getAlpha(), endAlpha = hoverEnd.getAlpha();

    // Background
    //g.setColour(Theme::Colors::background);
    //g.fillRoundedRectangle(bounds, 4.0f);

    // Track
    const float trackThickness = 2.0f;
    g.setColour(Theme::Colors::outline);
    g.drawLine(bounds.getX(), trackY, bounds.getRight(), trackY, trackThickness);

    // Active range
    g.setColour(Theme::Colors::highlight.withAlpha(0.5f));
    g.drawLine(startX, trackY, endX, trackY, trackThickness);

    // Handles
    const float handleHeight = bounds.getHeight() * 0.7f;
    const float handleTop = trackY - (handleHeight * 0.5f);
    const float handleBottom = trackY + (handleHeight * 0.5f);
    const float handleThickness = 2.0f;
    const float dotRadius = 3.0f;

    g.setColour(Theme::Colors::highlight.withAlpha(juce::jmap(startAlpha, 0.7f, 1.0f)));
    g.drawLine(startX, handleTop, startX, handleBottom, handleThickness + (handleThickness * startAlpha * 0.5f));
    g.fillEllipse(startX - dotRadius, trackY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);

    g.setColour(Theme::Colors::highlight.withAlpha(juce::jmap(endAlpha, 0.7f, 1.0f)));
    g.drawLine(endX, handleTop, endX, handleBottom, handleThickness + (handleThickness * endAlpha * 0.5f));
    g.fillEllipse(endX - dotRadius, trackY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);

    // Dragging selection
    if (selecting) {
        g.setColour(Theme::Colors::highlight.withAlpha(0.15f));
        g.fillRect(BoundsF(inverseMap(start), handleTop, inverseMap(end) - inverseMap(start), handleHeight));
    }

}