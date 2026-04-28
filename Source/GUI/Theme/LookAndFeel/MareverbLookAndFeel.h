#pragma once

#include "GUI/Theme/LookAndFeel/ButtonLookAndFeel.h"
#include "GUI/Theme/LookAndFeel/ComboBoxLookAndFeel.h"
#include "GUI/Theme/LookAndFeel/RotaryLookAndFeel.h"

#include <JuceHeader.h>

class MareverbLookAndFeel : public juce::LookAndFeel_V4 {
private:
	// Could make these each interface and allow setting via subclasses?
	ButtonLookAndFeel buttonLookAndFeel;
	ComboBoxLookAndFeel comboBoxLookAndFeel;
	RotaryLookAndFeel rotaryLookAndFeel;

public:
	MareverbLookAndFeel();
	~MareverbLookAndFeel() override = default;

	// Button
	juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;

	void drawButtonText(juce::Graphics& g, juce::TextButton& button,
		bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
		bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	// ComboBox
	void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
		int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& comboBox) override;

	juce::Font getComboBoxFont(juce::ComboBox& comboBox) override;

	void positionComboBoxText(juce::ComboBox& comboBox, juce::Label& label) override;

	void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

	void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
		bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
		const juce::String& text, const juce::String& shortcutKeyText,
		const juce::Drawable* icon, const juce::Colour* textColour) override;

	juce::Font getPopupMenuFont() override;

	// Rotary
	void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
		float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
};