#pragma once

#include <JuceHeader.h>

// Defines
static constexpr auto sqrt2_2 = juce::MathConstants<float>::sqrt2 / 2.0f;

static constexpr auto REFRESH_RATE = 60;
static constexpr auto CONTROL_RATE = 25.0f;

static constexpr auto N_CHANNELS = 2; // Stereo
static constexpr auto MAX_IR_COUNT = 8;

static constexpr auto L = 512;
static constexpr auto PARTITION_SIZE = 2 * L;
static constexpr auto FFT_SIZE = 4 * L; // 2048
static constexpr auto FFT_ORDER = 11; // 2^11, hardcoded
static constexpr auto HOP_SIZE = FFT_SIZE / 4; // L

const juce::StringArray positionPatterns { "Vanilla", "Orbit", "Spiral", "Floral", "Lissajous", "Random", "Walk" };
const juce::StringArray coordinatePatterns { "Vanilla", "Ring", "Orbit", "Grid", "Random", "Walk" };

// Aliases
using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;

// Enums
enum class Axis { X_AXIS, Y_AXIS }; // Polar coordinate reference axis
enum class PositionPattern { MANUAL, ORBIT, SPIRAL, LISSAJOUS, FLORAL, RANDOM_DISCRETE, RANDOM_WALK };
enum class CoordinatePattern { MANUAL, RING, ORBIT, GRID, RANDOM_DISCRETE, RANDOM_WALK };

// Structs
struct Settings {
    float globalMix;
    float decay;
    PositionPattern positionPattern;
    float positionRate, positionModA, positionModB;

    Settings() : globalMix(0.5f), decay(0.5f),
        positionPattern(PositionPattern::LISSAJOUS),
        positionRate(0.0f), positionModA(0.5f), positionModB(0.5f)
    {}
};

struct CartesianCoordinate { float x, y; };
struct PolarCoordinate { float r, theta; };