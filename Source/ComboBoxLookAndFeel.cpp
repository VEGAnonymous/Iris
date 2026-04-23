#include "ComboBoxLookAndFeel.h"

ComboBoxLookAndFeel::ComboBoxLookAndFeel() {}

void ComboBoxLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
    bool /*isButtonDown*/, int /*buttonX*/, int /*buttonY*/, int /*buttonWidth*/, int /*buttonH*/, juce::ComboBox& box) {
    const auto bounds = Bounds(0, 0, width, height).toFloat();
    const bool hovered = box.isMouseOver(); // ComboBox doesn't extend mouseEnter() or mouseExit() so no animation

    // Background
    const float cornerSize = 18.0f;
    g.setColour(Theme::Colors::background);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Border
    // g.setColour(hovered ? Theme::Colors::outlineHover : Theme::Colors::outline);
    // g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.5f);

    // Arrow
    const float arrowWidth = 7.0f;
    const float arrowHeight = 4.0f;
    const float arrowX = width - 20.0f;
    const float arrowY = (height - arrowHeight) * 0.5f;
    g.setColour(Theme::Colors::highlight.withAlpha(hovered ? 1.0f : 0.6f));
    g.drawLine(arrowX, arrowY, arrowX + (arrowWidth * 0.5f), arrowY + arrowHeight, 1.5f);
    g.drawLine(arrowX + (arrowWidth * 0.5f), arrowY + arrowHeight, arrowX + arrowWidth, arrowY, 1.5f);
}

juce::Font ComboBoxLookAndFeel::getComboBoxFont(juce::ComboBox& box) {
    return Theme::Fonts::getEquestriaNeueFont(juce::FontOptions().withHeight(static_cast<float>(box.getHeight()) * 0.45f));
}

void ComboBoxLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label) {
    label.setBounds(12, 0, box.getWidth() - 24, box.getHeight());
    label.setFont(getComboBoxFont(box));
    label.setColour(juce::Label::ColourIds::textColourId, Theme::Colors::textLight);
}

void ComboBoxLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height) {
    g.setColour(Theme::Colors::background);
    g.fillRect(0, 0, width, height);
}

void ComboBoxLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
    bool isSeparator, bool /*isActive*/, bool isHighlighted, bool isTicked, bool /*hasSubMenu*/,
    const juce::String& text, const juce::String& /*shortcutKeyText*/,
    const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/) {

    auto& bounds = area;

    // Separator
    if (isSeparator) {
        g.setColour(Theme::Colors::outline);
        g.fillRect(bounds.reduced(8, 0).withHeight(1).withY(bounds.getCentreY()));
        return;
    }

    // Highlight row
    if (isHighlighted) {
        g.setColour(Theme::Colors::highlight.withAlpha(0.15f));
        g.fillRoundedRectangle(bounds.reduced(2, 1).toFloat(), 6.0f);
    }

    // Dot (tick)
    if (isTicked) {
        const float dotX = static_cast<float>(bounds.getX()) + 10.0f;
        const float dotY = static_cast<float>(bounds.getCentreY());
        const float dotRadius = 2.0f;
        g.setColour(Theme::Colors::highlight);
        g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }

    // Text
    const juce::Colour textColor = isHighlighted ? Theme::Colors::textLight.brighter(0.2f)
        : (isTicked ? Theme::Colors::highlight : Theme::Colors::textLight);
    g.setColour(textColor);
    g.setFont(getPopupMenuFont());
    g.drawFittedText(text, bounds.withLeft(bounds.getX() + 20).withRight(bounds.getRight() - 8), juce::Justification::centredLeft, 1);
}

juce::Font ComboBoxLookAndFeel::getPopupMenuFont() {
    return Theme::Fonts::getEquestriaNeueFont(juce::FontOptions().withHeight(13.0f));
}