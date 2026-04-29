#pragma once

#include "GUI/Components/Controls/HoverableTextButton.h"
#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Components/Controls/Rotary.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>
#include <array>

class PositionFieldControlsPanel : public juce::Component {
public:
	struct ControlTab {
		juce::ComboBox* pattern = nullptr;
		std::vector<LabelledControl<Rotary>*> rotaries {};
	};

	enum ControlTabID { POSITION = 0, FIELD = 1 };

private:
	MareverbAudioProcessor& audioProcessor;
	juce::AnimatorUpdater& animatorUpdater;

	std::array<ControlTab, 2> controlTabs;
	ControlTabID activeTab = ControlTabID::POSITION;

	HoverableTextButton positionTabButton {animatorUpdater}, fieldTabButton {animatorUpdater};

	void prepare();

public:
	PositionFieldControlsPanel(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater, 
		std::array<ControlTab, 2> tabs);
	~PositionFieldControlsPanel() override = default;

	void paint(juce::Graphics& g) override;
	void resized() override;
};