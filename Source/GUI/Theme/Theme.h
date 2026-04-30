#pragma once

#include "Core/Defines.h"
#include "GUI/Theme/MareverbFonts.h"
#include "GUI/Theme/MareverbIcons.h"

#include <JuceHeader.h>

namespace Theme {
    namespace Colors {
        const juce::Colour
            background = juce::Colour::fromRGB(22, 31, 38),
            section = juce::Colour::fromRGB(36, 45, 54),
            outline = juce::Colour::fromRGB(58, 70, 82),
            outlineHover = juce::Colour::fromRGB(68, 82, 96),
            highlight = juce::Colour::fromRGB(147, 255, 219),
            highlightHover = juce::Colour::fromRGB(156, 255, 255),
            shadow = juce::Colour::fromRGB(16, 25, 32),
            disabled = juce::Colour::fromRGB(82, 94, 100),
            textLight = juce::Colour::fromRGB(196, 196, 196),
            textDark = juce::Colour::fromRGB(52, 64, 76);
    }

    namespace Fonts {
        inline const juce::Font getEquestriaNeueFont(juce::FontOptions options = juce::FontOptions()) {
            static auto typeface = juce::Typeface::createSystemTypefaceFor(
                MareverbFonts::Equestria_Neue_ttf,
                MareverbFonts::Equestria_Neue_ttfSize
            ); jassert(typeface);
            auto fontOptions = options.withStyle("").withTypeface(typeface);
            return juce::Font(juce::Font(fontOptions));
        }
        inline const juce::Font getEquestriaBoldFont(juce::FontOptions options = juce::FontOptions()) {
            static auto typeface = juce::Typeface::createSystemTypefaceFor(
                MareverbFonts::Equestria_Bold_otf,
                MareverbFonts::Equestria_Bold_otfSize
            ); jassert(typeface);
            auto fontOptions = options.withStyle("").withTypeface(typeface);
            return juce::Font(juce::Font(fontOptions));
        }
    }

    namespace Icons {
        inline const juce::Image getLogo() {
            return juce::ImageCache::getFromMemory(MareverbIcons::LYRAnonymous_png, MareverbIcons::LYRAnonymous_pngSize);
        }
        inline const juce::Image getBurgerMenuIcon() {
            return juce::ImageCache::getFromMemory(MareverbIcons::Menu_png, MareverbIcons::Menu_pngSize);
        }
        inline const juce::Image getRefreshIcon() {
            return juce::ImageCache::getFromMemory(MareverbIcons::Refresh_png, MareverbIcons::Refresh_pngSize);
        }
    }
}

static const std::array<juce::Colour, MAX_IR_COUNT> IR_SLOT_COLORS {
    juce::Colours::red,
    juce::Colours::orange,
    juce::Colours::yellow,
    juce::Colours::green,
    juce::Colours::cyan,
    juce::Colours::blue,
    juce::Colours::purple,
    juce::Colours::hotpink
};