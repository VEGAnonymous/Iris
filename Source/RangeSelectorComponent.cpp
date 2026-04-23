#include "RangeSelectorComponent.h"

/* PROTECTED */

float RangeSelectorComponent::map(float x) const { return juce::jlimit(0.0f, 1.0f, x / static_cast<float>(getWidth())); } // Pixels to normalized
float RangeSelectorComponent::inverseMap(float norm) const { return norm * static_cast<float>(getWidth()); } // Normalized to pixels

RangeSelectorComponent::DragTarget RangeSelectorComponent::hitHandle(juce::Point<float> p) const {
    if (std::abs(p.x - inverseMap(start)) <= hitRadius) return DragTarget::START;
    if (std::abs(p.x - inverseMap(end)) <= hitRadius) return DragTarget::END;
    return DragTarget::NONE;
}

/* PUBLIC */

RangeSelectorComponent::RangeSelectorComponent(juce::AnimatorUpdater& updater, bool shouldUpdateDuringDrag) : 
    hoverStart(*this, updater), hoverEnd(*this, updater), updateDuringDrag(shouldUpdateDuringDrag) {
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
    } else {
        selecting = true;
        dragStart = map(e.position.x);
    }

    repaint();
}

void RangeSelectorComponent::mouseDrag(const juce::MouseEvent& e) {
    const float norm = map(e.position.x);
    if (dragTarget != DragTarget::NONE) {
        if (dragTarget == DragTarget::START) {
            start = juce::jlimit(0.0f, end - 0.01f, norm);
            // Push other handle if window exceeds max length
            if (end - start > maxLength) end = juce::jlimit(0.0f, 1.0f, start + maxLength);
        } else {
            end = juce::jlimit(start + 0.01f, 1.0f, norm);
            if (end - start > maxLength) start = juce::jlimit(0.0f, 1.0f, end - maxLength);
        }
        if (updateDuringDrag) fireCallback();
    } else if (selecting) {
        start = std::min(dragStart, norm);
        end = std::min(start + maxLength, std::max(dragStart, norm));
        if (updateDuringDrag) fireCallback();
    }

    repaint();
}

void RangeSelectorComponent::mouseUp(const juce::MouseEvent&) {
    if (dragTarget != DragTarget::NONE || selecting) {
        fireCallback();
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

void RangeSelectorComponent::setRange(float nStart, float nEnd) {
    start = juce::jlimit(0.0f, 1.0f, nStart);
    end = juce::jlimit(start, 1.0f, nEnd);
    repaint();
}

void RangeSelectorComponent::setMaxLength(float norm) { maxLength = juce::jlimit(0.0f, 1.0f, norm); }