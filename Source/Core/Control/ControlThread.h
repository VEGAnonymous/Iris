#pragma once

#include "Core/Processors/State/ConvolutionState.h"
#include "Core/Control/State/GUIState.h"
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

    // Motion state
    PolarMap polarMap;
    MotionController motionController;

    // Convolution state
    bool decayChanged = false;
    bool weightsChanged = false;

    std::vector<int> setIRs;
    std::vector<int> clearedIRs;
    std::vector<int> activeChangedIRs;

    std::shared_ptr<ConvolutionStateHolder> convolutionState;

    bool updateIRBank(const std::shared_ptr<ConvolutionState>& currentState, std::shared_ptr<ConvolutionState>& nextState);
    void updateMixState(const std::shared_ptr<ConvolutionState>& currentState, std::shared_ptr<ConvolutionState>& nextState, bool irChanged);
    
    std::shared_ptr<ConvolutionState> buildConvolutionState();

    // Time
    float positionTime = 0.0f;
    float fieldTime = 0.0f;
    std::chrono::steady_clock::time_point lastTick;
    void advancePhase(float dt);

    // Weights
    void processBinaural(const std::array<float, MAX_IR_COUNT>& rawWeights, const std::vector<PolarCoordinate>& relatives);
    void updateWeights();

    float decay = -1.0f;
    std::array<std::array<float, MAX_IR_COUNT>, N_CHANNELS> irWeights {}; // Local copy

    // Concurrency
    juce::SpinLock fftJobLock;

    struct FFTJob {
        int irIndex;
        juce::ThreadPoolJob* job;
    };

    juce::ThreadPool fftThreadPool ;
    std::shared_ptr<ConvolutionIRBank> nextBank;

    std::vector<FFTJob> pendingJobs;
    bool jobsPending = false;

    void processIRCommands();
    void processIRResults();
    void processIRUpdates();

    // Control cycle
    void updateMotionParameters();
    void updateSwapParameters();

    std::shared_ptr<ConvolutionState> runControlCycle(float dt);

public:
    ControlThread(const juce::AudioProcessorValueTreeState& apvts, IRManager& manager, 
        GUIState& gState, std::shared_ptr<ConvolutionStateHolder> cState);
    ~ControlThread() = default;

    void run() override;
};