#pragma once

#include "ControlLabel.h"

#include <JuceHeader.h>

template <typename T>
class LabelledControl : public juce::Component {
public:
    T control;
    ControlLabel label;

    template <typename... Args>
    LabelledControl(const juce::String& paramName, Args&&... args) : control(std::forward<Args>(args)...), label(paramName) {
        addAndMakeVisible(control);
        addAndMakeVisible(label);
    }

    void resized() override {
        const int labelHeight = 12;
        auto bounds = getLocalBounds();
        label.setBounds(bounds.removeFromBottom(labelHeight));
        control.setBounds(bounds);
    }
};