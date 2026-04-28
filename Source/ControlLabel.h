#pragma once

#include "Control.h"
#include "Theme.h"

#include <JuceHeader.h>

class ControlLabel : public juce::Label {
private:
	const juce::Font labelFont = Theme::Fonts::getEquestriaNeueFont(juce::FontOptions().withHeight(12));

public:
	ControlLabel(const juce::String& name = "");
	~ControlLabel() override = default;

	void paint(juce::Graphics& g) override;
};