#include "GUI/Theme/Theme.h"
#include "GUI/Theme/LookAndFeel/TooltipWindowLookAndFeel.h"

/* PUBLIC */

TooltipWindowLookAndFeel::TooltipWindowLookAndFeel() {}

void TooltipWindowLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) {
    Bounds bounds { width, height };
    g.setColour(Theme::Colors::shadow);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    juce::AttributedString s;
    s.setWordWrap(juce::AttributedString::WordWrap::byChar);
    s.setJustification(juce::Justification::centred);
    s.append(
        text, 
        Theme::Fonts::getEquestriaNeueFont(juce::FontOptions(12.5f).withMetricsKind(getDefaultMetricsKind())), 
        Theme::Colors::textLight
    );

    juce::TextLayout textLayout;
    textLayout.createLayoutWithBalancedLineLengths(s, 321.0f);
    textLayout.draw(g, { static_cast<float>(width), static_cast<float>(height) });
}