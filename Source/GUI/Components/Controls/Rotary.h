#pragma once

#include "GUI/API/AnimatedAlpha.h"
#include "GUI/API/ValueTooltipClient.h"

#include <JuceHeader.h>

class Rotary : public juce::Slider, public ValueTooltipClient {
private:
    AnimatedAlpha hoverAnim;
    bool bipolar = false;

public:
    Rotary(juce::AnimatorUpdater& updater, bool isBipolar = false);
    ~Rotary() override = default;

    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e);
    void mouseDrag(const juce::MouseEvent& e);
    void mouseDoubleClick(const juce::MouseEvent& e);

    bool hitTest(int x, int y) override;

    juce::String textFromValue(double value) override;
    juce::String getValueTooltip() override;

    void setBipolar(bool nBipolar);
    bool isBipolar() const;

    float getHoverAlpha() const;
};