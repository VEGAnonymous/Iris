#pragma once

#include "AnimatedAlpha.h"

#include <JuceHeader.h>

class HoverableToggleButton : public juce::ToggleButton {
private:
    AnimatedAlpha hoverAnim;

public:
    HoverableToggleButton(juce::AnimatorUpdater& updater);
    ~HoverableToggleButton() override = default;

    void mouseEnter(const juce::MouseEvent& /*e*/) override;
    void mouseExit(const juce::MouseEvent& /*e*/) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    float getHoverAlpha() const;
};