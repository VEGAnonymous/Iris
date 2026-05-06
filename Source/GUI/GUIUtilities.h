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

// IR indicators
inline float getIRIndicatorAlpha(bool occupied, bool active) { return occupied ? (active ? 1.0f : 0.25f) : 0.0f; }

inline void updateFieldIndicatorStyle(juce::Image& fieldIndicatorIcon, const juce::Image& iconToTry, const juce::String& fieldIndicatorStyle) {
    const juce::Image fallbackFieldIcon = Theme::Mares::getAnonfilly();
    // Update field indicator style
    juce::Image fieldIcon{};
    if (fieldIndicatorStyle != FieldIndicatorStyle::Mareless) {
        if (fieldIndicatorStyle == FieldIndicatorStyle::Half_Mared) {
            auto& icon = iconToTry;
            if (!icon.isNull()) fieldIcon = icon;
        }
        else if (fieldIndicatorStyle == FieldIndicatorStyle::Mareful) {
            auto& icon = iconToTry;
            fieldIcon = icon.isNull() ? fallbackFieldIcon : icon;
        }
    }
    fieldIndicatorIcon = fieldIcon;
}

namespace Paint {
    inline void irIndicator(juce::Graphics& g, CartesianCoordinate center, float radius,
        int irIndex, bool occupied, bool active, bool selected,
        float indicatorAlpha = -1.0f, float selectionAlpha = -1.0f, juce::Colour color = juce::Colours::transparentBlack, 
        int glowPasses = 0, float glowStrength = 1.0f,
        juce::Image* mare = nullptr) {

        color = (color != juce::Colours::transparentBlack) ? color : Theme::Colors::irSlotColours[irIndex];
        indicatorAlpha = (indicatorAlpha >= 0.0f) ? indicatorAlpha : getIRIndicatorAlpha(occupied, active);
        selectionAlpha = (selectionAlpha >= 0.0f) ? selectionAlpha : (selected ? 1.0f : 0.0f);

        // Glow
        const float innerRadius = radius * 1.62f;
        const float innerAlpha = 0.0621f + (glowStrength * 0.126f);
        const float outerRadius = radius * 6.21f;
        const float outerAlpha = 0.0216f + (glowStrength * 0.126f);

        for (int i = 0; i < glowPasses; ++i) {
            const float glowRadius = innerRadius + (i * ((outerRadius - innerRadius) / glowPasses));
            const float glowAlpha = juce::jlimit(0.0f, 1.0f,
                indicatorAlpha
                * (innerAlpha - (i * ((innerAlpha - outerAlpha) / glowPasses))) 
                + (indicatorAlpha * selectionAlpha * 0.0621f)
            );

            juce::ColourGradient glowGradient(
                color.withAlpha(glowAlpha * 0.8f), center.x, center.y,
                color.withAlpha(0.0f), center.x + glowRadius, center.y,
                true
            );
            g.setGradientFill(glowGradient);
            g.fillEllipse(center.x - glowRadius, center.y - glowRadius, glowRadius * 2.0f, glowRadius * 2.0f);
        }

        // Mare indicator
        if (mare && !mare->isNull()) {
            radius += 4.0f;
            g.setOpacity(indicatorAlpha);
            g.drawImage(*mare, 
                static_cast<int>(center.x - radius), static_cast<int>(center.y - radius), 
                static_cast<int>(radius * 2.0f), static_cast<int>(radius * 2.0f),
                0, 0, mare->getWidth(), mare->getHeight()
            );
        } else {
            g.setColour(color.withAlpha(indicatorAlpha));
            g.fillEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);
        }

        // Selection ring
        if (selected) {
            radius *= 1.62f;
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