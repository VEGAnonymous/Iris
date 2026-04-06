#pragma once

#include "Defines.h"

#include <JuceHeader.h>
#include <array>
#include <vector>

class ConvolutionReverb {
private:
    // Mix
    int inputChannels = 2;

    // IRs
    std::array<juce::AudioBuffer<float>, MAX_IR_COUNT> irBuffers {};
    std::array<float, MAX_IR_COUNT> irWeights {1.0f / MAX_IR_COUNT};
    int maxIRPartitionCount = 0;
    
    // Audio buffers
    std::vector<std::array<float, PARTITION_SIZE>> inputBuffers {}; 
    std::vector<int> inputWriteIndex = {0};

    std::vector<std::array<float, FFT_SIZE*2>> inputPartitions;

    std::vector<std::array<std::array<float, PARTITION_SIZE>, 2>> overlapBuffersA {};
    std::vector<std::array<float, HOP_SIZE>> overlapBuffersB {};
    int convSaveCount = 0;
    int hopCounter = 0;

    std::vector<std::vector<float>> outputBuffers;
    std::vector<int> outputWriteIndex = {0};
    std::vector<int> outputReadIndex = {0};
    const int outputSize = 4 * L;

    // FFT and spectra
    juce::dsp::WindowingFunction<float> hannWindow;
    juce::dsp::FFT fft;

    std::vector<int> spectraIndex = {0};
    std::vector< // Channels
        std::vector< // Partitions
        std::array<float, FFT_SIZE> // Packed FFT output
        >
    > inputSpectra;

    std::array<float, FFT_SIZE*2> irPartition;
    std::array< // IRs 
        std::vector< // Channels
            std::vector< // Partitions
                std::array<float, FFT_SIZE> // Packed FFT output
            >
        >, MAX_IR_COUNT
    > irSpectra;

    std::vector< // Channels
        std::vector< // Partitions
            std::array<float, FFT_SIZE> // Packed FFT output
        >
    > mixedSpectra;

    std::vector<std::array<float, FFT_SIZE*2>> accumulators;

    void updateIRFFT(int index);

    void mixSpectrum();

public:
    ConvolutionReverb(int channels);
    ~ConvolutionReverb() = default;

    void setInputChannels(int n);

    void setIRBuffer(int index, juce::AudioBuffer<float> irBuffer);

    void setUniformWeights();
    void setWeights(std::array<float, MAX_IR_COUNT> weights);

    void process(juce::AudioBuffer<float>& in);
};