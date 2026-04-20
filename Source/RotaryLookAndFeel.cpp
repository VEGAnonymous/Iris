#include "Hoverable.h"
#include "RotaryLookAndFeel.h"
#include "Rotary.h"
#include "Theme.h"

/* PUBLIC */

RotaryLookAndFeel::RotaryLookAndFeel() {}

void RotaryLookAndFeel::drawTicks(juce::Graphics& g, juce::Point<float> center, float tickRadius,
    float startAngle, float endAngle, int numTicks, float hoverAlpha) {
    for (int i = 0; i < numTicks; ++i) {
        const float normAngle = static_cast<float>(i) / static_cast<float>(numTicks - 1);
        const float angle = juce::jmap(normAngle, startAngle, endAngle);

        const float tickX = center.x + (tickRadius * std::sin(angle));
        const float tickY = center.y - (tickRadius * std::cos(angle));
        const float tickDotRadius = 1.2f;

        juce::Colour tickColor = (Theme::Colors::highlight).withAlpha(0.3f);
        if (hoverAlpha > 0.0f) tickColor = tickColor.brighter(hoverAlpha * 0.25f);
        g.setColour(tickColor);
        g.fillEllipse(tickX - tickDotRadius, tickY - tickDotRadius, tickDotRadius * 2.0f, tickDotRadius * 2.0f);
    }
}

void RotaryLookAndFeel::drawOutline(juce::Graphics& g, juce::Point<float> center, float outlineRadius, float outlineThickness,
    float startAngle, float endAngle, float hoverAlpha) {
    auto outlineColor = (Theme::Colors::outline).interpolatedWith(Theme::Colors::outlineHover, hoverAlpha);

    juce::Path outline;
    outline.addCentredArc(center.x, center.y, outlineRadius, outlineRadius, 0.0f, startAngle, endAngle, true);

    g.setColour(outlineColor);
    g.strokePath(outline, juce::PathStrokeType(outlineThickness + (outlineThickness * hoverAlpha * 0.25f)));
}

void RotaryLookAndFeel::drawArc(juce::Graphics& g, juce::Point<float> center, float arcRadius, float arcThickness,
    float startAngle, float endAngle, float valueAngle, float hoverAlpha, bool isBipolar) {
    auto arcColor = (Theme::Colors::highlight).interpolatedWith(Theme::Colors::highlightHover, hoverAlpha);

    juce::Path arc;
    arc.addCentredArc(center.x, center.y, arcRadius, arcRadius, 0.0f,
        isBipolar ? (startAngle + endAngle) / 2.0f : startAngle, valueAngle, true);

    g.setColour(arcColor);
    g.strokePath(arc, juce::PathStrokeType(arcThickness + (arcThickness * hoverAlpha * 0.25f)));
}

void RotaryLookAndFeel::drawIndicator(juce::Graphics& g, juce::Point<float> center, float indicatorRadius, float indicatorDotRadius,
    float valueAngle, float hoverAlpha) {

    auto indicatorColor = (Theme::Colors::highlight).interpolatedWith(Theme::Colors::highlightHover, hoverAlpha);

    const float indicatorX = center.x + (indicatorRadius * std::sin(valueAngle));
    const float indicatorY = center.y - (indicatorRadius * std::cos(valueAngle));
    indicatorDotRadius = indicatorDotRadius + (indicatorDotRadius * hoverAlpha * 0.25f);
    g.setColour(indicatorColor);
    g.fillEllipse(indicatorX - indicatorDotRadius, indicatorY - indicatorDotRadius, indicatorDotRadius * 2.0f, indicatorDotRadius * 2.0f);
}

void RotaryLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, 
    float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) {

    bool isBipolar = false;
    float hoverAlpha = 0.0f;
    if (auto* rotarySlider = dynamic_cast<Rotary*>(&slider)) {
        isBipolar = rotarySlider->isBipolar();
        hoverAlpha = rotarySlider->hoverAlpha;
    }

    const float outerRadius = (static_cast<float>(juce::jmin(width, height)) * 0.5f);
    const juce::Point<float> center(
        static_cast<float>(x) + static_cast<float>(width) * 0.5f,
        static_cast<float>(y) + static_cast<float>(height) * 0.5f
    );

    const float valueAngle = juce::jmap(sliderPosProportional, rotaryStartAngle, rotaryEndAngle);

    // Draw layers
    /* UNUSED (maybe in the future)
    const int numTicks = 2;
    const float tickInset = 2.0f;
    const float tickRadius = outerRadius - tickInset;
    drawTicks(g, center, tickRadius, rotaryStartAngle, rotaryEndAngle, numTicks, hoverAlpha);
    */

    const float outlineInset = 10.0f;
    const float outlineThickness = 4.0f;
    const float outlineRadius = outerRadius - outlineInset;
    drawOutline(g, center, outlineRadius, outlineThickness, rotaryStartAngle, rotaryEndAngle, hoverAlpha);

    const float arcInset = outlineInset;
    const float arcThickness = 4.0f;
    const float arcRadius = outerRadius - arcInset;
    drawArc(g, center, arcRadius, arcThickness, rotaryStartAngle, rotaryEndAngle, valueAngle, hoverAlpha, isBipolar);

    const float indicatorInset = 5.0f;
    const float indicatorRadius = arcRadius - arcThickness - indicatorInset;
    const float indicatorDotRadius = outerRadius * 0.06f;
    drawIndicator(g, center, indicatorRadius, indicatorDotRadius, valueAngle, hoverAlpha);
}