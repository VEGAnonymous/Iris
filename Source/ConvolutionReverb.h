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
    std::vector<std::array<float, FFT_SIZE>> inputBuffers {};
    std::vector<std::array<float, FFT_SIZE/2>> overlapBuffers{};
    std::vector<std::deque<float>> outputQueues;
    std::vector<int> writeIndex = {0};

    // FFT and spectra
    juce::dsp::WindowingFunction<float> hannWindow;
    juce::dsp::FFT fft;

    std::vector<int> spectraIndex = {0};
    std::vector< // Channels
        std::vector< // Partitions
        std::array<float, FFT_SIZE*2> // Packed FFT output
        >
    > inputSpectra;

    std::array<float, FFT_SIZE*2> partitionBuffer;
    std::array< // IRs 
        std::vector< // Channels
            std::vector< // Partitions
                std::array<float, FFT_SIZE*2> // Packed FFT output
            >
        >, MAX_IR_COUNT
    > irSpectra;

    std::vector< // Channels
        std::vector< // Partitions
            std::array<float, FFT_SIZE*2> // Packed FFT output
        >
    > mixedSpectra;

    // M (input block size) = buffer.getNumSamples, P (IR partition size in samples) = power of 2, N (FFT size) = 2 * P

    void updateIRFFT(int index);

    void mixSpectrum();

    void processInput(std::array<float, FFT_SIZE*2>& partition);

public:
    ConvolutionReverb(int channels);
    ~ConvolutionReverb() = default;

    void setInputChannels(int n);

    void setIRBuffer(int index, juce::AudioBuffer<float> irBuffer);

    void setUniformWeights();
    void setWeights(std::array<float, MAX_IR_COUNT> weights);

    void process(juce::AudioBuffer<float>& in);
};