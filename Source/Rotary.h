#pragma once

#include "Hoverable.h"
#include <JuceHeader.h>

class Rotary : public juce::Slider, public Hoverable {
private:
    bool bipolar = false;

public:
    Rotary(bool isBipolar = false) : juce::Slider(
        juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, 
        juce::Slider::TextEntryBoxPosition::NoTextBox), 
        bipolar(isBipolar) {}

    ~Rotary() override = default;

    // TODO: Should only activate on hover over the knob proper, not just the component area
    // TODO: Animate the hover alpha
    void mouseEnter(const juce::MouseEvent&) override { hoverAlpha = 1.0f; repaint(); }
    void mouseExit(const juce::MouseEvent&)  override { hoverAlpha = 0.0f; repaint(); }

    void setBipolar(bool nBipolar) { bipolar = nBipolar; }
    bool isBipolar() const { return bipolar; }
};