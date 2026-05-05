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
    static constexpr auto MAP_INSET = 18;
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

    // Position indicator
    PolarCoordinate currentPosition {0.0f, 0.0f};
    const float basePositionRadius = 4.0f;
    float positionRadius = 4.0f;
    BoundsF positionBounds {};

    juce::Image positionIndicatorIcon {};

    // Field indicators
    std::vector<PolarCoordinate> fieldCoordinates {};
    const float baseCoordinateRadius = 6.0f;
    float coordinateRadius = 6.0f;

    std::array<juce::Image, MAX_IR_COUNT> fieldIndicatorIcons {};

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
    void notifyPathChanged();
    void notifyPositionChanged(PolarCoordinate nPosition);
    void notifyFieldChanged(std::vector<PolarCoordinate> nCoordinates, bool animate = true);
    void notifyIndicatorStyleChanged();

    std::atomic<bool>& getIRSwitched();
};