#pragma once

#include "Defines.h"

#include <JuceHeader.h>

namespace Theme {
    namespace Colors {
        const juce::Colour
            background = juce::Colour::fromRGB(22, 31, 38),
            section = juce::Colour::fromRGB(36, 45, 54),
            outline = juce::Colour::fromRGB(52, 64, 76),
            outlineHover = juce::Colour::fromRGB(62, 76, 90),
            highlight = juce::Colour::fromRGB(147, 255, 219),
            highlightHover = juce::Colour::fromRGB(156, 255, 255);
    }
}

static const std::array<juce::Colour, MAX_IR_COUNT> IR_SLOT_COLORS{
    juce::Colours::red,
    juce::Colours::orange,
    juce::Colours::yellow,
    juce::Colours::green,
    juce::Colours::cyan,
    juce::Colours::blue,
    juce::Colours::purple,
    juce::Colours::hotpink
};