#pragma once

#include "AnimatedAlpha.h"
#include "IRManager.h"

#include <JuceHeader.h>

class IRHeaderComponent : public juce::Component {
private:
	int currentIndex = 0;
	IRSlot currentIR {};
	juce::String currentPath = "";

	juce::TextButton clearButton { "Clear" };

	AnimatedAlpha indicatorActiveAnim;

	const float indicatorRadius = 5.0f;
	BoundsF getIndicatorBounds(Bounds bounds, const float radius) const;

	void mouseDown(const juce::MouseEvent& e) override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRHeaderComponent)

public:
	IRHeaderComponent(juce::AnimatorUpdater& updater);
	~IRHeaderComponent() override = default;

	std::function<void()> onClear;
	std::function<void(bool)> onActiveToggle;

	void setSlot(int irIndex, const IRSlot& slot);
	void setActive(bool nActive, bool animate = false);

	void paint(juce::Graphics& g) override;
	void resized() override;
};