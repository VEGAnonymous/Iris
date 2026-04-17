#pragma once

#include "Defines.h"

#include <map>

struct PatternParameterState { float rate, modA, modB; };

struct PatternState {
    PositionPattern lastPositionPattern;
    FieldPattern lastFieldPattern;

    juce::SpinLock patternStateLock;

    std::map<PositionPattern, PatternParameterState> positionParamStates;
    std::map<FieldPattern, PatternParameterState> fieldParamStates;
};