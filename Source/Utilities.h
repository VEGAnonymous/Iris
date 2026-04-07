#pragma once

#include "Defines.h"

inline float randFloat() { return juce::Random::getSystemRandom().nextFloat(); } // [0, 1)
inline float randSigned() { return (randFloat() * 2.0f) - 1.0f; } // [-1, 1]