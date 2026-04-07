#pragma once

#include "Defines.h"

PolarCoordinate computeDistanceDirection(PolarCoordinate p1, PolarCoordinate p2, Axis reference, bool computeAngle) {
    // TODO: Optimize with approximations
    float d = 0.0f, phi = 0.0f;
    float dTheta = p2.theta - p1.theta;
    d = sqrtf((p1.r * p1.r) + (p2.r * p2.r) - (2 * p1.r * p2.r * cosf(dTheta)));

    if (computeAngle) {
        if (reference == Axis::X_AXIS)
            phi = atan2f((p2.r * sinf(p2.theta)) - (p1.r * sinf(p1.theta)), (p2.r * cosf(p2.theta)) - (p1.r * cosf(p1.theta)));
        else // Y_AXIS
            phi = atan2f((p2.r * cosf(p2.theta)) - (p1.r * cosf(p1.theta)), (p2.r * sinf(p2.theta)) - (p1.r * sinf(p1.theta)));
    }

    return { d, phi };
}

float randFloat() { return juce::Random::getSystemRandom().nextFloat(); } // [0, 1)
float randSigned() { return (randFloat() * 2.0f) - 1.0f; } // [-1, 1]