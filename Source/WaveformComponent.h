#pragma once

#include "AnimatedAlpha.h"
#include "Defines.h"

#include <JuceHeader.h>

static constexpr auto THUMBNAIL_CACHE_SIZE = MAX_IR_COUNT + 1; // 8 IRs + currently selected

class WaveformComponent : public juce::Component {
private:
	float offsetX = 0.0f, offsetY = 0.0f, offsetWidth = 0.0f, heightScale = 1.0f;
	int numPoints = 0;
	juce::Colour color = juce::Colours::white;
	bool active = true;

	AnimatedAlpha activeAnim;

	juce::AudioThumbnailCache thumbnailCache { THUMBNAIL_CACHE_SIZE };
	juce::AudioThumbnail thumbnail { 126, formatManager, thumbnailCache };
	juce::AudioFormatManager formatManager;

	void paint(juce::Graphics& g) override;

public:
	WaveformComponent(juce::AnimatorUpdater& updater);
	~WaveformComponent() override = default;

	void setNumPoints(int nPoints);
	void setWaveform(const juce::AudioBuffer<float>* buffer, double sampleRate);
	void setDimensions(float nOffsetX = 0.0f, float nOffsetY = 0.0f, float nOffsetWidth = 0.0f, float nHeightScale = 1.0f);
	void setColor(juce::Colour nColor = juce::Colours::white);
	void setActive(bool nActive, bool animate = false);
};