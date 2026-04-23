#include "Defines.h"
#include "Envelope.h"

#include <JuceHeader.h>

void Envelope::computeEnvelope() { computeEnvelopeCurve(curve, length, type, attack, release); }

void Envelope::computeEnvelopeCurve(std::vector<float>& curve, int length, EnvelopeType type, float attack, float release) {
    jassert(length > 0);
    curve.resize(length);

    if (type == EnvelopeType::PERC) {
        for (int i = 0; i < length; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(juce::jmax(1, length - 1));
            curve[i] = getEnvelopeValue(t, EnvelopeType::PERC, false,
                juce::jmap(attack, 0.0f, 0.99f), juce::jmap(release, 1.26f, 40.0f));
        }
        return;
    }

    const int attackSamples = static_cast<int>(attack * length);
    const int releaseSamples = static_cast<int>(release * length);
    const int sustainSamples = length - (attackSamples + releaseSamples);

    for (int i = 0; i < attackSamples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(juce::jmax(1, attackSamples - 1));
        curve[i] = getEnvelopeValue(t, type, false);
    }

    for (int i = 0; i < sustainSamples; ++i) {
        const int idx = attackSamples + i;
        curve[idx] = 1.0f;
    }

    for (int i = 0; i < releaseSamples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(juce::jmax(1, releaseSamples - 1));
        const int idx = attackSamples + sustainSamples + i;
        curve[idx] = getEnvelopeValue(t, type, true);
    }
}

inline float Envelope::getEnvelopeValue(float t, EnvelopeType type, bool isRelease, float attack, float release) {
    auto& PI = juce::MathConstants<float>::pi;
    t = juce::jlimit(0.0f, 1.0f, t);
    switch (type) {
    case EnvelopeType::HANN:
        return isRelease ? 0.5f * (1.0f + cosf(PI * t))
            : 0.5f * (1.0f - cosf(PI * t));

    case EnvelopeType::HAMMING:
        return isRelease ? 0.54f + (0.46f * cosf(PI * t))
            : 0.54f - (0.46f * cosf(PI * t));

    case EnvelopeType::SINE:
        return isRelease ? cosf(PI * 0.5f * t)
            : sinf(PI * 0.5f * t);

    case EnvelopeType::TRI:
        return isRelease ? 1.0f - t
            : t;

    case EnvelopeType::PERC: {
        if (t < attack)
            return t / std::max(attack, EPSILON); // Linear attack
        else { // Exponential decay
            float tDecay = (t - attack) / (1.0f - attack);
            return expf(-release * tDecay); // release parameter = decay factor
        }
    }
    case EnvelopeType::NONE:
    default:
        return 1.0f;
    }
}