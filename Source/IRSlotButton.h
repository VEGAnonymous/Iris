#pragma once

#include "Defines.h"
#include "AnimatedAlpha.h"
#include "DragAndDropMixin.h"
#include "WaveformComponent.h"

#include <JuceHeader.h>

class IRSlotButton : public juce::Button, public DragAndDropMixin {
private:
    const float indicatorRadius = 3.0f;
    int irIndex;
    bool occupied = false;
    bool active = false;

    AnimatedAlpha hoverAnim, indicatorActiveAnim;

    WaveformComponent waveformPreview;

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;

    BoundsF getIndicatorBounds(Bounds bounds, const float radius) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRSlotButton)

public:
    AnimatedAlpha dragHover;

    std::function<void(bool)> onActiveToggle;

    IRSlotButton(int index, juce::AnimatorUpdater& updater);
    ~IRSlotButton() override = default;

    void setWaveform(const juce::AudioBuffer<float>* buffer, double sampleRate);
    void setOccupied(bool nOccupied);
    void setActive(bool nActive);

    int getIndex() const;
};