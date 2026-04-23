#pragma once

#include "ControlLabel.h"

#include <JuceHeader.h>

template <typename T>
class LabelledControl : public juce::Component {
private:
    int labelHeight = 12;
    int controlHeight = 0;

public:
    T control;
    ControlLabel label;

    template <typename... Args>
    LabelledControl(const juce::String& paramName, Args&&... args) 
        : control(std::forward<Args>(args)...), label(paramName) {
        addAndMakeVisible(control);
        addAndMakeVisible(label);
    }

    void resized() override {
        auto bounds = getLocalBounds();
        label.setBounds(bounds.removeFromBottom(labelHeight));
        control.setBounds(controlHeight > 0 ? bounds.withSizeKeepingCentre(bounds.getWidth(), controlHeight) : bounds);
    }

    void setLabelHeight(int nHeight) { labelHeight = nHeight; }
    void setControlHeight(int nHeight) { controlHeight = nHeight; }
};