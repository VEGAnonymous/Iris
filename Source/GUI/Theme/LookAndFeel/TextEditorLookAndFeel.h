#pragma once

#include <JuceHeader.h>

class TextEditorLookAndFeel : public juce::LookAndFeel_V4 {
private:

public:
	TextEditorLookAndFeel();
	~TextEditorLookAndFeel() override = default;

	void fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override;
	void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor) override;
};