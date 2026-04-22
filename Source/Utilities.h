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
inline PolarCoordinate cartesianToPolar(CartesianCoordinate p) { return { std::sqrt((p.x * p.x) + (p.y * p.y)), std::atan2(p.y, p.x) }; }

inline float wrapAngle(float angle) {
    const float TWO_PI = juce::MathConstants<float>::twoPi;
    if (angle >= 0.0f && angle <= TWO_PI) return angle;

    angle = std::fmod(angle, TWO_PI);
    if (angle < 0.0f) angle += TWO_PI;
    return angle;
}

inline float getEnvelopeValue(float t, EnvelopeType type) {
    auto& PI = juce::MathConstants<float>::pi;
    switch (type) {
        case EnvelopeType::NONE: return 1.0f;
        case EnvelopeType::HANN: return 0.5f * (1.0f - cosf(2.0f * PI * t));
        case EnvelopeType::HAMMING: return 0.54f - (0.46f * cosf(2.0f * PI * t));
        case EnvelopeType::SINE: return sinf(PI * t);
        case EnvelopeType::TRI: return 1.0f - fabsf(2.0f * t - 1.0f);
        case EnvelopeType::PERC: {
            constexpr float attack = 0.0f; constexpr float decay = 5.0f;
            if (t < attack) {
                return t / std::max(attack, EPSILON); // Linear attack
            }
            else { // Exponential decay
                float t_decay = (t - attack) / (1.0f - attack);
                return expf(-decay * t_decay);
            }
        }
        case EnvelopeType::SMOOTH_RECT: {
            constexpr float smooth = 0.05f;
            if (t < smooth) { // Fade in
                return 0.5f * (1.0f - cosf(PI * t / smooth));
            } else if (t >= (1.0f - smooth)) { // Fade out
                return 0.5f * (1.0f - cosf(PI * (1.0f - t) / smooth));
            } else { return 1.0f; }
        }
        default: return 1.0f;
    }
}