#pragma once

#include "Defines.h"

#include <JuceHeader.h>

class ConvolutionState {
private:
    juce::dsp::FFT fft;
    std::array<float, FFT_SIZE*2> irPartition {0};

public:
    std::array<std::shared_ptr<SpectraData>, MAX_IR_COUNT> irSpectra;
    std::array<std::array<float, MAX_IR_COUNT>, N_CHANNELS> irWeights {};
    std::vector<float> irEnvelopes {0.0f};

    std::array<int, MAX_IR_COUNT> irPartitionCounts {};
    std::array<int, MAX_IR_COUNT> irChannelCounts {};
    int maxIRPartitionCount = 0;

    SpectraData mixedSpectra;

    ConvolutionState();
    ConvolutionState(const ConvolutionState& other);
    ~ConvolutionState() = default;

    void updateMaxIRPartitionCount();
    void setIR(int irIndex, const juce::AudioBuffer<float>& irBuffer);
    void setDecay(float decay);
    void mixSpectrum();
};

struct ConvolutionStateHolder { std::shared_ptr<ConvolutionState> state; };