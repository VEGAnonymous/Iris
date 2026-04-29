#include "GUI/Theme/LookAndFeel/ButtonLookAndFeel.h"
#include "GUI/Components/Controls/HoverableImageButton.h"
#include "GUI/Components/Controls/HoverableTextButton.h"
#include "GUI/Components/Controls/HoverableToggleButton.h"
#include "GUI/Theme/Theme.h"

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

void ButtonLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
    bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/) {
    const auto bounds = button.getLocalBounds().toFloat();

    juce::Colour baseColor;
    if (button.getToggleState()) baseColor = Theme::Colors::highlight;
    else baseColor = Theme::Colors::disabled;

    float hoverAlpha = 0.0f;
    if (auto* hoverable = dynamic_cast<HoverableToggleButton*>(&button)) hoverAlpha = hoverable->getHoverAlpha();

    // Fill
    const float cornerSize = 3.0f;
    auto toggleColor = baseColor.withAlpha(juce::jmap(hoverAlpha, 0.8f, 1.0f));
    g.setColour(toggleColor);
    g.fillRoundedRectangle(bounds, cornerSize);
}

void ButtonLookAndFeel::drawImageButton(juce::Graphics& g, juce::Image* image, int imageX, int imageY, int imageW, int imageH,
    const juce::Colour& overlayColour, float imageOpacity, juce::ImageButton& button) {

    juce::AffineTransform transform = juce::RectanglePlacement(juce::RectanglePlacement::stretchToFit)
        .getTransformToFit(image->getBounds().toFloat(), Bounds(imageX, imageY, imageW, imageH).toFloat());

    juce::Colour baseColor = overlayColour;

    float hoverAlpha = 0.0f;
    if (auto* hoverable = dynamic_cast<HoverableImageButton*>(&button)) hoverAlpha = hoverable->getHoverAlpha();

    auto overlayColor = baseColor.interpolatedWith(Theme::Colors::textLight, hoverAlpha);
    g.setOpacity(imageOpacity);
    g.setColour(overlayColor);
    g.drawImageTransformed(*image, transform, true);
}