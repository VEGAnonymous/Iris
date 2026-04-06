#pragma once

#include <JuceHeader.h>

// Defines
static constexpr auto sqrt2_2 = juce::MathConstants<float>::sqrt2 / 2.0f;

static constexpr auto CONTROL_RATE = 25.0f; // Hz

static constexpr auto N_CHANNELS = 2; // Stereo
static constexpr auto MAX_IR_COUNT = 8;

static constexpr auto L = 512;
static constexpr auto PARTITION_SIZE = 2 * L;
static constexpr auto FFT_SIZE = 4 * L; // 2048
static constexpr auto FFT_ORDER = 11; // 2^11, hardcoded
static constexpr auto HOP_SIZE = FFT_SIZE / 4; // L



// Aliases
using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;

// Enums
enum Axis { X_AXIS, Y_AXIS }; // Polar coordinate reference axis
enum MotionPattern { RANDOM_WALK, LISSAJOUS }; // Curve

// Structs
struct PolarCoordinate {
	float r;
	float theta;
};