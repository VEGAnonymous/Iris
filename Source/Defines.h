#pragma once

#include <JuceHeader.h>

// Defines
static constexpr auto MAX_IR_COUNT = 8;
static constexpr auto FFT_ORDER = 11; // 2^11
static constexpr auto FFT_SIZE = 1 << FFT_ORDER; // 2048
static constexpr auto HOP_SIZE = FFT_SIZE / 2; // 50% overlap

// Aliases
using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;