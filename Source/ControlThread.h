#pragma once

#include "ConvolutionState.h"
#include "GUIState.h"
#include "IRManager.h"
#include "MotionController.h"
#include "PolarMap.h"

#include <JuceHeader.h>
#include <chrono>

struct ConvolutionStateFlags {
    bool decayChanged = false;
    bool weightsChanged = false;

    void resetFlags() {
        decayChanged = false;
        weightsChanged = false;
    }
};

class ControlThread : public juce::Thread {
private:
    const juce::AudioProcessorValueTreeState& apvts;
    IRManager& irManager;
    GUIState& guiState;

    // Motion state
    PolarMap polarMap;
    MotionController motionController;

    // Convolution state
    ConvolutionStateFlags stateFlags;
    std::shared_ptr<ConvolutionStateHolder> convolutionState;
    
    std::shared_ptr<ConvolutionState> buildConvolutionState();

    float decay = -1.0f;

    // Time
    float positionTime = 0.0f;
    float fieldTime = 0.0f;
    std::chrono::steady_clock::time_point lastTick;
    void advancePhase(float dt);

    // Weights
    void processBinaural(const std::array<float, MAX_IR_COUNT>& rawWeights, const std::vector<PolarCoordinate>& relatives);
    void updateWeights();
    std::array<std::array<float, MAX_IR_COUNT>, N_CHANNELS> irWeights {}; // Local copy

    // Concurrency
    juce::SpinLock flagLock;
    std::shared_ptr<ConvolutionState> runControlCycle(float dt);

public:
    ControlThread(const juce::AudioProcessorValueTreeState& a, IRManager& m, GUIState& g, std::shared_ptr<ConvolutionStateHolder> c);
    ~ControlThread() = default;

    void updateMotionParameters();

    void run() override;
};