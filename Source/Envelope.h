#pragma once

#include <vector>

enum class EnvelopeType { NONE, HANN, HAMMING, SINE, TRI, PERC };

struct Envelope {
    int length = 0;
    EnvelopeType type = EnvelopeType::NONE;
    float attack = 0.05f, release = 0.05f; // Normalized (up to half window length)
    
    std::vector<float> curve {};

    void computeEnvelope();
    static void computeEnvelopeCurve(std::vector<float>& curve, int length, EnvelopeType type, float attack, float release);
    static float getEnvelopeValue(float t, EnvelopeType type, bool isRelease, float attack = 0.01f, float release = 5.0f);
};