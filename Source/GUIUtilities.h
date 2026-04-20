#pragma once

#include "Defines.h"
#include "Theme.h"

#include <JuceHeader.h>

enum class Ellipsis { FRONT, BACK, MIDDLE };
inline juce::String formatPath(juce::String path, int maxLength, Ellipsis ellipsis = Ellipsis::FRONT) {
    if (path.length() > maxLength) {
        switch (ellipsis) {
            case Ellipsis::FRONT: return "..." + path.substring(path.length() - (maxLength - 3));
            case Ellipsis::BACK: return path.substring(0, maxLength - 3) + "...";
            case Ellipsis::MIDDLE: {
                const int front = (maxLength - 3) / 2;
                const int back = (maxLength - 3) - front;
                return path.substring(0, front) + "..." + path.substring(path.length() - back);
            }
            default: return path;
        }
    } else return path;
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