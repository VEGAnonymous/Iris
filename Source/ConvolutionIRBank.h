#pragma once

#include "Defines.h"
#include <array>
#include <memory>

class ConvolutionIRBank {
private:
    juce::dsp::FFT fft;

    std::array<std::shared_ptr<SpectraData>, MAX_IR_COUNT> irSpectra;
    std::array<float, FFT_SIZE*2> irPartition {0}; // Temp buffer

    std::array<int, MAX_IR_COUNT> irPartitionCounts {};
    std::array<int, MAX_IR_COUNT> irChannelCounts {};

    int maxPartitionCount = 0;
    void updateMaxPartitionCount();

public:
    ConvolutionIRBank();
    ConvolutionIRBank(const ConvolutionIRBank& other);
    ~ConvolutionIRBank() = default;

    void setIR(int irIndex, const juce::AudioBuffer<float>& irBuffer);
    void clearIR(int irIndex);

    const SpectraData& getSpectra(int irIndex) const;
    int getMaxPartitionCount() const;
    int getPartitionCount(int irIndex) const;
    int getChannelCount(int irIndex) const;
};