#include "Core/Control/IRDefines.h"
#include "Core/Control/ControlThread.h"
#include "Core/Control/State/ParameterSettings.h"
#include "Core/Utilities.h"

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

void ControlThread::advancePhase(float dt) {
    constexpr float positionRateScale = 0.2f, fieldRateScale = 0.05f;

    const float positionFreq = apvts.getRawParameterValue(ParamID::positionRate)->load() * positionRateScale;
    const float fieldFreq = apvts.getRawParameterValue(ParamID::fieldRate)->load() * fieldRateScale;

    const float positionPhaseIncrement = juce::MathConstants<float>::twoPi * positionFreq * dt;
    const float fieldPhaseIncrement = juce::MathConstants<float>::twoPi * fieldFreq * dt;

    positionTime += positionPhaseIncrement;
    fieldTime += fieldPhaseIncrement;
}

// Binaural processing

void ControlThread::processBinaural(const std::array<float, MAX_IR_COUNT>& rawWeights,
    const std::vector<PolarCoordinate>& relatives) {
    const float spread = std::powf(apvts.getRawParameterValue(ParamID::spread)->load(), 1.5f);
    for (int ir = 0; ir < MAX_IR_COUNT; ++ir) {
        const float& azimuth = relatives[ir].theta;

        // ILD
        const float pan = std::sinf(azimuth);
        const float gainFB = 0.75f + (0.25f * std::cosf(azimuth)); // Front-back
        float gainL = std::sqrtf(0.5f * (1.0f - pan)) * gainFB; // L-R
        float gainR = std::sqrtf(0.5f * (1.0f + pan)) * gainFB;

        gainL = juce::jmap(spread, 1.0f, gainL);
        gainR = juce::jmap(spread, 1.0f, gainR);

        irWeights[0][ir] = rawWeights[ir] * gainL;
        irWeights[1][ir] = rawWeights[ir] * gainR;
    }
}

// Convolution state

void ControlThread::updateWeights() {
    std::array<float, MAX_IR_COUNT> distanceWeights{};

    // Parameterize
    WeightingMode weightingMode = static_cast<WeightingMode>(apvts.getRawParameterValue(ParamID::weightingMode)->load());
    const float& strength = apvts.getRawParameterValue(ParamID::strength)->load();

    const float minDistance = juce::jmap(strength, 0.05f, 0.5f), maxWeight = 1.0f / (minDistance * minDistance), trim = 0.5f; // Absolute
    const float distanceFactor = juce::jmap(strength * strength, 0.5f, 3.5f), contrast = juce::jmap(strength, 0.8f, 2.5f); // Relative

    // Compute inverse-distance weights
    auto relatives = polarMap.getRelatives();
    if (weightingMode == WeightingMode::WEIGHTING_RELATIVE) {
        float sum = 0.0f;
        for (int i = 0; i < MAX_IR_COUNT; ++i) {
            const auto& slot = irManager.getIRSlot(i);
            if (!slot.active || !slot.occupied) { // Inactive IRs don't contribute
                distanceWeights[i] = 0.0f;
                continue;
            }

            float weight = 1.0f / powf(relatives[i].r + EPSILON, distanceFactor);
            weight = powf(weight, contrast);
            distanceWeights[i] = weight;
            sum += weight;
        }

        // Normalize active weights sum to 1
        if (sum > 0.0f) {
            for (int i = 0; i < MAX_IR_COUNT; ++i) {
                const auto& slot = irManager.getIRSlot(i);
                if (slot.active || slot.occupied) {
                    distanceWeights[i] /= sum;
                }
            }
        }
    } else { // WeightingMode::WEIGHTING_ABSOLUTE
        for (int i = 0; i < MAX_IR_COUNT; ++i) {
            float d = std::max(relatives[i].r, minDistance);
            distanceWeights[i] = ((1.0f / (d * d)) / maxWeight) * trim;
        }
    }

    processBinaural(distanceWeights, relatives); // Inject binaural cues, mutate irWeights
}

bool ControlThread::updateIRBank(const std::shared_ptr<ConvolutionState>& currentState, std::shared_ptr<ConvolutionState>& nextState) {
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
                {
                    juce::SpinLock::ScopedLockType lock(guiState.irWaveformLock);
                    guiState.irWaveforms[irIndex] = buffer;
                }

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

void ControlThread::updateMixState(const std::shared_ptr<ConvolutionState>& currentState, std::shared_ptr<ConvolutionState>& nextState, bool irChanged) {
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

std::shared_ptr<ConvolutionState> ControlThread::buildConvolutionState() {
    auto currentState = std::atomic_load(&convolutionState->state);
    jassert(currentState && currentState->irBank);

    auto nextState = std::make_shared<ConvolutionState>();
    bool irChanged = updateIRBank(currentState, nextState);
    jassert(nextState->irBank);
    nextState->prepare();
    updateMixState(currentState, nextState, irChanged);

    return nextState;
}

void ControlThread::updateMotionParameters() {
    ParameterSettings settings = getParameterSettings(apvts);

    if (!guiState.syncingPosition.load(std::memory_order_acquire)) {
        motionController.setPositionParameters({
            settings.positionPattern,
            settings.positionRate,
            settings.positionModA,
            settings.positionModB
            });
    }

    if (!guiState.syncingField.load(std::memory_order_acquire)) {
        motionController.setFieldParameters({
            MAX_IR_COUNT,
            apvts.state.getProperty(PropertyID::selectedIR),
            settings.fieldPattern,
            settings.fieldRate,
            settings.fieldModA,
            settings.fieldModB
            });
    }
}

void ControlThread::updateSwapParameters() {
    for (int ir = 0; ir < MAX_IR_COUNT; ++ir) {
        float nMin = apvts.getRawParameterValue(ParamID::irSwapMin(ir))->load();
        float nMax = apvts.getRawParameterValue(ParamID::irSwapMax(ir))->load();
        irManager.setSwapInterval(ir, nMin, nMax);
    }
}

void ControlThread::processIRCommands() {
    IRCommand cmd;
    auto* commandQueue = irManager.getCommandQueue();
    while (commandQueue->try_dequeue(cmd)) {
        switch (cmd.type) {
        case IRCommand::IR_LOAD: {
            const int irIndex = cmd.irIndex;
            const juce::File file = cmd.irFile;
            irManager.irThreadPool.addJob(
                [this, irIndex, file]() {
                    juce::AudioBuffer<float> irBuffer = irManager.readIR(file);
                    irManager.getResultQueue()->enqueue(
                        IRResult { irIndex, file, std::move(irBuffer) }
                    );
                }
            );
            break;
        }
        case IRCommand::IR_LOAD_RANDOM: {
            if (irManager.getIRDirectories().empty()) return;

            const int irIndex = cmd.irIndex;
            const auto randomFile = irManager.sampleRandomIR();
            irManager.irThreadPool.addJob(
                [this, irIndex, randomFile]() {
                    juce::AudioBuffer<float> irBuffer = irManager.readIR(randomFile);
                    irManager.getResultQueue()->enqueue(
                        IRResult { irIndex, randomFile, std::move(irBuffer) }
                    );
                }
            );
            break;
        }
        case IRCommand::IR_LOAD_RANDOM_ALL: {
            if (irManager.getIRDirectories().empty()) return;

            for (int i = 0; i < MAX_IR_COUNT; ++i) {
                const int irIndex = i;
                const auto randomFile = irManager.sampleRandomIR();
                irManager.getFFTBusy().store(true, std::memory_order_release);
                irManager.irThreadPool.addJob(
                    [this, irIndex, randomFile]() {
                        juce::AudioBuffer<float> irBuffer = irManager.readIR(randomFile);
                        irManager.getResultQueue()->enqueue(
                            IRResult{ irIndex, randomFile, std::move(irBuffer) }
                        );
                    }
                );
            }
            break;
        }
        case IRCommand::IR_CLEAR: {
            irManager.clearIR(cmd.irIndex);
            break;
        }
        case IRCommand::IR_CLEAR_ALL: {
            irManager.clearIRs();
            break;
        }
        case IRCommand::IR_SET_ACTIVE_STATE: {
            irManager.setIRActive(cmd.irIndex, cmd.irActiveState);
            break;
        }
        case IRCommand::IR_SET_WINDOW: {
            irManager.setWindow(cmd.irIndex, cmd.windowStart, cmd.windowLength);
            break;
        }
        case IRCommand::IR_SET_ENVELOPE: {
            irManager.setEnvelope(cmd.irIndex, cmd.envelopeType, cmd.envelopeAttack, cmd.envelopeRelease);
            break;
        }
        case IRCommand::IR_SET_SWAP_ACTIVE: {
            irManager.setSwapActive(cmd.irIndex, cmd.swapActiveState);
            break;
        }
        case IRCommand::IR_SET_SWAP_INTERVAL: {
            irManager.setSwapInterval(cmd.irIndex, cmd.swapMinTime, cmd.swapMaxTime);
            break;
        }
        case IRCommand::IR_DIRECTORY_ADD: {
            irManager.addIRDirectory(cmd.irDirectory);
            break;
        }
        case IRCommand::IR_DIRECTORY_REMOVE: {
            irManager.removeIRDirectory(cmd.irDirectoryIndex);
            break;
        }
        case IRCommand::IR_DIRECTORY_SET_ACTIVE_STATE: {
            irManager.setIRDirectoryActive(cmd.irDirectoryIndex, cmd.irDirectoryActiveState);
            break;
        }
        case IRCommand::IR_DIRECTORY_REFRESH: {
            irManager.collectIRs();
            break;
        }
        case IRCommand::SET_SAMPLING_MODE: {
            irManager.setSamplingMode(cmd.samplingMode);
            break;
        }
        default:
            continue;
        }
    }
}

void ControlThread::processIRResults() {
    IRResult result;
    auto* resultQueue = irManager.getResultQueue();

    auto startTime = std::chrono::steady_clock::now();
    const int deadlineMs = 5;

    while (resultQueue->try_dequeue(result)) {
        irManager.setIR(result.irIndex, result.file, result.buffer);
        if ((std::chrono::steady_clock::now() - startTime) > std::chrono::milliseconds(deadlineMs)) break;
    }
}

void ControlThread::processIRUpdates() {
    setIRs.clear();
    clearedIRs.clear();
    activeChangedIRs.clear();

    IRUpdate update;
    auto* updateQueue = irManager.getUpdateQueue();

    while (updateQueue->try_dequeue(update)) {
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
}

std::shared_ptr<ConvolutionState> ControlThread::runControlCycle(float dt) {
    advancePhase(dt);

    // Swap timers
    updateSwapParameters();
    if (!guiState.syncingSwap.load(std::memory_order_acquire)) {
        irManager.advanceSwapTimers(dt);
    }

    // Position + field
    updateMotionParameters();
    motionController.updateField();
    motionController.updatePosition();

    // Pass state to GUI
    if (motionController.hasPositionUpdated() || guiState.updatePosition.exchange(false, std::memory_order_acquire)) {
        guiState.position.store(polarMap.getPosition(), std::memory_order_release);
        guiState.positionChanged.store(true, std::memory_order_release);
    }

    if (motionController.hasFieldUpdated() || guiState.updateField.exchange(false, std::memory_order_acquire)) {
        juce::SpinLock::ScopedLockType lock(guiState.fieldLock);
        guiState.fieldCoordinates = polarMap.getCoordinates();
        guiState.fieldChanged.store(true, std::memory_order_release);
    }

    // Decay
    float nDecay = apvts.getRawParameterValue(ParamID::decay)->load();
    if (nDecay != decay) {
        decay = nDecay;
        decayChanged = true;
    }

    // Weights
    if (motionController.hasPositionUpdated() || motionController.hasFieldUpdated() 
        || guiState.updateWeights.exchange(false, std::memory_order_acquire)) {

        polarMap.computeRelatives();
        updateWeights();
        weightsChanged = true;
    }

    // Process IR state
    processIRCommands();
    processIRResults();
    processIRUpdates();

    // Build final state
    return buildConvolutionState();
}

/* PUBLIC */

ControlThread::ControlThread(const juce::AudioProcessorValueTreeState& apvts, IRManager& manager, 
    GUIState& gState, std::shared_ptr<ConvolutionStateHolder> cState) : juce::Thread("Control Thread"),
    apvts(apvts), irManager(manager), 
    guiState(gState), convolutionState(cState),
    motionController(&polarMap, &positionTime, &fieldTime),
    fftThreadPool() {}

ControlThread::~ControlThread() {
    fftThreadPool.removeAllJobs(false, 1000);
}

void ControlThread::run() {
    lastTick = std::chrono::steady_clock::now();
    while (!threadShouldExit()) {
        auto startTime = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(startTime - lastTick).count();
        lastTick = startTime;
        // DBG("dt: " << dt);

        // Update state
        auto nextState = runControlCycle(dt);
        std::atomic_store(&convolutionState->state, nextState);

        // eepy
        auto elapsedTime = std::chrono::steady_clock::now() - startTime;
        // DBG("Control cycle: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count() << " ms");
        auto remainingTime = std::chrono::duration<double>(1.0f / CONTROL_RATE) - elapsedTime;
        auto headroomMs = std::chrono::duration_cast<std::chrono::milliseconds>(remainingTime).count();
        if (headroomMs < 0) DBG("Control deadline(s) missed: " << headroomMs << " ms");

        if (remainingTime > std::chrono::duration<double>::zero()) 
            wait(static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(remainingTime).count()));
    }
}