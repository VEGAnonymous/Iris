#pragma once

#include "Defines.h"

#include <map>

struct PatternState {
    PositionPattern lastPositionPattern;
    FieldPattern lastFieldPattern;

    struct PatternParameterState { float rate, modA, modB; };

    std::map<PositionPattern, PatternParameterState> positionParamStates;
    std::map<FieldPattern, PatternParameterState> fieldParamStates;
};