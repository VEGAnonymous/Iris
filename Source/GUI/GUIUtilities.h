#pragma once

#include "Core/Defines.h"
#include "GUI/Theme/Theme.h"

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

inline float getIRIndicatorAlpha(bool occupied, bool active) { return occupied ? (active ? 1.0f : 0.25f) : 0.0f; }

namespace Paint {
    inline void irIndicator(juce::Graphics& g, CartesianCoordinate center, float radius, 
        int irIndex, bool occupied, bool active, bool selected,
        float indicatorAlpha = -1.0f, float selectionAlpha = -1.0f, 
        juce::Colour color = juce::Colours::transparentBlack) {

        color = (color != juce::Colours::transparentBlack) ? color : IR_SLOT_COLORS[irIndex];
        indicatorAlpha = (indicatorAlpha >= 0.0f) ? indicatorAlpha : getIRIndicatorAlpha(occupied, active);
        g.setColour(color.withAlpha(indicatorAlpha));
        g.fillEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);

        if (selected) {
            radius *= 1.62f;
            selectionAlpha = (selectionAlpha >= 0.0f) ? selectionAlpha : (selected ? 1.0f : 0.0f);
            selectionAlpha *= indicatorAlpha;
            g.setColour(color.withAlpha(selectionAlpha));
            g.drawEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f, 1.26f);
        }
    }

    inline void dragAndDropHover(juce::Graphics& g, BoundsF bounds,
        float alpha = -1.0f, juce::Colour color = Theme::Colors::highlight.withAlpha(0.5f)) {

        alpha = (alpha >= 0.0f) ? alpha : 1.0f;
        g.setColour(color.withMultipliedAlpha(alpha));
        g.drawRoundedRectangle(bounds, 4.0f, 3.0f);
    }
}