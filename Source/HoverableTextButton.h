#pragma once

#include "AnimatedAlpha.h"
#include "Theme.h"

#include <JuceHeader.h>

class HoverableTextButton : public juce::TextButton {
private:
    AnimatedAlpha hoverAnim;

public:
    HoverableTextButton(juce::AnimatorUpdater& updater) : juce::TextButton(), hoverAnim(*this, updater) {}
    ~HoverableTextButton() override = default;

    void mouseEnter(const juce::MouseEvent& /*e*/) override { hoverAnim.setAlpha(1.0f); }
    void mouseExit(const juce::MouseEvent& /*e*/) override { hoverAnim.setAlpha(0.0f); }

    void mouseDown(const juce::MouseEvent& e) override { 
        juce::TextButton::mouseDown(e);
        hoverAnim.setAlpha(0.0f);
    }

    void mouseUp(const juce::MouseEvent& e) override { 
        juce::TextButton::mouseUp(e);
        hoverAnim.setAlpha(1.0f); 
    }

    float getHoverAlpha() const { return hoverAnim.getAlpha(); }

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override {
        auto bounds = getLocalBounds();
        g.setColour(Theme::Colors::background);
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
        getLookAndFeel().drawButtonText(g, *this, isMouseOver, isButtonDown);
    }
};