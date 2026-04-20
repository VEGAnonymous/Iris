#pragma once

#include <JuceHeader.h>

class RotaryLookAndFeel : public juce::LookAndFeel_V4 {
private:
    void drawOutline(juce::Graphics& g, juce::Point<float> center, float outlineRadius, float outlineThickness,
        float startAngle, float endAngle, float hoverAlpha);

    void drawTicks(juce::Graphics& g, juce::Point<float> center, float tickRadius, 
        float startAngle, float endAngle, int numTicks, float hoverAlpha);

    void drawArc(juce::Graphics& g, juce::Point<float> center, float arcRadius, float arcThickness,
        float startAngle, float endAngle, float valueAngle, float hoverAlpha, bool isBipolar);

    void drawIndicator(juce::Graphics& g, juce::Point<float> center, float indicatorRadius, float indicatorDotRadius, 
        float valueAngle, float hoverAlpha);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotaryLookAndFeel)

public:
    RotaryLookAndFeel();
    ~RotaryLookAndFeel() override = default;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
};