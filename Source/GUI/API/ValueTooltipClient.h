#pragma once

#include "Core/Utilities.h"
#include "GUI/API/ValueTooltipWindow.h"

#include <JuceHeader.h>

class ValueTooltipClient {
private:
	juce::String tooltip;

protected:
	void showValueTooltip();
	void updateValueTooltipText();
	void updateValueTooltipPosition(juce::Point<float> localPosition);
	void hideValueTooltip() const;
	virtual juce::String textFromValue(double value);

public:
	std::function<void()> onShowValueTooltip;
	std::function<void(const juce::String)> onUpdateValueTooltipText;
	std::function<void(juce::Point<float>)> onUpdateValueTooltipPosition;
	std::function<void()> onHideValueTooltip;

	std::function<juce::String(double)> formatTextFromValueFunction;

	void setValueTooltip(juce::String nTooltip);
	virtual void setValueTooltip(double nValue);
	virtual juce::String getValueTooltip();
	juce::String getValueTooltip(double nValue, bool setInternal = true);

	void bindValueTooltipCallbacks(ValueTooltipWindow& valueTooltip, juce::Component& parentComponent);
};