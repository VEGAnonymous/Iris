#pragma once

#include "Defines.h"

#include <JuceHeader.h>

static constexpr auto
    WAVEFORM_PREVIEW_POINTS = 126,
    WAVEFORM_POINTS = 621;

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

namespace Theme {
    namespace Colors {
        const auto
            main = juce::Colour::fromRGBA(147, 255, 219, 255);
    }
}

inline float getIRAlpha(bool occupied, bool active) { return occupied ? (active ? 1.0f : 0.25f) : 0.0f; }

namespace Paint {
    inline void irIndicator(juce::Graphics& g, CartesianCoordinate center, float radius, int irIndex, bool occupied, bool active, 
        float alpha = -1.0f, juce::Colour color = juce::Colours::transparentBlack) {

        alpha = (alpha >= 0.0f) ? alpha : getIRAlpha(occupied, active);
        color = (color != juce::Colours::transparentBlack) ? color : IR_SLOT_COLORS[irIndex];
        g.setColour(color.withAlpha(alpha));
        g.fillEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);
    }
}