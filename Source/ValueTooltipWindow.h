#pragma once

#include "Theme.h"

#include <JuceHeader.h>

class ValueTooltipWindow : public juce::Component {
private:
    juce::String text;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ValueTooltipWindow)

public:
    ValueTooltipWindow();

    void setText(const juce::String nText);

    void updatePosition(const juce::String& tooltip, juce::Point<int> position, juce::Rectangle<int> parentBounds);

    void paint(juce::Graphics& g) override;
};