#pragma once

#include "Defines.h"

inline bool validateIRIndex(int irIndex) { return irIndex >= 0 && irIndex < MAX_IR_COUNT; }
inline bool validateSwapInterval(float minTime, float maxTime) { return minTime >= SWAP_INTERVAL_MIN && 
																	    maxTime <= SWAP_INTERVAL_MAX && 
                                                                        maxTime > SWAP_INTERVAL_MIN &&
																		maxTime > minTime; }

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

template <typename T> 
inline int sgn(T x) { return (T(0) < x) - (x < T(0)); }