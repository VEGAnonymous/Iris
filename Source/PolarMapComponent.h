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
    CartesianCoordinate inverseMap(CartesianCoordinate p) const;
    BoundsF toBounds(PolarCoordinate p, float radius) const;

    // Parametric path
    juce::Path parametricPath;
    bool pathChanged = true;
    
    void repaintPath();

    // Position indicator
    PolarCoordinate currentPosition {0.0f, 0.0f};
    static constexpr float positionRadius = 4.0f;
    BoundsF positionBounds {};

    // Field indicators
    std::vector<PolarCoordinate> fieldCoordinates {};
    static constexpr float coordinateRadius = 6.0f;

    // Dragging
    enum class DragTarget { NONE, POSITION, FIELD };
    DragTarget dragTarget { DragTarget::NONE };

    int fieldIndex { -1 };
    static constexpr float hitRadius = 6.0f;

    bool hitPosition(juce::Point<float> p) const;
    int hitField(juce::Point<float> p) const;

    std::atomic<bool> switchedIR { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolarMapComponent)

public:
    PolarMapComponent(MareverbAudioProcessor& processor);
    ~PolarMapComponent() override = default;

    void paint(juce::Graphics& g) override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    std::atomic<bool>& getIRSwitched();

    // Callbacks
    void notifyPathChanged();
    void notifyPositionChanged(PolarCoordinate nPosition);
    void notifyFieldChanged(std::vector<PolarCoordinate> nCoordinates);
};