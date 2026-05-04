#pragma once

#include "GUI/Theme/LookAndFeel/ButtonLookAndFeel.h"
#include "GUI/Theme/LookAndFeel/ComboBoxLookAndFeel.h"
#include "GUI/Theme/LookAndFeel/RotaryLookAndFeel.h"
#include "GUI/Theme/LookAndFeel/ScrollBarLookAndFeel.h"
#include "GUI/Theme/LookAndFeel/TextEditorLookAndFeel.h"
#include "GUI/Theme/LookAndFeel/TooltipWindowLookAndFeel.h"

#include <JuceHeader.h>

class MareverbLookAndFeel : public juce::LookAndFeel_V4 {
private:
	ButtonLookAndFeel buttonLookAndFeel;
	ComboBoxLookAndFeel comboBoxLookAndFeel;
	RotaryLookAndFeel rotaryLookAndFeel;
	ScrollBarLookAndFeel scrollBarLookAndFeel;
	TextEditorLookAndFeel textEditorLookAndFeel;
	TooltipWindowLookAndFeel tooltipWindowLookAndFeel;

public:
	MareverbLookAndFeel();
	~MareverbLookAndFeel() override = default;

	// Button
	juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;

	void drawButtonText(juce::Graphics& g, juce::TextButton& button,
		bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
		bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	void drawImageButton(juce::Graphics& g, juce::Image* image, int imageX, int imageY, int imageW, int imageH,
		const juce::Colour& overlayColour, float imageOpacity, juce::ImageButton& button) override;

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

	// Scrollbar
	void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height,
		bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;

	// TextEditor
	void fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override;

	void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override;

	// TooltipWindow
	void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override;
};