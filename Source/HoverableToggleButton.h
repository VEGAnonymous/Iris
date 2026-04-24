#pragma once

#include "AnimatedAlpha.h"

#include <JuceHeader.h>

class HoverableToggleButton : public juce::ToggleButton {
private:
    AnimatedAlpha hoverAnim;

public:
    HoverableToggleButton(juce::AnimatorUpdater& updater) : hoverAnim(*this, updater) {}
    ~HoverableToggleButton() override = default;

    void mouseEnter(const juce::MouseEvent& /*e*/) override { hoverAnim.setAlpha(1.0f); }
    void mouseExit(const juce::MouseEvent& /*e*/) override { hoverAnim.setAlpha(0.0f); }

    void mouseDown(const juce::MouseEvent& e) override {
        juce::ToggleButton::mouseDown(e);
        hoverAnim.setAlpha(0.0f);
    }

    void mouseUp(const juce::MouseEvent& e) override {
        juce::ToggleButton::mouseUp(e);
        hoverAnim.setAlpha(1.0f);
    }

    float getHoverAlpha() const { return hoverAnim.getAlpha(); }
};