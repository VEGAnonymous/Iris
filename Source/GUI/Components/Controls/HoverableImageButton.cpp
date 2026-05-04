#include "GUI/Components/Controls/HoverableImageButton.h"

/* PUBLIC */

HoverableImageButton::HoverableImageButton(juce::AnimatorUpdater& updater) : juce::ImageButton(), hoverAnim(*this, updater) {}

void HoverableImageButton::mouseEnter(const juce::MouseEvent& /*e*/) { hoverAnim.setValue(1.0f); }
void HoverableImageButton::mouseExit(const juce::MouseEvent& /*e*/) { hoverAnim.setValue(0.0f); }

void HoverableImageButton::mouseDown(const juce::MouseEvent& e) {
    juce::ImageButton::mouseDown(e);
    hoverAnim.setValue(0.0f);
}

void HoverableImageButton::mouseUp(const juce::MouseEvent& e) {
    juce::ImageButton::mouseUp(e);
    hoverAnim.setValue(1.0f);
}
void HoverableImageButton::setColorOnHover(juce::Colour nColor) { colorOnHover = nColor; }

float HoverableImageButton::getHoverAlpha() const { return hoverAnim.getValue(); }
juce::Colour HoverableImageButton::getColoronHover() const { return colorOnHover; }