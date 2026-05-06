#pragma once

#include "Core/Defines.h"

#include <array>
#include <memory>

class ConvolutionIRBank {
private:
    juce::dsp::FFT fft;

    std::array<std::shared_ptr<SpectraData>, MAX_IR_COUNT> irSpectra;
    std::array<bool, MAX_IR_COUNT> irActiveStates {true};
    std::array<float, MAX_IR_COUNT> irGains {};

    std::array<int, MAX_IR_COUNT> irPartitionCounts {};
    std::array<int, MAX_IR_COUNT> irChannelCounts {};

    int maxPartitionCount = 0;

public:
    ConvolutionIRBank();
    ConvolutionIRBank(const ConvolutionIRBank& other);
    ~ConvolutionIRBank() = default;

    void setIR(int irIndex, const juce::AudioBuffer<float>& irBuffer);
    void clearIR(int irIndex);
    void setIRActive(int irIndex, bool nState);
    void setIRGain(int irIndex, float nGain = 1.0f);
    void updateMaxPartitionCount();

    const SpectraData& getSpectra(int irIndex) const;
    int getMaxPartitionCount() const;
    int getPartitionCount(int irIndex) const;
    int getChannelCount(int irIndex) const;
    bool isActive(int irIndex) const;
    float getGain(int irIndex) const;
};