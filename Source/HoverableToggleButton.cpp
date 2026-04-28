#include "HoverableToggleButton.h"

/* PUBLIC */

HoverableToggleButton::HoverableToggleButton(juce::AnimatorUpdater& updater) : hoverAnim(*this, updater) {}

void HoverableToggleButton::mouseEnter(const juce::MouseEvent& /*e*/) { hoverAnim.setAlpha(1.0f); }
void HoverableToggleButton::mouseExit(const juce::MouseEvent& /*e*/) { hoverAnim.setAlpha(0.0f); }

void HoverableToggleButton::mouseDown(const juce::MouseEvent& e) {
    juce::ToggleButton::mouseDown(e);
    hoverAnim.setAlpha(0.0f);
}

void HoverableToggleButton::mouseUp(const juce::MouseEvent& e) {
    juce::ToggleButton::mouseUp(e);
    hoverAnim.setAlpha(1.0f);
}

float HoverableToggleButton::getHoverAlpha() const { return hoverAnim.getAlpha(); }