#pragma once

#include "Defines.h"

#include <JuceHeader.h>

struct Settings {
    float globalMix, decay, lowCut, highCut;
    WeightingMode weightingMode; float strength, spread;
    PositionPattern positionPattern;
    float positionRate, positionModA, positionModB;
    FieldPattern fieldPattern;
    float fieldRate, fieldModA, fieldModB;

    Settings() : globalMix(0.5f), decay(0.5f), lowCut(20.0f), highCut(20000.0f),

        weightingMode(WeightingMode::ABSOLUTE), strength(0.5f), spread(1.0f),

        positionPattern(PositionPattern::LISSAJOUS),
        positionRate(0.0f), positionModA(0.5f), positionModB(0.5f),

        fieldPattern(FieldPattern::RING),
        fieldRate(0.0f), fieldModA(0.5f), fieldModB(0.5f)
    {}
};

inline Settings getSettings(const juce::AudioProcessorValueTreeState& parameters) {
    Settings settings;

    settings.globalMix = parameters.getRawParameterValue(ParamID::globalMix)->load();
    settings.decay = parameters.getRawParameterValue(ParamID::decay)->load();
    settings.lowCut = parameters.getRawParameterValue(ParamID::lowCut)->load();
    settings.highCut = parameters.getRawParameterValue(ParamID::highCut)->load();

    settings.weightingMode = static_cast<WeightingMode>(parameters.getRawParameterValue(ParamID::weightingMode)->load());
    settings.strength = parameters.getRawParameterValue(ParamID::strength)->load();
    settings.spread = parameters.getRawParameterValue(ParamID::spread)->load();

    settings.positionPattern = static_cast<PositionPattern>(parameters.getRawParameterValue(ParamID::positionPattern)->load());
    settings.positionRate = parameters.getRawParameterValue(ParamID::positionRate)->load();
    settings.positionModA = parameters.getRawParameterValue(ParamID::positionModA)->load();
    settings.positionModB = parameters.getRawParameterValue(ParamID::positionModB)->load();

    settings.fieldPattern = static_cast<FieldPattern>(parameters.getRawParameterValue(ParamID::fieldPattern)->load());
    settings.fieldRate = parameters.getRawParameterValue(ParamID::fieldRate)->load();
    settings.fieldModA = parameters.getRawParameterValue(ParamID::fieldModA)->load();
    settings.fieldModB = parameters.getRawParameterValue(ParamID::fieldModB)->load();

    return settings;
}