#pragma once

#include "Defines.h"

#include <JuceHeader.h>

class IRSlotButton : public juce::Button {
private:
    int irIndex;
    bool occupied = false;
    std::array<std::vector<float>, N_CHANNELS> waveformPoints;

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRSlotButton)

public:
    IRSlotButton(int index);
    ~IRSlotButton() override = default;

    void setWaveform(const juce::AudioBuffer<float>* buffer);
    void setOccupied(bool occupied);

    int getIndex() const;
    bool isOccupied() const;
};