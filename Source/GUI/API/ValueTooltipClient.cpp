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

void ValueTooltipClient::bindValueTooltipCallbacks(ValueTooltipWindow& valueTooltip, juce::Component& parentComponent) {
    if (auto* component = dynamic_cast<juce::Component*>(this)) {
        juce::Component* parentComponentPtr = &parentComponent;

        onShowValueTooltip = [&valueTooltip]() {
            DBG("Showing tooltip");
            valueTooltip.setVisible(true);
        };
        onUpdateValueTooltipText = [&valueTooltip](const juce::String tooltip) {
            DBG(tooltip);
            valueTooltip.setText(tooltip);
        };
        onUpdateValueTooltipPosition =
            [this, &valueTooltip, parentComponentPtr, component](juce::Point<float> position) {
            auto localPoint = parentComponentPtr->getLocalPoint(component, position.roundToInt());
            DBG("Local point: " << localPoint.x << ", " << localPoint.y);
            valueTooltip.updatePosition(getValueTooltip(), localPoint, parentComponentPtr->getLocalBounds());
        };
        onHideValueTooltip = [&valueTooltip]() {
            DBG("Hid tooltip");
            valueTooltip.setVisible(false);
        };
    }
}