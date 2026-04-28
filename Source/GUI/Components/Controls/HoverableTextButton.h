#pragma once

#include "GUI/API/AnimatedAlpha.h"
#include "GUI/Theme/Theme.h"

#include <JuceHeader.h>

class HoverableTextButton : public juce::TextButton {
private:
    AnimatedAlpha hoverAnim;

public:
    HoverableTextButton(juce::AnimatorUpdater& updater);
    ~HoverableTextButton() override = default;

    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    float getHoverAlpha() const;

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown);
};