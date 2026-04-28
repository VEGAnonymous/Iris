
#include "ControlLabel.h"

/* PUBLIC */

ControlLabel::ControlLabel(const juce::String& name) {
	setText(name, juce::dontSendNotification);
	setJustificationType(juce::Justification::centred);
	setMinimumHorizontalScale(0.0f);
}

void ControlLabel::paint(juce::Graphics& g) {
	auto bounds = getLocalBounds();
	g.setColour(Theme::Colors::background);
	g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

	g.setFont(labelFont);
	g.setColour(Theme::Colors::textLight);
	g.drawFittedText(getText(), bounds.reduced(1), getJustificationType(), 1, getMinimumHorizontalScale());
}