#pragma once

#include "GUI/API/ValueTooltipWindow.h"
#include "GUI/Components/WindowOverlayComponent.h"
#include "GUI/Components/Base/WaveformComponent.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>

class IRDisplayComponent : public juce::Component {
private:
	MareverbAudioProcessor& audioProcessor;
	juce::AnimatorUpdater& animatorUpdater;

	WaveformComponent irWaveformComponent;
	WindowOverlayComponent windowOverlayComponent;

	void prepare();

public:
	IRDisplayComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater);
	~IRDisplayComponent() override = default;

	void paint(juce::Graphics& g) override;
	void resized() override;

	WaveformComponent* getWaveformComponent();
	WindowOverlayComponent* getWindowOverlayComponent();
};