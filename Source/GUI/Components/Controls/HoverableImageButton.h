#pragma once

#include "GUI/API/AnimatedValue.h"
#include "GUI/Theme/Theme.h"

#include <JuceHeader.h>

class HoverableImageButton : public juce::ImageButton {
private:
    AnimatedValue hoverAnim;

    juce::Colour colorOnHover = Theme::Colors::textLight;

public:
    HoverableImageButton(juce::AnimatorUpdater& updater);
    ~HoverableImageButton() override = default;

    void mouseEnter(const juce::MouseEvent& /*e*/) override;
    void mouseExit(const juce::MouseEvent& /*e*/) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void setColorOnHover(juce::Colour nColor);

    float getHoverAlpha() const;
    juce::Colour getColoronHover() const;
};