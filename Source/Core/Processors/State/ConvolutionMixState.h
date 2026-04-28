#pragma once

#include "Core/Defines.h"
#include "Core/Processors/State/ConvolutionIRBank.h"

#include <array>
#include <vector>

struct ConvolutionMixState {
    std::array<std::array<float, MAX_IR_COUNT>, N_CHANNELS> irWeights{};
    std::vector<float> irEnvelopes;
    SpectraData mixedSpectra;

    ConvolutionMixState();
    ~ConvolutionMixState() = default;

    void resize(int partitionCount);
    void setWeights(std::array<std::array<float, MAX_IR_COUNT>, N_CHANNELS> weights);
    void setDecay(float decay, int maxPartitions);
    void mixSpectrum(const ConvolutionIRBank& irBank);
};