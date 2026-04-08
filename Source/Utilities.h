#pragma once

#include "Defines.h"

// Random

inline float randFloat() { return juce::Random::getSystemRandom().nextFloat(); } // [0, 1)
inline float randSigned() { return (randFloat() * 2.0f) - 1.0f; } // [-1, 1]

// Position motion patterns

inline CartesianCoordinate polarToCartesian(PolarCoordinate p) { return { p.r * cosf(p.theta), p.r * sinf(p.theta) }; }
inline PolarCoordinate cartesianToPolar(CartesianCoordinate p) { return { std::sqrt((p.x * p.x) + (p.y * p.y)), std::atan2(p.y, p.x) }; }