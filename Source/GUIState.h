#pragma once

#include "Defines.h"

#include <atomic>
#include <mutex>
#include <vector>

struct GUIState {
    std::atomic<PolarCoordinate> position { {0.0f, 0.0f} };
    std::atomic<bool> positionChanged { false };

    std::vector<PolarCoordinate> fieldCoordinates;
    std::mutex fieldMutex;
    std::atomic<bool> fieldChanged { false }; // Notify editor
    std::atomic<bool> updateField { false }; // Editor forced update (e.g., parameter changes)
};