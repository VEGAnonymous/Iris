#include "ValueTooltipWindow.h"

/* PUBLIC */

ValueTooltipWindow::ValueTooltipWindow() {
    setAlwaysOnTop(true);
    setOpaque(true);
    setAccessible(false);
    setInterceptsMouseClicks(false, false);
}

void ValueTooltipWindow::setText(const juce::String nText) {
    if (text != nText) {
        text = nText;
        repaint();
    }
}

void ValueTooltipWindow::updatePosition(const juce::String& tooltip, juce::Point<int> position, Bounds parentBounds) {
    setBounds(getLookAndFeel().getTooltipBounds(tooltip, position, parentBounds).expanded(2, 1));
}

void ValueTooltipWindow::paint(juce::Graphics& g) {
    g.setColour(Theme::Colors::shadow);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

    g.setColour(Theme::Colors::textLight);
    g.setFont(Theme::Fonts::getEquestriaNeueFont(juce::FontOptions(12.0f)));
    g.drawText(text, getLocalBounds(), juce::Justification::centred, false);
}