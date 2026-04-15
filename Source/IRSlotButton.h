#pragma once

#include "Defines.h"

#include <JuceHeader.h>

class IRSlotButton : public juce::Button {
private:
    int irIndex;
    bool occupied = false;
    bool active = false;
    std::array<std::vector<float>, N_CHANNELS> waveformPoints;

    const float indicatorRadius = 3.0f;

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;
    void mouseDown(const juce::MouseEvent& e) override;

    BoundsF getIndicatorBounds(Bounds bounds, const float radius) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRSlotButton)

public:
    std::function<void(bool)> onActiveToggle;

    IRSlotButton(int index);
    ~IRSlotButton() override = default;

    void setWaveform(const juce::AudioBuffer<float>* buffer);
    void setOccupied(bool nOccupied);
    void setActive(bool nActive);

    int getIndex() const;
};