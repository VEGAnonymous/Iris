#include "Core/Utilities.h"
#include "Core/Control/ConvolutionStateBuilder.h"

class IRFFTJob : public juce::ThreadPoolJob {
private:
    std::shared_ptr<ConvolutionIRBank> bank;
    juce::AudioBuffer<float> buffer;
    int index;

public:
    IRFFTJob(std::shared_ptr<ConvolutionIRBank> bank, juce::AudioBuffer<float> buf, int irIndex)
        : ThreadPoolJob("IR FFT " + juce::String(irIndex)),
        bank(std::move(bank)), buffer(std::move(buf)), index(irIndex) {
    }

    JobStatus runJob() override {
        if (shouldExit()) return jobHasFinished;
        bank->setIR(index, buffer);
        return jobHasFinished;
    }

    std::shared_ptr<ConvolutionIRBank> getBank() { return bank; }
    int getIRIndex() const { return index; }
};

/* PRIVATE */

bool ConvolutionStateBuilder::updateIRBank(const std::shared_ptr<ConvolutionState>& currentState, std::shared_ptr<ConvolutionState>& nextState) {
    std::shared_ptr<const ConvolutionIRBank> irBank = currentState->irBank;
    bool irChanged = !(setIRs.empty() && clearedIRs.empty() && activeChangedIRs.empty());
    if (irChanged) {
        if (!nextBank) nextBank = std::make_shared<ConvolutionIRBank>(*currentState->irBank);
        auto& nBank = nextBank;

        // Remove finished or requeued jobs
        {
            juce::SpinLock::ScopedLockType lock(fftJobLock);
            for (auto& fftJob : pendingJobs) {
                for (int irIndex : setIRs) {
                    if (fftJob.irIndex == irIndex) fftThreadPool.removeJob(fftJob.job, true, 0);
                }
            }

            pendingJobs.erase(
                std::remove_if(pendingJobs.begin(), pendingJobs.end(),
                    [this](const FFTJob& pending) {
                        return !fftThreadPool.contains(pending.job);
                    }),
                pendingJobs.end()
            );
        }

        // Load IRs
        if (!setIRs.empty()) {
            for (int irIndex : setIRs) {
                if (!validateIRIndex(irIndex)) continue;
                // DBG("Setting IR " << irIndex);
                {
                    juce::SpinLock::ScopedLockType lock(fftJobLock);
                    if (fftThreadPool.getNumJobs() >= MAX_IR_COUNT) continue;
                    bool jobPending = std::any_of(pendingJobs.begin(), pendingJobs.end(),
                        [irIndex](const FFTJob& job) { return job.irIndex == irIndex; });
                    if (jobPending) continue;
                }

                auto buffer = irManager.applyWindow(irIndex);
                auto* job = new IRFFTJob(nBank, std::move(buffer), irIndex);
                {
                    juce::SpinLock::ScopedLockType lock(fftJobLock);
                    pendingJobs.push_back({ irIndex, job });
                }
                fftThreadPool.addJob(job, true);

            }
        }

        // Clear IRs
        if (!clearedIRs.empty()) {
            for (int irIndex : clearedIRs) {
                if (!validateIRIndex(irIndex)) continue;
                // DBG("Clearing IR " << irIndex);
                nBank->clearIR(irIndex);
                nBank->updateMaxPartitionCount();
            }
        }

        // Set IR active states
        if (!activeChangedIRs.empty()) {
            for (int irIndex : activeChangedIRs) {
                if (!validateIRIndex(irIndex)) continue;
                // DBG("Setting IR active state " << irIndex);
                const auto& active = irManager.getIRSlot(irIndex).active;
                nBank->setIRActive(irIndex, active);
            }
        }

        // Update GUI state
        guiState.irChanged.store(true, std::memory_order_release);
        guiState.updatePosition.store(true, std::memory_order_release);
        guiState.updateField.store(true, std::memory_order_release);
        guiState.updateWeights.store(true, std::memory_order_release);

        // Jobs pending, defer mix
        irBank = currentState->irBank;
        jobsPending = true;
        irChanged = false;
    }

    // Unless no FFT jobs are active
    if (jobsPending && fftThreadPool.getNumJobs() == 0) {
        // Then swap in the new bank
        nextBank->updateMaxPartitionCount();
        irBank = nextBank;
        nextBank = nullptr;

        jobsPending = false;
        irChanged = true; // And allow mixing
        irManager.getFFTBusy().store(false, std::memory_order_release); // And allow pressing that Celestia-forsaken button again
    }

    nextState->irBank = irBank;
    return irChanged;
}

void ConvolutionStateBuilder::updateMixState(const std::shared_ptr<ConvolutionState>& currentState, std::shared_ptr<ConvolutionState>& nextState, 
    bool irChanged, float decay, const std::array<std::array<float, MAX_IR_COUNT>, N_CHANNELS>& irWeights) {

    auto& nextMixState = nextState->mixState;

    if (decayChanged || irChanged) nextMixState.setDecay(decay, nextState->irBank->getMaxPartitionCount());
    else nextMixState.irEnvelopes = currentState->mixState.irEnvelopes;

    if (weightsChanged) nextMixState.setWeights(irWeights);
    else nextMixState.irWeights = currentState->mixState.irWeights;

    if (weightsChanged || decayChanged || irChanged) nextMixState.mixSpectrum(*(nextState->irBank));
    else nextMixState.mixedSpectra = currentState->mixState.mixedSpectra;

    decayChanged = false;
    weightsChanged = false;
}

/* PUBLIC */

ConvolutionStateBuilder::ConvolutionStateBuilder(IRManager& irManager, GUIState& guiState) : irManager(irManager), guiState(guiState) {}

ConvolutionStateBuilder::~ConvolutionStateBuilder() { 
    fftThreadPool.removeAllJobs(false, 1000); 
}

void ConvolutionStateBuilder::pushIRUpdate(const IRUpdate& update) {
    switch (update.type) {
        case IRUpdate::IR_SET: {
            setIRs.push_back(update.irIndex);
            break;
        }
        case IRUpdate::IR_CLEAR: {
            clearedIRs.push_back(update.irIndex);
            break;
        }
        case IRUpdate::IR_ACTIVE_CHANGED: {
            activeChangedIRs.push_back(update.irIndex);
            break;
        }
    }
}

void ConvolutionStateBuilder::clearUpdates() {
    setIRs.clear();
    clearedIRs.clear();
    activeChangedIRs.clear();
}

void ConvolutionStateBuilder::notifyDecayChanged() { decayChanged = true; }
void ConvolutionStateBuilder::notifyWeightsChanged() { weightsChanged = true; }

std::shared_ptr<ConvolutionState> ConvolutionStateBuilder::build(const std::shared_ptr<ConvolutionState>& currentState,
    float decay, const std::array<std::array<float, MAX_IR_COUNT>, N_CHANNELS>& irWeights) {

    jassert(currentState && currentState->irBank);

    auto nextState = std::make_shared<ConvolutionState>();
    bool irChanged = updateIRBank(currentState, nextState);
    jassert(nextState->irBank);
    nextState->prepare();
    updateMixState(currentState, nextState, irChanged, decay, irWeights);

    return nextState;
}