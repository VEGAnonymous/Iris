#pragma once

#include <JuceHeader.h>
#include <vector>

enum class EnvelopeType { HANN, SINE, TRI, PERC };

const juce::StringArray envelopeTypes { "Hann", "Sine", "Tri", "Perc" };

static constexpr float
    ENVELOPE_PERC_RELEASE_MIN = 1.26f,
    ENVELOPE_PERC_RELEASE_MAX = 62.1f;

struct Envelope {
    int length = 0;
    EnvelopeType type = EnvelopeType::HANN;
    float attack = 0.0f, release = 0.0f; // Normalized (up to half window length)
    
    std::vector<float> curve {};

    void computeEnvelope();
    static void computeEnvelopeCurve(std::vector<float>& curve, int length, EnvelopeType type, float attack, float release);
    static float getEnvelopeValue(float t, EnvelopeType type, bool isRelease, float attack = 0.01f, float release = 5.0f);
};