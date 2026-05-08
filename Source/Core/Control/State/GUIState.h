#pragma once

#include "Core/Defines.h"

#include <atomic>
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

    std::atomic<bool> indicatorChanged { false };

    std::array<std::atomic<bool>, MAX_IR_COUNT> irSlotChanged {};
    std::atomic<bool> selectedIRChanged { false };

    std::atomic<bool> irControlsChanged { false };

    juce::SpinLock irWeightsLock;
    std::array<float, MAX_IR_COUNT> irWeights {};
    std::atomic<bool> weightsChanged { false };
    std::atomic<bool> updateWeights { false }; // Editor forced update

    std::array<std::shared_ptr<const juce::AudioBuffer<float>>, MAX_IR_COUNT> irWaveforms;
    juce::SpinLock irWaveformLock;

    std::array<juce::Image, MAX_IR_COUNT> mareImages;
    juce::SpinLock mareLock;

    std::array<std::atomic<float>, MAX_IR_COUNT> crossfadeStates;
    std::array<std::atomic<bool>, MAX_IR_COUNT> crossfadeActives;

    std::atomic<bool> tooltipsEnabledChanged { false };

    GUIState() {
        for (int i = 0; i < MAX_IR_COUNT; ++i) {
            irSlotChanged[i].store(false);
            crossfadeStates[i].store(0.0f);
            crossfadeActives[i].store(false);
        }
    }
};