#pragma once

#include <JuceHeader.h>

class ButtonLookAndFeel : public juce::LookAndFeel_V4 {
private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonLookAndFeel)

public:
    ButtonLookAndFeel();
    ~ButtonLookAndFeel() override = default;

    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, 
        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawImageButton(juce::Graphics& g, juce::Image* image, int imageX, int imageY, int imageW, int imageH,
        const juce::Colour& overlayColour, float imageOpacity, juce::ImageButton& button) override;
};