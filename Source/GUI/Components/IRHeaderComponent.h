#pragma once

#include "Core/Control/IRManager.h"
#include "GUI/API/AnimatedValue.h"
#include "GUI/Components/Controls/HoverableTextButton.h"
#include "GUI/Theme/LookAndFeel/ButtonLookAndFeel.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>

class IRHeaderComponent : public juce::Component, public juce::SettableTooltipClient {
private:
	MareverbAudioProcessor& audioProcessor;

	int currentIndex = 0;
	IRSlotLite currentIR {};
	juce::String currentPath = "";
	juce::Image currentIndicator {};

	juce::String indicatorStyle;

	AnimatedValue indicatorActiveAnim;

	const float indicatorRadius = 5.0f;
	BoundsF getIndicatorBounds(Bounds bounds, const float radius) const;

	void mouseMove(const juce::MouseEvent& e) override;
	void mouseDown(const juce::MouseEvent& e) override;
	void mouseExit(const juce::MouseEvent& e) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRHeaderComponent)

public:
	IRHeaderComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater);
	~IRHeaderComponent() override = default;

	void setSlot(int irIndex, const IRSlotLite slot);
	void setActive(bool nActive, bool animate = false);
	void setIndicatorStyle(const juce::String nStyle);

	void updateIndicator();

	void paint(juce::Graphics& g) override;
};