#pragma once

#include "AnimatedAlpha.h"

#include <JuceHeader.h>

class Rotary : public juce::Slider {
private:
    AnimatedAlpha hoverAnim;
    bool bipolar = false;

public:
    Rotary(juce::AnimatorUpdater& updater, bool isBipolar = false) : juce::Slider(
        juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, 
        juce::Slider::TextEntryBoxPosition::NoTextBox),
        hoverAnim(*this, updater),
        bipolar(isBipolar) {}

    ~Rotary() override = default;

    void mouseEnter(const juce::MouseEvent&) override { hoverAnim.animateIn(); }
    void mouseExit(const juce::MouseEvent&) override { hoverAnim.animateOut(); }

    bool hitTest(int x, int y) override {
        // Constrain mouse event region to the knob area
        const float centerX = juce::Component::getWidth() * 0.5f;
        const float centerY = juce::Component::getHeight() * 0.5f;
        const float radius = (static_cast<float>(juce::jmin(juce::Component::getWidth(), juce::Component::getHeight())) * 0.5f) - 2.0f;
        return juce::Point<float>(static_cast<float>(x), static_cast<float>(y)).getDistanceFrom({ centerX, centerY }) <= radius;
    }

    void setBipolar(bool nBipolar) { bipolar = nBipolar; }
    bool isBipolar() const { return bipolar; }

    float getHoverAlpha() const { return hoverAnim.getAnimateAlpha(); }
};