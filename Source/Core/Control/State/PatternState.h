#pragma once

#include "Core/Defines.h"

#include <map>

struct PatternParameterState { float rate, modA, modB; };

struct PatternState {
    PositionPattern lastPositionPattern = PositionPattern::FLORAL;
    FieldPattern lastFieldPattern = FieldPattern::RING;

    juce::SpinLock patternStateLock;

    std::map<PositionPattern, PatternParameterState> positionParamStates;
    std::map<FieldPattern, PatternParameterState> fieldParamStates;
};