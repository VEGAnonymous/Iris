#include "WindowOverlayComponent.h"

/* PRIVATE */


float WindowOverlayComponent::map(float x) const { return juce::jlimit(0.0f, 1.0f, x / static_cast<float>(getWidth())); } // Pixels to normalized
float WindowOverlayComponent::inverseMap(float norm) const { return norm * static_cast<float>(getWidth()); } // Normalized to pixels

WindowOverlayComponent::DragTarget WindowOverlayComponent::hitHandle(juce::Point<float> p) const {
    if (std::abs(p.x - inverseMap(start)) <= hitRadius) return DragTarget::START;
    if (std::abs(p.x - inverseMap(end)) <= hitRadius) return DragTarget::END;
    return DragTarget::NONE;
}

/* PUBLIC */

WindowOverlayComponent::WindowOverlayComponent(juce::AnimatorUpdater& updater) : 
    hoverStart(*this, updater), hoverEnd(*this, updater) {
    setInterceptsMouseClicks(true, false);
    setMouseCursor(juce::MouseCursor::NormalCursor);
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
        g.fillRect(juce::Rectangle<float>(inverseMap(start), bounds.getY(), inverseMap(end) - inverseMap(start), bounds.getHeight()));
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
}

void WindowOverlayComponent::mouseMove(const juce::MouseEvent& e) {
    auto target = hitHandle(e.position);
    setMouseCursor(target != DragTarget::NONE
        ? juce::MouseCursor::LeftRightResizeCursor
        : juce::MouseCursor::NormalCursor);

    if (target == DragTarget::START) hoverStart.setAlpha(1.0f);
    else if (target == DragTarget::END) hoverEnd.setAlpha(1.0f);
    else if (target == DragTarget::NONE) {
        hoverStart.setAlpha(0.0f);
        hoverEnd.setAlpha(0.0f);
    }
}

void WindowOverlayComponent::mouseDown(const juce::MouseEvent& e) {
    dragTarget = hitHandle(e.position);
    if (dragTarget != DragTarget::NONE) {
        if (dragTarget == DragTarget::START) hoverStart.setAlpha(0.0f);
        else hoverEnd.setAlpha(0.0f);
    } else {
        selecting = true;
        dragStart = map(e.position.x);
    }
        
    repaint();
}

void WindowOverlayComponent::mouseDrag(const juce::MouseEvent& e) {
    const float norm = map(e.position.x);
    if (dragTarget != DragTarget::NONE) {
        if (dragTarget == DragTarget::START) {
            start = juce::jlimit(0.0f, end - 0.01f, norm);
            // Push end handle if window exceeds max length
            if (end - start > maxLength) end = juce::jlimit(0.0f, 1.0f, start + maxLength);
        } else {
            end = juce::jlimit(start + 0.01f, 1.0f, norm);
            // Push start handle if window exceeds max length
            if (end - start > maxLength) start = juce::jlimit(0.0f, 1.0f, end - maxLength);

        }
    } else if (selecting) {
        start = std::min(dragStart, norm);
        end = std::min(start + maxLength, std::max(dragStart, norm));
    }

    repaint();
}

void WindowOverlayComponent::mouseUp(const juce::MouseEvent&) {
    if (dragTarget != DragTarget::NONE || selecting)
        if (onWindowChanged) onWindowChanged(start, end - start);
    dragTarget = DragTarget::NONE;
    selecting = false;

    repaint();
}

void WindowOverlayComponent::mouseExit(const juce::MouseEvent&) {
    if (dragTarget == DragTarget::NONE) {
        hoverStart.setAlpha(0.0f);
        hoverEnd.setAlpha(0.0f);
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void WindowOverlayComponent::setWindow(float nStart, float nEnd) {
    start = juce::jlimit(0.0f, 1.0f, nStart);
    end = juce::jlimit(start, 1.0f, nEnd);
    repaint();
}

void WindowOverlayComponent::setMaxLength(float norm) { maxLength = juce::jlimit(0.0f, 1.0f, norm); }