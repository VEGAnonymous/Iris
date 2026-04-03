#pragma once

#include "Defines.h"

#include <JuceHeader.h>
#include <array>
#include <vector>

class ConvolutionReverb {
private:
    std::array<juce::AudioBuffer<float>, MAX_IR_COUNT> irBuffers {};
    std::array<float, MAX_IR_COUNT> irWeights {};
    
    std::vector<float> inputBufferL{}, inputBufferR{}, outputBufferL{}, outputBufferR{};

public:
    ConvolutionReverb();
    ~ConvolutionReverb() = default;

    void setIRBuffer(int index, juce::AudioBuffer<float> irBuffer);

    void setUniformWeights();
    void setWeights(std::array<float, MAX_IR_COUNT> weights);

    void process(const juce::AudioBuffer<float>& in);
};