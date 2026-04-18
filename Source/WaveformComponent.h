#pragma once

#include "Defines.h"

#include <JuceHeader.h>

class WaveformComponent : public juce::Component {
private:
	float offsetX = 0.0f, offsetY = 0.0f, offsetWidth = 0.0f, widthScale = 1.0f, heightScale = 1.0f;
	juce::Colour color = juce::Colours::white;
	bool active = true;

	std::array<std::vector<float>, N_CHANNELS> waveformPoints;

	void paint(juce::Graphics& g) override;

public:
	WaveformComponent();
	~WaveformComponent() override = default;

	void setWaveform(const juce::AudioBuffer<float>* buffer, const int numPoints = 126);
	void setDimensions(float nOffsetX = 0.0f, float nOffsetY = 0.0f, float nOffsetWidth = 0.0f, float nHeightScale = 1.0f);
	void setColor(juce::Colour nColor = juce::Colours::white);
	void setActive(bool nActive);
};