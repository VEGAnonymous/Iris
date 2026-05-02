#pragma once

#include "Core/Defines.h"
#include "GUI/Theme/MareverbAssets.h"
#include "GUI/Theme/MareverbFonts.h"
#include "GUI/Theme/MareverbMaresMain.h"

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

        static const std::array<juce::Colour, MAX_IR_COUNT> irSlotColours{
            juce::Colour::fromRGB(104, 56, 140),  // Twiggles
            juce::Colour::fromRGB(253, 188, 95),  // Appul
            juce::Colour::fromRGB(238, 231, 243), // Rarara
            juce::Colour::fromRGB(255, 253, 135), // Shyhorse
            juce::Colour::fromRGB(135, 210, 248), // plap plap
            juce::Colour::fromRGB(237, 138, 188), // Plenka Po
            juce::Colour::fromRGB(34, 74, 191),   // Excites me so
            juce::Colour::fromRGB(96, 216, 169)   // Sunbutt
        };
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
            return juce::ImageCache::getFromMemory(MareverbAssets::LYRAnonymous_png, MareverbAssets::LYRAnonymous_pngSize);
        }
        inline const juce::Image getBurgerMenuIcon() {
            return juce::ImageCache::getFromMemory(MareverbAssets::Menu_png, MareverbAssets::Menu_pngSize);
        }
        inline const juce::Image getRefreshIcon() {
            return juce::ImageCache::getFromMemory(MareverbAssets::Refresh_png, MareverbAssets::Refresh_pngSize);
        }
        inline juce::Image getAnon() {
            return juce::ImageCache::getFromMemory(MareverbAssets::anon_gif, MareverbAssets::anon_gifSize);
        }
    }

    namespace Images {
        inline const juce::Image getDerpyHooves() {
            return juce::ImageCache::getFromMemory(MareverbAssets::Derpy_Hooves_png, MareverbAssets::Derpy_Hooves_pngSize);
        }
    }

    namespace Mares {
        inline juce::Image getMare(juce::String mareToFind) {
            auto normalize = [](juce::String string) -> juce::String {
                string = string.toLowerCase();

                juce::String sanitizedString;
                sanitizedString.preallocateBytes(string.length());

                for (int i = 0; i < string.length(); ++i) {
                    juce::juce_wchar character = string[i];

                    // Sanitize characters
                    if (character == '-' || character == '_' || character == ' ')
                        sanitizedString += '-';
                    else if ((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9'))
                        sanitizedString += character;
                }

                // Collapse consecutive '-'
                juce::String result;
                result.preallocateBytes(sanitizedString.length());

                bool lastWasSeparator = false;
                for (int i = 0; i < sanitizedString.length(); ++i) {
                    bool isSeparator = (sanitizedString[i] == '-');
                    if (isSeparator && lastWasSeparator) continue;
                    result += sanitizedString[i];
                    lastWasSeparator = isSeparator;
                }

                return result;
            };

            mareToFind = normalize(mareToFind);

            const int numMares = MareverbMaresMain::namedResourceListSize;

            // Match by ranking: earlier index + longer match wins
            // Index e.g, '00_00_14_Pinkie_Anxious__I can't wait another minute to find out if rainbow dash got in or not!' <- Pinkie
            // Length e.g., 'Twilight...', 'Twilight Velvet...' <- Twilight Velvet
            int bestIndex = -1;
            int bestPosition = INT_MAX;
            int bestLength = -1;

            for (int i = 0; i < numMares; ++i) {
                juce::String mareName = juce::String::fromUTF8(MareverbMaresMain::originalFilenames[i]).upToLastOccurrenceOf(".", false, false);
                mareName = normalize(mareName);
                if (mareName.isEmpty()) continue;

                int position = mareToFind.indexOf(mareName);
                if (position == -1) continue;

                int length = mareName.length();
                bool betterMatch = (position < bestPosition)
                    || ((position == bestPosition) && length > bestLength);

                if (betterMatch) {
                    bestIndex = i;
                    bestPosition = position;
                    bestLength = length;
                }
            }

            if (bestIndex >= 0) { // winrar!
                int mareSize = 0;
                const char* resourceName = MareverbMaresMain::namedResourceList[bestIndex];
                auto mare = MareverbMaresMain::getNamedResource(resourceName, mareSize);
                return juce::ImageCache::getFromMemory(mare, mareSize);
            }

            return juce::Image();
        }
    }
}