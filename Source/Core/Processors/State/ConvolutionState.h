#pragma once

#include "ConvolutionIRBank.h"
#include "ConvolutionMixState.h"

#include <memory>

struct ConvolutionState {
    std::shared_ptr<const ConvolutionIRBank> irBank;
    ConvolutionMixState mixState;

    void prepare() { mixState.resize(irBank->getMaxPartitionCount()); }
};

struct ConvolutionStateHolder { std::shared_ptr<ConvolutionState> state; };