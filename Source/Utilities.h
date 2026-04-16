#pragma once

#include "Defines.h"

inline bool validateIRIndex(int irIndex) { return irIndex >= 0 && irIndex < MAX_IR_COUNT; }
inline bool validateSwapInterval(float minTime, float maxTime) { return minTime > SWAP_INTERVAL_MIN && 
																	    maxTime <= SWAP_INTERVAL_MAX && 
																		maxTime > minTime; }

inline float randFloat() { return juce::Random::getSystemRandom().nextFloat(); } // [0, 1)
inline float randSigned() { return (randFloat() * 2.0f) - 1.0f; } // [-1, 1]

inline CartesianCoordinate polarToCartesian(PolarCoordinate p) { return { p.r * cosf(p.theta), p.r * sinf(p.theta) }; }
inline PolarCoordinate cartesianToPolar(CartesianCoordinate p) { return { std::sqrt((p.x * p.x) + (p.y * p.y)), std::atan2(p.y, p.x) }; }

inline float wrapAngle(float angle) {
    const float TWO_PI = juce::MathConstants<float>::twoPi;
    if (angle >= 0.0f && angle <= TWO_PI) return angle;

    angle = std::fmod(angle, TWO_PI);
    if (angle < 0.0f) angle += TWO_PI;
    return angle;
}