#pragma once

#include "AnimatedAlpha.h"
#include "Theme.h"

#include <JuceHeader.h>

class WindowOverlayComponent : public juce::Component {
private:
    float start = 0.0f, end = 1.0f;
    int offsetX = 0;

    enum class DragTarget { NONE, START, END };
    DragTarget dragTarget{ DragTarget::NONE };

    bool selecting = false;
    float dragStart = 0.0f;

    const float hitRadius = 4.0f;

    AnimatedAlpha hoverStart, hoverEnd;

    float map(float x) const;
    float inverseMap(float norm) const;
    
    DragTarget hitHandle(juce::Point<float> p) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WindowOverlayComponent)

public:
    std::function<void(float start, float length)> onWindowChanged;

    WindowOverlayComponent(juce::AnimatorUpdater& updater);
    ~WindowOverlayComponent() override = default;

    void paint(juce::Graphics& g) override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    void setOffsetX(int nOffsetX = 0);
    void setWindow(float nStart, float nEnd);
};