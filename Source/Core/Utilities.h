#pragma once

#include "Core/Defines.h"

/* VALIDATION */

inline bool validateIRIndex(int irIndex) { return irIndex >= 0 && irIndex < MAX_IR_COUNT; }
inline bool validateSwapInterval(float minTime, float maxTime) { return minTime >= SWAP_INTERVAL_MIN && 
																	    maxTime <= SWAP_INTERVAL_MAX && 
                                                                        maxTime > SWAP_INTERVAL_MIN &&
																		maxTime > minTime; }

/* FORMATTING */

namespace Format {
    // digits + 1 for the decimal point
    inline juce::String dimensionless(float value, int digits = 4) {
        return juce::String(value).substring(0, digits + 1);
    }

    inline juce::String percent(float value, int digits = 4) {
        return juce::String(value * 100.0f).substring(0, digits + 1) + "%";
    }

    inline juce::String frequency(float value, int digits = 4) {
        if (abs(value) >= 1000.0f)
             return juce::String(value / 1000.0f).substring(0, digits + 1) + " kHz";
        else return juce::String(value).substring(0, digits + 1) + " Hz";
    }

    inline juce::String seconds(float value, int digits = 4) {
        if (abs(value) < 1.0f)
             return juce::String(value * 1000.0f).substring(0, digits + 1) + " ms";
        else return juce::String(value).substring(0, digits + 1) + " s";
    }
}

/* MATH */

template <typename T>
inline int sgn(T x) { return (T(0) < x) - (x < T(0)); }

inline float randFloat() { return juce::Random::getSystemRandom().nextFloat(); } // [0, 1)
inline float randSigned() { return (randFloat() * 2.0f) - 1.0f; } // [-1, 1]

inline CartesianCoordinate polarToCartesian(PolarCoordinate p) { return { p.r * cosf(p.theta), p.r * sinf(p.theta) }; }
inline PolarCoordinate cartesianToPolar(CartesianCoordinate c) { return { std::sqrtf((c.x * c.x) + (c.y * c.y)), std::atan2(c.y, c.x) }; }

inline float cartesianDistance(CartesianCoordinate c1, CartesianCoordinate c2) {
    float dx = c1.x - c2.x, dy = c1.y - c2.y;
    return std::sqrtf((dx * dx) + (dy * dy));
}

inline float wrapAngle(float angle) {
    const float TWO_PI = juce::MathConstants<float>::twoPi;
    if (angle >= 0.0f && angle <= TWO_PI) return angle;

    angle = std::fmod(angle, TWO_PI);
    if (angle < 0.0f) angle += TWO_PI;
    return angle;
}

/* DEBUGGING */

inline void profileTime(juce::String desc, std::chrono::steady_clock::time_point startTime) {
    desc; startTime; // """Use""" the parameters
    DBG(desc << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count() << " ms");
}