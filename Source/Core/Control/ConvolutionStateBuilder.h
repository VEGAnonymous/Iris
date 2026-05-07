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
    // State pool
    std::array<std::shared_ptr<ConvolutionState>, 2> statePool {};
    int stateIndex = 0;

    // Diffs
    bool decayChanged = false;
    bool weightsChanged = false;

    std::vector<int> setIRs;
    std::vector<int> clearedIRs;
    std::vector<int> activeChangedIRs;
    std::vector<int> gainedIRs;

    // Concurrency
    juce::SpinLock fftJobLock;

    struct FFTJob {
        int irIndex;
        juce::ThreadPoolJob* job;
    };

    juce::ThreadPool fftThreadPool { juce::jmax(1, juce::SystemStats::getNumCpus() / 3) };
    std::shared_ptr<ConvolutionIRBank> nextBank;

    std::vector<FFTJob> pendingJobs;
    bool jobsPending = false;

    // Crossfade
    std::array<CrossfadeSlot, MAX_IR_COUNT> crossfadeSlots {};

    // Build stages
    bool updateIRBank(const std::shared_ptr<ConvolutionState>& currentState, std::shared_ptr<ConvolutionState>& nextState, float crossfadeTime);
    void updateMixState(const std::shared_ptr<ConvolutionState>& currentState, std::shared_ptr<ConvolutionState>& nextState,
        bool irChanged, float decay, const std::array<std::array<float, MAX_IR_BANK_SLOTS>, N_CHANNELS>& irWeights);

public:
    ConvolutionStateBuilder(IRManager& irManager, GUIState& guiState);
    ~ConvolutionStateBuilder();

    void prepare();

    void pushIRUpdate(const IRUpdate& update);
    void clearUpdates();

    void notifyDecayChanged();
    void notifyWeightsChanged();

    void advanceCrossfades(float dt);

    std::shared_ptr<ConvolutionState> build(const std::shared_ptr<ConvolutionState>& currentState,
        float decay, const std::array<std::array<float, MAX_IR_BANK_SLOTS>, N_CHANNELS>& irWeights, float crossfadeTime);

    const CrossfadeSlot& getCrossfadeSlot(int index) const;
};