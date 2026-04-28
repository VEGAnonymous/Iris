#include "ValueTooltipClient.h"

/* PROTECTED */

void ValueTooltipClient::showValueTooltip() {
	if (onShowValueTooltip) onShowValueTooltip();
}

void ValueTooltipClient::updateValueTooltipText() {
	if (onUpdateValueTooltipText) onUpdateValueTooltipText(getValueTooltip());
}

void ValueTooltipClient::updateValueTooltipPosition(juce::Point<float> localPosition) {
	if (onUpdateValueTooltipPosition) onUpdateValueTooltipPosition(localPosition);
}

void ValueTooltipClient::hideValueTooltip() const {
	if (onHideValueTooltip) onHideValueTooltip();
}

juce::String ValueTooltipClient::textFromValue(double value) {
	if (formatTextFromValueFunction) return formatTextFromValueFunction(value);
	return Format::dimensionless(static_cast<float>(value), 4);
}

/* PUBLIC */

void ValueTooltipClient::setValueTooltip(juce::String nTooltip) { tooltip = nTooltip; }

void ValueTooltipClient::setValueTooltip(double nValue) { tooltip = textFromValue(nValue); }

juce::String ValueTooltipClient::getValueTooltip() { return tooltip; }

juce::String ValueTooltipClient::getValueTooltip(double nValue, bool setInternal) { 
	if (setInternal) {
		setValueTooltip(nValue);
		return tooltip;
	} else return textFromValue(nValue);
}