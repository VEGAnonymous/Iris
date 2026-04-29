#pragma once

#include "GUI/Components/Controls/HoverableTextButton.h"
#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Components/Controls/Rotary.h"

#include <JuceHeader.h>
#include <vector>

class InteractionControlsPanel : public juce::Component {
public:
	struct InteractionRow {
		LabelledControl<HoverableTextButton>* weightingControl = nullptr;
		std::vector<LabelledControl<Rotary>*> rotaries {};
	};
private:
	InteractionRow interactionControls {};

	void prepare();

public:
	InteractionControlsPanel(InteractionRow controls);
	~InteractionControlsPanel() override = default;

	void paint(juce::Graphics& g) override;
	void resized() override;
};