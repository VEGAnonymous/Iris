#pragma once

#include "GUI/API/AnimatedAlpha.h"

#include <JuceHeader.h>

class HoverableImageButton : public juce::ImageButton {
private:
    AnimatedAlpha hoverAnim;

public:
    HoverableImageButton(juce::AnimatorUpdater& updater);
    ~HoverableImageButton() override = default;

    void mouseEnter(const juce::MouseEvent& /*e*/) override;
    void mouseExit(const juce::MouseEvent& /*e*/) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    float getHoverAlpha() const;
};