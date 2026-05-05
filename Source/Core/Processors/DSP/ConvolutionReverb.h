#pragma once

#include "Core/Processors/State/ConvolutionState.h"
#include "Core/Defines.h"

#include <JuceHeader.h>
#include <array>
#include <vector>

class ConvolutionReverb {
private:
    // State
    std::shared_ptr<ConvolutionStateHolder> convolutionState;
    
    // Audio buffers
    std::array<std::array<float, PARTITION_SIZE>, N_CHANNELS> inputBuffers {}; 
    std::array<int, N_CHANNELS> inputWriteIndex = {0};

    std::array<std::array<float, FFT_SIZE*2>, N_CHANNELS> inputPartitions {};

    std::array<std::array<std::array<float, PARTITION_SIZE>, 2>, N_CHANNELS> overlapBuffersA {};
    std::array<std::array<float, HOP_SIZE>, N_CHANNELS> overlapBuffersB {};
    int convSaveCount = 0;
    int hopCounter = 0;

    std::array<std::array<float, OUTPUT_SIZE>, N_CHANNELS> outputBuffers {};
    std::array<int, N_CHANNELS> outputWriteIndex = {0};
    std::array<int, N_CHANNELS> outputReadIndex = {0};

    // FFT and spectra
    juce::dsp::WindowingFunction<float> hannWindow;
    juce::dsp::FFT fft;

    std::array<int, N_CHANNELS> spectraIndex = {0};
    SpectraData inputSpectra {};

    std::array<std::array<float, FFT_SIZE*2>, N_CHANNELS> accumulators {0};

    // Hot path
    void accumulateSpectra(ConvolutionState* state, const int channel);
    void overlapAdd(const int channel);

    void processHop(ConvolutionState* state, const int channel);

public:
    ConvolutionReverb(std::shared_ptr<ConvolutionStateHolder> stateHolder);
    ~ConvolutionReverb() = default;

    static int wrapIndex(int index, int size);

    void process(juce::AudioBuffer<float>& in);
};