#pragma once

#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Components/Controls/Rotary.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>
#include <vector>

class InteractionControlsPanel : public juce::Component {
public:
	struct InteractionRow {
		LabelledControl<juce::ComboBox>* weightingControl = nullptr;
		std::vector<LabelledControl<Rotary>*> rotaries {};
	};
private:
	MareverbAudioProcessor& audioProcessor;

	InteractionRow interactionControls {};

	void prepare();

public:
	InteractionControlsPanel(MareverbAudioProcessor& processor, InteractionRow controls);
	~InteractionControlsPanel() override = default;

	void paint(juce::Graphics& g) override;
	void resized() override;
};