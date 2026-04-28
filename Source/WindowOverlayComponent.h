#pragma once

#include "DragAndDropMixin.h"
#include "RangeSelectorComponent.h"

#include <JuceHeader.h>

class WindowOverlayComponent : public RangeSelectorComponent, public DragAndDropMixin {
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WindowOverlayComponent)

protected:
    void beginGesture() override;
    void updateGesture() override;
    void endGesture() override;

public:
    AnimatedAlpha dragHover;

    std::function<void(float start, float length)> onWindowChanged;

    WindowOverlayComponent(juce::AnimatorUpdater& updater);
    ~WindowOverlayComponent() override = default;

    void paint(juce::Graphics& g) override;

    void setWindow(float nStart, float nEnd);
};