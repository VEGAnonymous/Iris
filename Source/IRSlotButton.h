#pragma once

#include "Defines.h"
#include "WaveformComponent.h"

#include <JuceHeader.h>

class IRSlotButton : public juce::Button {
private:
    int irIndex;
    bool occupied = false;
    bool active = false;

    const float indicatorRadius = 3.0f;

    WaveformComponent waveformPreview;

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    BoundsF getIndicatorBounds(Bounds bounds, const float radius) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRSlotButton)

public:
    std::function<void(bool)> onActiveToggle;

    IRSlotButton(int index);
    ~IRSlotButton() override = default;

    void setWaveform(const juce::AudioBuffer<float>* buffer, double sampleRate);
    void setOccupied(bool nOccupied);
    void setActive(bool nActive);

    int getIndex() const;
};