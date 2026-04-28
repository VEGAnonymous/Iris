#pragma once

#include "Core/Control/IRManager.h"
#include "GUI/API/AnimatedAlpha.h"
#include "GUI/Components/Controls/HoverableTextButton.h"
#include "GUI/Theme/LookAndFeel/ButtonLookAndFeel.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>

class IRHeaderComponent : public juce::Component {
private:
	MareverbAudioProcessor& audioProcessor;

	int currentIndex = 0;
	IRSlot currentIR {};
	juce::String currentPath = "";

	AnimatedAlpha indicatorActiveAnim;

	const float indicatorRadius = 5.0f;
	BoundsF getIndicatorBounds(Bounds bounds, const float radius) const;

	void mouseDown(const juce::MouseEvent& e) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRHeaderComponent)

public:
	IRHeaderComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater);
	~IRHeaderComponent() override = default;

	void setSlot(int irIndex, const IRSlot& slot);
	void setActive(bool nActive, bool animate = false);

	void paint(juce::Graphics& g) override;
};