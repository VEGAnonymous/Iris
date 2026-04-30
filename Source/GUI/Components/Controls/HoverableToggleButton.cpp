#include "GUI/Components/Controls/HoverableToggleButton.h"

/* PUBLIC */

HoverableToggleButton::HoverableToggleButton(juce::AnimatorUpdater& updater) : hoverAnim(*this, updater) {}

void HoverableToggleButton::mouseEnter(const juce::MouseEvent& /*e*/) { hoverAnim.setValue(1.0f); }
void HoverableToggleButton::mouseExit(const juce::MouseEvent& /*e*/) { hoverAnim.setValue(0.0f); }

void HoverableToggleButton::mouseDown(const juce::MouseEvent& e) {
    juce::ToggleButton::mouseDown(e);
    hoverAnim.setValue(0.0f);
}

void HoverableToggleButton::mouseUp(const juce::MouseEvent& e) {
    juce::ToggleButton::mouseUp(e);
    hoverAnim.setValue(1.0f);
}

float HoverableToggleButton::getHoverAlpha() const { return hoverAnim.getValue(); }