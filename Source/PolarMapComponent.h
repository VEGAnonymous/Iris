#pragma once

#include "Defines.h"
#include "MotionController.h"
#include "Utilities.h"

#include <JuceHeader.h>

class MareverbAudioProcessor;

class PolarMapComponent : public juce::Component {
public:
    PolarMapComponent(MareverbAudioProcessor& processor);
    ~PolarMapComponent() override = default;

    void paint(juce::Graphics& g) override;

    // Callbacks
    void notifyPathChanged();
    void notifyPositionChanged(PolarCoordinate newPosition);

private:
    MareverbAudioProcessor& audioProcessor;

    CartesianCoordinate map(CartesianCoordinate p) const;

    // Parametric path
    juce::Path parametricPath;
    bool pathChanged = true;
    
    void buildPath();

    // Position indicator
    PolarCoordinate currentPosition {0.0f, 0.0f};
    static constexpr float indicatorRadius = 4.0f;
    juce::Rectangle<float> indicatorBounds {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolarMapComponent)
};