#include "RangeSelectorComponent.h"
#include "Utilities.h"

/* PROTECTED */

float RangeSelectorComponent::map(float x) const { return juce::jlimit(0.0f, 1.0f, x / static_cast<float>(getWidth())); } // Pixels to normalized
float RangeSelectorComponent::inverseMap(float norm) const { return norm * static_cast<float>(getWidth()); } // Normalized to pixels

RangeSelectorComponent::DragTarget RangeSelectorComponent::hitHandle(juce::Point<float> p) const {
    const float distStart = std::abs(p.x - inverseMap(start));
    const float distEnd = std::abs(p.x - inverseMap(end));
    const bool hitStart = distStart <= hitRadius;
    const bool hitEnd = distEnd <= hitRadius;

    if (hitStart && hitEnd) return (distStart < distEnd) ? DragTarget::START : DragTarget::END; // Hit both, prefer closest
    if (hitStart) return DragTarget::START;
    if (hitEnd) return DragTarget::END;

    return DragTarget::NONE;
}

/* PUBLIC */

RangeSelectorComponent::RangeSelectorComponent(juce::AnimatorUpdater& updater, float hitRadius, bool shouldUpdateDuringDrag) : 
    hoverStart(*this, updater), hoverEnd(*this, updater), hitRadius(hitRadius), updateDuringDrag(shouldUpdateDuringDrag) {
    setInterceptsMouseClicks(true, false);
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void RangeSelectorComponent::mouseMove(const juce::MouseEvent& e) {
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

void RangeSelectorComponent::mouseDown(const juce::MouseEvent& e) {
    dragTarget = hitHandle(e.position);
    if (dragTarget != DragTarget::NONE) {
        if (dragTarget == DragTarget::START) hoverStart.setAlpha(0.0f);
        else hoverEnd.setAlpha(0.0f);
        beginGesture();
    } else {
        selecting = true;
        dragStart = map(e.position.x);
        beginGesture();
    }

    repaint();
}

void RangeSelectorComponent::mouseDrag(const juce::MouseEvent& e) {
    const float norm = map(e.position.x);
    if (dragTarget != DragTarget::NONE) {
        if (dragTarget == DragTarget::START) {
            start = juce::jlimit(0.0f, std::max(end - MIN_GAP, 0.0f), norm);
            // Push other handle if window exceeds max length
            if (end - start > maxLength) end = juce::jlimit(start + MIN_GAP, 1.0f, start + maxLength);
        } else {
            end = juce::jlimit(std::min(start + MIN_GAP, 1.0f), 1.0f, norm);
            if (end - start > maxLength) start = juce::jlimit(0.0f, end - MIN_GAP, end - maxLength);
        }
        if (updateDuringDrag) updateGesture();
    } else if (selecting) {
        start = std::min(dragStart, norm);
        end = std::min(start + maxLength, std::max(dragStart, norm));
        if (updateDuringDrag) updateGesture();
    }

    repaint();
}

void RangeSelectorComponent::mouseUp(const juce::MouseEvent&) {
    if (dragTarget != DragTarget::NONE || selecting) {
        endGesture();
        if (dragTarget == DragTarget::START) hoverStart.setAlpha(1.0f);
        else if (dragTarget == DragTarget::END) hoverEnd.setAlpha(1.0f);
    }

    dragTarget = DragTarget::NONE;
    selecting = false;

    repaint();
}

void RangeSelectorComponent::mouseExit(const juce::MouseEvent&) {
    if (dragTarget == DragTarget::NONE) {
        hoverStart.setAlpha(0.0f);
        hoverEnd.setAlpha(0.0f);
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }
}

juce::String RangeSelectorComponent::getTextFromValue(double value) {
    if (textFromValueFunction) return textFromValueFunction(value);
    else return Format::dimensionless(static_cast<float>(value), 4);
}

juce::String RangeSelectorComponent::getTooltip() {
    return "[" + getTextFromValue(start) + " - " + getTextFromValue(end) + "]";
}

void RangeSelectorComponent::setRange(float nStart, float nEnd) {
    start = juce::jlimit(0.0f, 1.0f, nStart);
    end = juce::jlimit(start, 1.0f, nEnd);
    repaint();
}

void RangeSelectorComponent::setMaxLength(float norm) { maxLength = juce::jlimit(0.0f, 1.0f, norm); }

float RangeSelectorComponent::getStart() const { return start; }
float RangeSelectorComponent::getEnd() const { return end; }