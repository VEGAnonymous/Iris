#pragma once

#include "Core/Defines.h"
#include "Core/Utilities.h"
#include "Core/Control/MotionController.h"
#include "GUI/API/AnimatedValue.h"

#include <JuceHeader.h>
#include <array>

class MareverbAudioProcessor;

class PolarMapPanel : public juce::Component {
private:
    static constexpr auto MAP_INSET = 62;
    static constexpr auto HIT_RADIUS = 6.0f;

    MareverbAudioProcessor& audioProcessor;
    juce::AnimatorUpdater& animatorUpdater;

    // Utilities
    CartesianCoordinate map(CartesianCoordinate p) const;
    CartesianCoordinate inverseMap(CartesianCoordinate p) const;
    BoundsF toBounds(PolarCoordinate p, float radius) const;

    // Animation
    std::vector<AnimatedValue> fieldActiveStates;

    AnimatedValue positionInteractionState;
    std::vector<AnimatedValue> fieldInteractionStates;

    std::vector<AnimatedValue> fieldSelectionStates;

    std::array<bool, MAX_IR_COUNT> lastCrossfadeActives {};

    // Hovering
    struct HoverState {
        bool hoveringPosition = false;
        int fieldIndex = -1;
    };

    bool hitPosition(juce::Point<float> p) const;
    int hitField(juce::Point<float> p) const;

    HoverState getHoverState(juce::Point<float> p) const;
    void applyHoverState(const HoverState& hoverState);

    // Dragging
    enum class DragTarget { NONE, POSITION, FIELD };
    DragTarget dragTarget{ DragTarget::NONE };

    // State
    std::atomic<bool> switchedIR { false };

    // Parametric path
    juce::Path parametricPath;
    bool pathChanged = true;
    
    void repaintPath();

    // Polar map
    PolarMap polarMap;

    // Position indicator
    const float basePositionRadius = 8.0f;
    float positionRadius = 8.0f;
    BoundsF positionBounds {};

    juce::Image positionIndicatorIcon {};
    juce::String positionIndicatorStyle;

    // Field indicators
    const float baseCoordinateRadius = 6.0f;
    float coordinateRadius = 6.0f;
    std::array<float, MAX_IR_COUNT> coordinateWeights {};
    float weightScale = 1.0f;

    std::array<juce::Image, MAX_IR_COUNT> incomingMares {}; // Incoming mares
    std::array<juce::Image, MAX_IR_SLOT_PAIRS> fieldIndicatorIcons {}; // Outgoing mares
    juce::String fieldIndicatorStyle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolarMapPanel)

public:
    PolarMapPanel(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater);
    ~PolarMapPanel() override = default;

    void paint(juce::Graphics& g) override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    // Updates
    void updatePath();
    void updatePosition();
    void updateField(bool animate = true);
    void setIndicatorStyle();

    void updatePositionIndicator();
    void updateFieldIndicator(int irIndex);
    void updateIndicators();
    void updateWeights();
    void syncCrossfade(int irIndex, bool crossfadeActive, bool crossfadeWasActive);


    std::atomic<bool>& getIRSwitched();
};