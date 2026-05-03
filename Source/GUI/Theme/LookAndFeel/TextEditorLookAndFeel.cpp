#include "GUI/Components/Controls/HoverableTextEditor.h"
#include "GUI/Theme/Theme.h"
#include "GUI/Theme/LookAndFeel/TextEditorLookAndFeel.h"

/* PUBLIC */

TextEditorLookAndFeel::TextEditorLookAndFeel() {}

void TextEditorLookAndFeel::fillTextEditorBackground(juce::Graphics& g, int /*width*/, int /*height*/, juce::TextEditor& /*textEditor*/) {
	g.fillAll(Theme::Colors::background);
}

void TextEditorLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) {
    float hoverAlpha = 0.0f;
    if (auto* hoverable = dynamic_cast<HoverableTextEditor*>(&textEditor)) hoverAlpha = hoverable->getHoverAlpha();

    auto baseColor = textEditor.isEnabled() ? Theme::Colors::outline : Theme::Colors::shadow;
    auto outlineColor = baseColor.interpolatedWith(Theme::Colors::outlineHover, hoverAlpha);
    g.setColour(outlineColor);

    g.drawRoundedRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 6.0f, 2.0f + hoverAlpha);
}