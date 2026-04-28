#include "GUIUtilities.h"
#include "WindowOverlayComponent.h"

/* PROTECTED */

void WindowOverlayComponent::beginGesture() { }
void WindowOverlayComponent::updateGesture() { }
void WindowOverlayComponent::endGesture() { if (onWindowChanged) onWindowChanged(start, end - start); }

/* PUBLIC */

WindowOverlayComponent::WindowOverlayComponent(juce::AnimatorUpdater& updater) 
    : RangeSelectorComponent(updater, 4.0f, false), dragHover(*this, updater) {
    dragHandler.typeAccepted = DragAndDropHandler::DragAndDropType::FILES;
    dragHandler.acceptsMultiple = false;
}

void WindowOverlayComponent::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();
    const float startX = inverseMap(start);
    const float endX = inverseMap(end);
    float startAlpha = hoverStart.getAlpha(), endAlpha = hoverEnd.getAlpha();

    // Dim outside window
    g.setColour(Theme::Colors::background.withAlpha(0.5f));
    if (startX > 0.0f) g.fillRect(bounds.withWidth(startX));
    if (endX < bounds.getRight()) g.fillRect(bounds.withLeft(endX));

    // Dragging selection
    if (selecting) {
        g.setColour(Theme::Colors::highlight.withAlpha(0.15f));
        g.fillRect(BoundsF(inverseMap(start), bounds.getY(), inverseMap(end) - inverseMap(start), bounds.getHeight()));
    }

    // Handles
    const float handleThickness = 2.0f;
    g.setColour(Theme::Colors::highlight.withAlpha(juce::jmap(startAlpha, 0.7f, 1.0f)));
    g.drawLine(startX, bounds.getY(), startX, bounds.getBottom(), handleThickness + (handleThickness * startAlpha * 0.5f));
    g.setColour(Theme::Colors::highlight.withAlpha(juce::jmap(endAlpha, 0.7f, 1.0f)));
    g.drawLine(endX, bounds.getY(), endX, bounds.getBottom(), handleThickness + (handleThickness * endAlpha * 0.5f));

    // Dot
    /*
    const float dotRadius = 3.0f;
    const float dotY = bounds.getCentreY();
    g.setColour(Theme::Colors::highlight.withAlpha(juce::jmap(startAlpha, 0.7f, 1.0f)));
    g.fillEllipse(startX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    g.setColour(Theme::Colors::highlight.withAlpha(juce::jmap(endAlpha, 0.7f, 1.0f)));
    g.fillEllipse(endX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    */

    // Drag and drop indication
    const float dragAlpha = dragHover.getAlpha();
    Paint::dragAndDropHover(g, getLocalBounds().toFloat(), dragAlpha, (Theme::Colors::highlight).withAlpha(0.5f));
}

void WindowOverlayComponent::setWindow(float nStart, float nEnd) { setRange(nStart, nEnd); }