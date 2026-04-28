#pragma once

#include "GUI/Theme/Theme.h"

#include <JuceHeader.h>

class ComboBoxLookAndFeel : public juce::LookAndFeel_V4 {
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComboBoxLookAndFeel)

public:
    ComboBoxLookAndFeel();
    ~ComboBoxLookAndFeel() override = default;

    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
        int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;

    juce::Font getComboBoxFont(juce::ComboBox&) override;

    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;

    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;

    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area,
        bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, 
        const juce::String& text, const juce::String& shortcutKeyText,
        const juce::Drawable* icon, const juce::Colour* textColour) override;

    juce::Font getPopupMenuFont() override;
};