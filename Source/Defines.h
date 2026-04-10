#pragma once

#include <JuceHeader.h>

// Defines
static constexpr auto EPSILON = 1e-3f,
                      sqrt2_2 = juce::MathConstants<float>::sqrt2 / 2.0f;
                      
static constexpr auto REFRESH_RATE = 30;
static constexpr auto CONTROL_RATE = 25.0f;

static constexpr auto N_CHANNELS = 2,
                      MAX_IR_COUNT = 8;

static constexpr auto L = 512,
                      PARTITION_SIZE = 2 * L,
                      FFT_SIZE = 4 * L, // 2048
                      FFT_ORDER = 11, // 2^11, hardcoded
                      HOP_SIZE = FFT_SIZE / 4; // L

namespace ParamID {
    static constexpr auto 
        globalMix = "Global Mix",
        decay = "Decay",

        weightingMode = "Weighting Mode",
        strength = "Strength",
        spread = "Spread",

        positionPattern = "Position Pattern",
        positionRate = "Position Rate",
        positionModA = "Position Mod A",
        positionModB = "Position Mod B",

        fieldSelect = "Field Select",
        fieldPattern = "Field Pattern",
        fieldRate = "Field Rate",
        fieldModA = "Field Mod A",
        fieldModB = "Field Mod B";
}

const juce::StringArray weightingModes { "Absolute", "Relative" };
const juce::StringArray positionPatterns { "Vanilla", "Orbit", "Spiral", "Floral", "Lissajous", "Random", "Walk" };
const juce::StringArray fieldPatterns { "Vanilla", "Ring", "Orbits", "Random", "Walk" };

// Aliases
using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;

// Enums
enum class Axis { X_AXIS, Y_AXIS }; // Polar coordinate reference axis
enum class WeightingMode { ABSOLUTE, RELATIVE };
enum class PositionPattern { MANUAL, ORBIT, SPIRAL, LISSAJOUS, FLORAL, RANDOM_DISCRETE, RANDOM_WALK };
enum class FieldPattern { MANUAL, RING, ORBITS, RANDOM_DISCRETE, RANDOM_WALK };

// Structs
struct Settings {
    float globalMix, decay;
    WeightingMode weightingMode; float strength, spread;
    PositionPattern positionPattern;
    float positionRate, positionModA, positionModB;
    int fieldSelect;
    FieldPattern fieldPattern;
    float fieldRate, fieldModA, fieldModB;

    Settings() : globalMix(0.5f), decay(0.5f),

        weightingMode(WeightingMode::ABSOLUTE), strength(0.5f), spread(1.0f),

        positionPattern(PositionPattern::LISSAJOUS),
        positionRate(0.0f), positionModA(0.5f), positionModB(0.5f),

        fieldSelect(0),
        fieldPattern(FieldPattern::RING),
        fieldRate(0.0f), fieldModA(0.5f), fieldModB(0.5f)
    {}
};

struct CartesianCoordinate { 
    float x, y; 
    bool operator==(const CartesianCoordinate& b) const {
        return std::fabs(x - b.x) < EPSILON && std::fabs(y - b.y) < EPSILON;
    }
};

struct PolarCoordinate {
    float r, theta;
    bool operator==(const PolarCoordinate& b) const {
        // Unwrap angle
        float dTheta = std::fmod(std::fabs(theta - b.theta), juce::MathConstants<float>::twoPi);
        if (dTheta > juce::MathConstants<float>::pi) dTheta = juce::MathConstants<float>::twoPi - dTheta;
        return std::fabs(r - b.r) < EPSILON && dTheta < EPSILON;
    }
};