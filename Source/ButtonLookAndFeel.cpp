#include "ButtonLookAndFeel.h"
#include "HoverableTextButton.h"
#include "Theme.h"

/* PUBLIC */

ButtonLookAndFeel::ButtonLookAndFeel() {}

juce::Font ButtonLookAndFeel::getTextButtonFont(juce::TextButton& button, int /*buttonHeight*/) {
    auto options = juce::FontOptions().withHeight(button.getHeight() * 0.4f);
	return Theme::Fonts::getEquestriaNeueFont(options);
}

void ButtonLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button, 
    bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/) {
    auto bounds = button.getLocalBounds();

    juce::Colour baseColor;
    if (!button.isToggleable() || button.getToggleState()) baseColor = Theme::Colors::highlight;
    else baseColor = Theme::Colors::textDark;

    float hoverAlpha = 0.0f;
    if (auto* hoverable = dynamic_cast<HoverableTextButton*>(&button)) hoverAlpha = hoverable->getHoverAlpha();

    auto textColor = baseColor.interpolatedWith(Theme::Colors::textLight, hoverAlpha);
    g.setColour(textColor);
    g.setFont(getTextButtonFont(button, bounds.getHeight()));
    g.drawFittedText(button.getButtonText(), bounds, juce::Justification::centred, 1);
}