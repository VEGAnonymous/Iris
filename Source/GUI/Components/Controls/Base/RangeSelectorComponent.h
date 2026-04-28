#pragma once

#include "GUI/API/AnimatedAlpha.h"
#include "GUI/API/ValueTooltipClient.h"
#include "GUI/Theme/Theme.h"

#include <JuceHeader.h>

static constexpr float MIN_GAP = 0.01f;

class RangeSelectorComponent : public juce::Component, public juce::SettableTooltipClient, public ValueTooltipClient {
protected:
    float start = 0.0f, end = 1.0f;
    float maxLength = 1.0f; // norm

    enum class DragTarget { NONE, START, END };
    DragTarget dragTarget{ DragTarget::NONE };
    float hitRadius = 4.0f;

    bool selecting = false;
    float dragStart = 0.0f;
    
    const bool updateDuringDrag = false;

    AnimatedAlpha hoverStart, hoverEnd;

    float map(float x) const;
    float inverseMap(float norm) const;

    DragTarget hitHandle(juce::Point<float> p) const;

    virtual void beginGesture() {}
    virtual void updateGesture() {}
    virtual void endGesture() {}

public:
    RangeSelectorComponent(juce::AnimatorUpdater& updater, float hitRadius = 4.0f, bool shouldUpdateDuringDrag = false);
    ~RangeSelectorComponent() override = default;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    juce::String getValueTooltip();

    void setRange(float nStart, float nEnd);
    void setMaxLength(float norm);

    float getStart() const;
    float getEnd() const;
};