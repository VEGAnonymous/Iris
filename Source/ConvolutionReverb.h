#pragma once

#include "Defines.h"

#include <JuceHeader.h>
#include <array>
#include <vector>

class ConvolutionReverb {
private:
    // Concurrency
    juce::ReadWriteLock irLock;

    // IRs
    std::array<juce::AudioBuffer<float>, MAX_IR_COUNT> irBuffers {};
    std::array<std::array<float, MAX_IR_COUNT>, N_CHANNELS> irWeights {};
    std::vector<float> irEnvelopes {};

    int maxIRPartitionCount = 0;
    
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
    using SpectraData =
        std::array< // Channels
            std::vector< // Partitions
                std::array<float, FFT_SIZE> // Packed FFT output
            >,
        N_CHANNELS>;

    juce::dsp::WindowingFunction<float> hannWindow;
    juce::dsp::FFT fft;

    std::array<int, N_CHANNELS> spectraIndex = {0};
    SpectraData inputSpectra {};

    std::array<float, FFT_SIZE*2> irPartition {0};
    std::array<SpectraData, MAX_IR_COUNT> irSpectra {};

    SpectraData mixedSpectra;

    std::array<std::array<float, FFT_SIZE*2>, N_CHANNELS> accumulators {0};

    // Preprocessing
    void updateMaxIRPartitionCount();
    void updateIRFFT(int index);
    void mixSpectrum();

    // Hot path
    void accumulateSpectra(int channel);
    void overlapAdd(int channel);
    void processHop(int channel);

public:
    ConvolutionReverb();
    ~ConvolutionReverb() = default;

    static int wrapIndex(int index, int size);

    void setIRBuffer(int index, juce::AudioBuffer<float> irBuffer);
    void clearIRBuffer(int index);

    void setWeights(std::array<std::array<float, MAX_IR_COUNT>, N_CHANNELS> weights);

    void setDecay(float decay);

    void process(juce::AudioBuffer<float>& in);
};