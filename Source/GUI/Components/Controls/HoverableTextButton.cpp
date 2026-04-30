#include "GUI/Components/Controls/HoverableTextButton.h"

/* PUBLIC */

HoverableTextButton::HoverableTextButton(juce::AnimatorUpdater& updater) : juce::TextButton(), hoverAnim(*this, updater) {}

void HoverableTextButton::mouseEnter(const juce::MouseEvent& /*e*/) { hoverAnim.setValue(1.0f); }
void HoverableTextButton::mouseExit(const juce::MouseEvent& /*e*/) { hoverAnim.setValue(0.0f); }

void HoverableTextButton::mouseDown(const juce::MouseEvent& e) {
    juce::TextButton::mouseDown(e);
    hoverAnim.setValue(0.0f);
}

void HoverableTextButton::mouseUp(const juce::MouseEvent& e) {
    juce::TextButton::mouseUp(e);
    hoverAnim.setValue(1.0f);
}

float HoverableTextButton::getHoverAlpha() const { return hoverAnim.getValue(); }

void HoverableTextButton::paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) {
    auto bounds = getLocalBounds();
    g.setColour(Theme::Colors::background);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    getLookAndFeel().drawButtonText(g, *this, isMouseOver, isButtonDown);
}