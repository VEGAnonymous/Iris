#pragma once

#include "GUI/API/AnimatedValue.h"
#include "GUI/Theme/Theme.h"

#include <JuceHeader.h>

class HoverableTextEditor : public juce::TextEditor {
private:
    AnimatedValue hoverAnim;

public:
    HoverableTextEditor(juce::AnimatorUpdater& updater);
    ~HoverableTextEditor() override = default;

    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    float getHoverAlpha() const;
};