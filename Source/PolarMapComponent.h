#pragma once

#include "Defines.h"
#include "MotionController.h"
#include "Utilities.h"

#include <JuceHeader.h>

class MareverbAudioProcessor;

class PolarMapComponent : public juce::Component {
private:
    MareverbAudioProcessor& audioProcessor;

    CartesianCoordinate map(CartesianCoordinate p) const;
    juce::Rectangle<float> toBounds(PolarCoordinate p, float radius) const;

    // Parametric path
    juce::Path parametricPath;
    bool pathChanged = true;
    
    void repaintPath();

    // Position indicator
    PolarCoordinate currentPosition {0.0f, 0.0f};
    static constexpr float positionRadius = 4.0f;
    juce::Rectangle<float> positionBounds {};

    // Field indicators
    std::vector<PolarCoordinate> fieldCoordinates {};
    static constexpr float coordinateRadius = 6.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolarMapComponent)

public:
    PolarMapComponent(MareverbAudioProcessor& processor);
    ~PolarMapComponent() override = default;

    void paint(juce::Graphics& g) override;

    // Callbacks
    void notifyPathChanged();
    void notifyPositionChanged(PolarCoordinate nPosition);
    void notifyFieldChanged(std::vector<PolarCoordinate> nCoordinates);
};