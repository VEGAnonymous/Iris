#include "Rotary.h"

/* PUBLIC */

Rotary::Rotary(juce::AnimatorUpdater& updater, bool isBipolar) : juce::Slider(
    juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
    juce::Slider::TextEntryBoxPosition::NoTextBox),
    hoverAnim(*this, updater),
    bipolar(isBipolar) {
}

void Rotary::mouseEnter(const juce::MouseEvent& e) { 
    hoverAnim.setAlpha(1.0f); 
    updateValueTooltipPosition(e.position);
}
void Rotary::mouseExit(const juce::MouseEvent& /*e*/) { 
    hoverAnim.setAlpha(0.0f); 
    hideValueTooltip();
}

void Rotary::mouseDown(const juce::MouseEvent& e) {
    juce::Slider::mouseDown(e);
    updateValueTooltipText();
    showValueTooltip();
}

void Rotary::mouseDrag(const juce::MouseEvent& e) {
    juce::Slider::mouseDrag(e);
    updateValueTooltipText();
}

void Rotary::mouseDoubleClick(const juce::MouseEvent& e) {
    juce::Slider::mouseDoubleClick(e);
    updateValueTooltipText();
}

bool Rotary::hitTest(int x, int y) {
    // Constrain mouse event region to the knob area
    const float centerX = juce::Component::getWidth() * 0.5f;
    const float centerY = juce::Component::getHeight() * 0.5f;
    const float radius = (static_cast<float>(juce::jmin(juce::Component::getWidth(), juce::Component::getHeight())) * 0.5f) - 2.0f;
    return juce::Point<float>(static_cast<float>(x), static_cast<float>(y)).getDistanceFrom({ centerX, centerY }) <= radius;
}

juce::String Rotary::textFromValue(double value) { return juce::Slider::getTextFromValue(value); }

juce::String Rotary::getValueTooltip() { return textFromValue(getValue()); }

void Rotary::setBipolar(bool nBipolar) { bipolar = nBipolar; }
bool Rotary::isBipolar() const { return bipolar; }

float Rotary::getHoverAlpha() const { return hoverAnim.getAlpha(); }