#pragma once

#include "Defines.h"
#include "Theme.h"

#include <JuceHeader.h>

inline juce::String formatPath(juce::File path) { 
    auto parent = path.getParentDirectory();
    if (parent.isRoot()) return path.getFileName();
    else return parent.getFileName() + "/" + path.getFileName();
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