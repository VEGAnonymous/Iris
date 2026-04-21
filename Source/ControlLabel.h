#pragma once

#include "Control.h"
#include "Theme.h"

#include <JuceHeader.h>

class ControlLabel : public juce::Label {
private:
	const juce::Font labelFont = Theme::Fonts::getEquestriaNeueFont(juce::FontOptions().withHeight(12));

public:
	ControlLabel(const juce::String& name) {
		setText(name, juce::dontSendNotification);
		setJustificationType(juce::Justification::centred);
		setMinimumHorizontalScale(0.0f);
	}

	void paint(juce::Graphics& g) override {
		auto bounds = getLocalBounds();
		g.setColour(Theme::Colors::background);
		g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

		g.setFont(labelFont);
		g.setColour(Theme::Colors::textLight);
		g.drawFittedText(getText(), bounds.reduced(1), getJustificationType(), 1, getMinimumHorizontalScale());
	}
};