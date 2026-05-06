#pragma once

#include "Core/Defines.h"

#include <atomic>
#include <mutex>
#include <vector>

struct GUIState {
    std::atomic<PolarCoordinate> position { {0.0f, 0.0f} };
    juce::SpinLock positionLock;
    std::atomic<bool> positionPathChanged { false };
    std::atomic<bool> positionChanged { false };
    std::atomic<bool> updatePosition { false }; // Editor forced update
    std::atomic<bool> syncingPosition { false }; // Guard

    std::vector<PolarCoordinate> fieldCoordinates;
    juce::SpinLock fieldLock;
    std::atomic<bool> fieldChanged { false };
    std::atomic<bool> updateField { false }; // Editor forced update
    std::atomic<bool> syncingField { false }; // Guard

    std::atomic<bool> indicatorStyleChanged { false };

    std::atomic<bool> irChanged { false };
    std::atomic<bool> selectedIRChanged { false };

    std::atomic<bool> irControlsChanged { false };

    std::atomic<bool> updateWeights { false }; // Editor forced update

    std::array<std::shared_ptr<const juce::AudioBuffer<float>>, MAX_IR_COUNT> irWaveforms;
    juce::SpinLock irWaveformLock;

    std::array<juce::Image, MAX_IR_COUNT> mareImages;
    juce::SpinLock mareLock;

    std::atomic<bool> tooltipsEnabledChanged { false };
};