#pragma once

#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Components/Controls/Rotary.h"

#include <JuceHeader.h>
#include <vector>

class GlobalControlsPanel : public juce::Component {
public:
	struct GlobalRow {
		std::vector<LabelledControl<Rotary>*> rotaries {};
	};

private:
	GlobalRow globalControls {};

	void prepare();

public:
	GlobalControlsPanel(GlobalRow controls);
	~GlobalControlsPanel() override = default;

	void paint(juce::Graphics& g) override;
	void resized() override;
};