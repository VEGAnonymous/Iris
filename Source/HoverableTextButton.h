#pragma once

#include "AnimatedAlpha.h"
#include "Theme.h"

#include <JuceHeader.h>

class HoverableTextButton : public juce::TextButton {
private:
    AnimatedAlpha hoverAnim;

public:
    HoverableTextButton(juce::AnimatorUpdater& updater) : juce::TextButton(), hoverAnim(*this, updater, true) {}
    ~HoverableTextButton() override = default;

    void mouseEnter(const juce::MouseEvent& /*e*/) override { hoverAnim.animateIn(); }
    void mouseExit(const juce::MouseEvent& /*e*/) override { hoverAnim.animateOut(); }

    void mouseDown(const juce::MouseEvent& e) override { 
        juce::TextButton::mouseDown(e);
        hoverAnim.animateOut();
    }

    void mouseUp(const juce::MouseEvent& e) override { 
        juce::TextButton::mouseUp(e);
        hoverAnim.animateIn(); 
    }

    float getHoverAlpha() const { return hoverAnim.getAnimateAlpha(); }

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override {
        auto bounds = getLocalBounds();
        g.setColour(Theme::Colors::background);
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
        getLookAndFeel().drawButtonText(g, *this, isMouseOver, isButtonDown);
    }
};