#pragma once

#include "Core/Defines.h"
#include "Core/Control/State/GUIState.h"
#include "GUI/API/AnimatedValue.h"
#include "GUI/API/DragAndDropMixin.h"
#include "GUI/Components/Base/WaveformComponent.h"

#include <JuceHeader.h>

class IRSlotButton : public juce::Button, public DragAndDropMixin {
private:
    GUIState& guiState;

    const float indicatorRadius = 3.0f;
    int irIndex;
    bool occupied = false;
    bool active = false;

    AnimatedValue hoverAnim, indicatorActiveAnim;

    WaveformComponent waveformPreview;

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;

    BoundsF getIndicatorBounds(Bounds bounds, const float radius) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRSlotButton)

public:
    AnimatedValue dragHover;

    std::function<void(bool)> onActiveToggle;

    IRSlotButton(int index, juce::AnimatorUpdater& updater, GUIState& gState);
    ~IRSlotButton() override = default;

    void setWaveform(const juce::AudioBuffer<float>* buffer, double sampleRate);
    void setOccupied(bool nOccupied);
    void setActive(bool nActive);

    int getIndex() const;
};