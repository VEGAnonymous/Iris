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

    std::atomic<bool> swapChanged { false };
    std::atomic<bool> syncingSwap { false }; // Guard

    std::atomic<bool> irChanged { false };
    std::atomic<bool> selectedIRChanged { false };
};