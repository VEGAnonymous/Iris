#pragma once

#include "Core/Processors/State/ConvolutionState.h"
#include "Core/Control/State/GUIState.h"
#include "Core/Control/ConvolutionStateBuilder.h"
#include "Core/Control/IRManager.h"
#include "Core/Control/MotionController.h"
#include "Core/Control/State/PolarMap.h"

#include <JuceHeader.h>
#include <chrono>

class ControlThread : public juce::Thread {
private:
    const juce::AudioProcessorValueTreeState& apvts;
    IRManager& irManager;
    GUIState& guiState;

    // Time
    float positionTime = 0.0f;
    float fieldTime = 0.0f;
    std::chrono::steady_clock::time_point lastTick;

    void advancePhase(float dt);

    // Parameters
    void updateSwapParameters();

    // Motion state
    PolarMap polarMap;
    MotionController motionController;

    void updateMotionParameters();

    // Weights
    float decay = -1.0f;
    std::array<std::array<float, MAX_IR_COUNT>, N_CHANNELS> irWeights{}; // Local copy

    void processBinaural(const std::array<float, MAX_IR_COUNT>& rawWeights, const std::vector<PolarCoordinate>& relatives);
    void updateWeights();

    // Convolution state
    ConvolutionStateBuilder convolutionStateBuilder;
    std::shared_ptr<ConvolutionStateHolder> convolutionState;

    void updateConvolutionState();

    // Concurrency
    void processIRCommands();
    void processIRResults();
    void processIRUpdates();

    // Control cycle
    float controlRate = 30.0f;
    juce::SpinLock controlLock;

    void runControlCycle(float dt);

public:
    ControlThread(const juce::AudioProcessorValueTreeState& apvts, IRManager& manager, 
        GUIState& gState, std::shared_ptr<ConvolutionStateHolder> cState);
    ~ControlThread() override = default;

    void setControlRate(float nRate);

    void run() override;
};