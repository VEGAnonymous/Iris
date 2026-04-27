#pragma once

#include "ControlLabel.h"

#include <JuceHeader.h>

template <typename T>
class LabelledControl : public juce::Component {
private:
    float labelWidth = 0.0f, labelHeight = 0.0f;
    float controlWidth = 0.0f, controlHeight = 0.0f;
    juce::FlexItem::Margin labelMargin = 0.0f, controlMargin = 0.0f;

public:
    enum class LabelPosition { LEFT, RIGHT, TOP, BOTTOM };

    T control;
    ControlLabel label;
    juce::String paramName;
    LabelPosition labelPosition = LabelPosition::BOTTOM;
    juce::FlexBox flex;

    template <typename... Args>
    LabelledControl(const juce::String& paramName, Args&&... args)
        : control(std::forward<Args>(args)...), label(paramName), paramName(paramName),
        flex(juce::FlexBox::JustifyContent::center) {
        addAndMakeVisible(control);
        addAndMakeVisible(label);
        flex.alignItems = juce::FlexBox::AlignItems::center;
    }

    void resized() override {
        flex.items.clear();
        auto bounds = getLocalBounds();

        juce::FlexItem labelItem(label);
        juce::FlexItem controlItem(control);

        labelItem = labelItem.withFlex(0.0f);
        controlItem = controlItem.withFlex(0.0f);

        labelItem = labelItem
            .withWidth(labelWidth)
            .withHeight(labelHeight)
            .withMargin(labelMargin);
            
        controlItem = controlItem
            .withWidth(controlWidth)
            .withHeight(controlHeight)
            .withMargin(controlMargin);

        switch (labelPosition) {
            case LabelPosition::LEFT:
            case LabelPosition::RIGHT:
                flex.flexDirection = juce::FlexBox::Direction::row;
                break;
            case LabelPosition::TOP:
            case LabelPosition::BOTTOM:
                flex.flexDirection = juce::FlexBox::Direction::column;
                break;
        }

        switch (labelPosition) {
            case LabelPosition::LEFT:
            case LabelPosition::TOP:
                flex.items.add(labelItem);
                flex.items.add(controlItem);
                break;
            case LabelPosition::RIGHT:
            case LabelPosition::BOTTOM:
                flex.items.add(controlItem);
                flex.items.add(labelItem);
                break;
        }

        flex.performLayout(bounds.toFloat());
    }

    void setLabelDimensions(float nWidth = -1.0f, float nHeight = -1.0f) { 
        if (nWidth >= 0) labelWidth = nWidth; 
        if (nHeight >= 0) labelHeight = nHeight;
    }

    void setLabelMargin(juce::FlexItem::Margin nMargin) { labelMargin = nMargin; }

    void setControlDimensions(float nWidth = -1.0f, float nHeight = -1.0f) {
        if (nWidth >= 0) controlWidth = nWidth;
        if (nHeight >= 0) controlHeight = nHeight;
    }

    void setControlMargin(juce::FlexItem::Margin nMargin) { controlMargin = nMargin; }
    
    void setLabelPosition(LabelPosition nPosition) {
        labelPosition = nPosition;
        resized();
    }

};