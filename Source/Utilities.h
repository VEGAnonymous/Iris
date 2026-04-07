#pragma once

#include "Defines.h"

inline Settings getSettings(juce::AudioProcessorValueTreeState& parameters) {
    Settings settings;

    settings.globalMix = parameters.getRawParameterValue("Global Mix")->load();
    settings.decay = parameters.getRawParameterValue("Decay")->load();
    settings.motionPattern = static_cast<MotionPattern>(parameters.getRawParameterValue("Motion Pattern")->load());
    settings.motionRate = parameters.getRawParameterValue("Motion Rate")->load();
    settings.motionModA = parameters.getRawParameterValue("Motion Mod A")->load();
    settings.motionModB = parameters.getRawParameterValue("Motion Mod B")->load();

    return settings;
}

// Random

inline float randFloat() { return juce::Random::getSystemRandom().nextFloat(); } // [0, 1)
inline float randSigned() { return (randFloat() * 2.0f) - 1.0f; } // [-1, 1]

// Position motion patterns

inline CartesianCoordinate polarToCartesian(PolarCoordinate p) { return { p.r * cosf(p.theta), p.r * sinf(p.theta) }; }
inline PolarCoordinate cartesianToPolar(CartesianCoordinate p) { return { std::sqrt((p.x * p.x) + (p.y * p.y)), std::atan2(p.y, p.x) }; }