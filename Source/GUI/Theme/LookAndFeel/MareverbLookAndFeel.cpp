#include "MareverbLookAndFeel.h"

/* PUBLIC */

MareverbLookAndFeel::MareverbLookAndFeel() {}

// Button

juce::Font MareverbLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight) { 
	return buttonLookAndFeel.getTextButtonFont(button, buttonHeight);
}

void MareverbLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
	bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
	
	buttonLookAndFeel.drawButtonText(g, button, 
		shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
}

void MareverbLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
	bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
	
	buttonLookAndFeel.drawToggleButton(g, button, 
		shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
}

// ComboBox

void MareverbLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
	int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& comboBox) {
	
	comboBoxLookAndFeel.drawComboBox(g, width, height, isButtonDown, 
		buttonX, buttonY, buttonW, buttonH, comboBox);
}

juce::Font MareverbLookAndFeel::getComboBoxFont(juce::ComboBox& comboBox) {
	return comboBoxLookAndFeel.getComboBoxFont(comboBox);
}

void MareverbLookAndFeel::positionComboBoxText(juce::ComboBox& comboBox, juce::Label& label) {
	comboBoxLookAndFeel.positionComboBoxText(comboBox, label);
}

void MareverbLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height) {
	comboBoxLookAndFeel.drawPopupMenuBackground(g, width, height);
}

void MareverbLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
	bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
	const juce::String& text, const juce::String& shortcutKeyText,
	const juce::Drawable* icon, const juce::Colour* textColour) {

	comboBoxLookAndFeel.drawPopupMenuItem(g, area, 
		isSeparator, isActive, isHighlighted, isTicked, hasSubMenu, 
		text, shortcutKeyText, 
		icon, textColour);
}

juce::Font MareverbLookAndFeel::getPopupMenuFont() {
	return comboBoxLookAndFeel.getPopupMenuFont();
}

// Rotary

void MareverbLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
	float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) {

	rotaryLookAndFeel.drawRotarySlider(g, x, y, width, height,
		sliderPosProportional, rotaryStartAngle, rotaryEndAngle, slider);
}