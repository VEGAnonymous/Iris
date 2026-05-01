#pragma once

#include "Core/Processors/State/ConvolutionState.h"
#include "Core/Control/IRManager.h"
#include "Core/Control/State/GUIState.h"
#include "Core/Control/IRDefines.h"

#include <JuceHeader.h>
#include <vector>
#include <array>

class ConvolutionStateBuilder {
private:
	IRManager& irManager;
    GUIState& guiState;

    // Diffs
    bool decayChanged = false;
    bool weightsChanged = false;

    std::vector<int> setIRs;
    std::vector<int> clearedIRs;
    std::vector<int> activeChangedIRs;

    // Concurrency
    juce::SpinLock fftJobLock;

    struct FFTJob {
        int irIndex;
        juce::ThreadPoolJob* job;
    };

    juce::ThreadPool fftThreadPool;
    std::shared_ptr<ConvolutionIRBank> nextBank;

    std::vector<FFTJob> pendingJobs;
    bool jobsPending = false;

    // Build stages
    bool updateIRBank(const std::shared_ptr<ConvolutionState>& currentState, std::shared_ptr<ConvolutionState>& nextState);
    void updateMixState(const std::shared_ptr<ConvolutionState>& currentState, std::shared_ptr<ConvolutionState>& nextState,
        bool irChanged, float decay, const std::array<std::array<float, MAX_IR_COUNT>, N_CHANNELS>& irWeights);

public:
    ConvolutionStateBuilder(IRManager& irManager, GUIState& guiState);
    ~ConvolutionStateBuilder();

    void pushIRUpdate(const IRUpdate& update);
    void clearUpdates();

    void notifyDecayChanged();
    void notifyWeightsChanged();

    std::shared_ptr<ConvolutionState> build(const std::shared_ptr<ConvolutionState>& currentState,
        float decay, const std::array<std::array<float, MAX_IR_COUNT>, N_CHANNELS>& irWeights);
};