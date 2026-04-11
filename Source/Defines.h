#pragma once

#include <JuceHeader.h>

// Defines
static constexpr auto EPSILON = 1e-3f,
                      sqrt2_2 = juce::MathConstants<float>::sqrt2 / 2.0f;       

static constexpr auto L = 512,
                      PARTITION_SIZE = 2 * L,
                      FFT_SIZE = 4 * L, // 2048
                      FFT_ORDER = 11, // 2^11, hardcoded
                      HOP_SIZE = FFT_SIZE / 4; // L

static constexpr auto N_CHANNELS = 2,
                      MAX_IR_COUNT = 8,
                      MAX_IR_PARTITIONS = 512,
                      MAX_IR_SAMPLES = MAX_IR_PARTITIONS * PARTITION_SIZE;

static constexpr auto REFRESH_RATE = 30;
static constexpr auto CONTROL_RATE = 25.0f;

static constexpr auto SWAP_INTERVAL_MIN = 5.0f,
                      SWAP_INTERVAL_MAX = 60.0f;

namespace ParamID {
    static constexpr auto 
        globalMix = "Global Mix",
        decay = "Decay",
        lowCut = "Low Cut",
        highCut = "High Cut",

        weightingMode = "Weighting Mode",
        strength = "Strength",
        spread = "Spread",

        positionPattern = "Position Pattern",
        positionRate = "Position Rate",
        positionModA = "Position Mod A",
        positionModB = "Position Mod B",

        fieldPattern = "Field Pattern",
        fieldRate = "Field Rate",
        fieldModA = "Field Mod A",
        fieldModB = "Field Mod B";

    inline juce::String irSwapMin(int i) { return "IR " + juce::String(i) + " Swap Min"; }
    inline juce::String irSwapMax(int i) { return "IR " + juce::String(i) + " Swap Max"; }
}

namespace PropertyID {
    static constexpr auto selectedIR = "Selected IR";
}

const juce::StringArray weightingModes { "Absolute", "Relative" };
const juce::StringArray positionPatterns { "Vanilla", "Orbit", "Spiral", "Floral", "Lissajous", "Random", "Walk" };
const juce::StringArray fieldPatterns { "Vanilla", "Ring", "Orbits", "Random", "Walk" };

// Aliases
using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;

using Bounds = juce::Rectangle<int>;

// Enums
enum class WeightingMode { ABSOLUTE, RELATIVE };

enum class Axis { X_AXIS, Y_AXIS }; // Polar coordinate reference axis
enum class PositionPattern { MANUAL, ORBIT, SPIRAL, LISSAJOUS, FLORAL, RANDOM_DISCRETE, RANDOM_WALK };
enum class FieldPattern { MANUAL, RING, ORBITS, RANDOM_DISCRETE, RANDOM_WALK };

enum class ControlGroup { GLOBAL, INTERACTION, POSITION, FIELD };

// Structs
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

struct ControlDef {
    juce::Component* component;
    ControlGroup group;
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