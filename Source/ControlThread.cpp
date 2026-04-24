#include "ControlThread.h"
#include "ParameterSettings.h"
#include "Utilities.h"

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

    float minDistance = juce::jmap(strength, 0.05f, 0.5f), maxWeight = 1.0f / (minDistance * minDistance), trim = 0.5f; // Absolute
    float distanceFactor = juce::jmap(strength * strength, 0.5f, 3.5f); // Relative

    // Compute inverse-distance weights
    auto relatives = polarMap.getRelatives();
    if (weightingMode == WeightingMode::WEIGHTING_RELATIVE) {
        for (int ir = 0; ir < MAX_IR_COUNT; ++ir) distanceWeights[ir] = 1.0f / powf(relatives[ir].r + EPSILON, distanceFactor);
        float sum = std::accumulate(distanceWeights.begin(), distanceWeights.end(), 0.0f);
        if (sum > 0.0f) for (int ir = 0; ir < MAX_IR_COUNT; ++ir) distanceWeights[ir] /= sum; // Normalize weights sum to 1
    }
    else { // WeightingMode::WEIGHTING_ABSOLUTE
        for (int ir = 0; ir < MAX_IR_COUNT; ++ir) {
            float d = std::max(relatives[ir].r, minDistance);
            distanceWeights[ir] = ((1.0f / (d * d)) / maxWeight) * trim;
        }
    }

    processBinaural(distanceWeights, relatives); // Inject binaural cues, mutate irWeights
}

std::shared_ptr<ConvolutionState> ControlThread::buildConvolutionState() {
    auto currentState = std::atomic_load(&convolutionState->state);
    jassert(currentState && currentState->irBank);
    auto nextState = std::make_shared<ConvolutionState>();

    // DBG("Building convolution state");

    // Get flags
    std::deque<int> irsSet, irsCleared, irsActiveStateSet;
    bool decayChanged, weightsChanged;
    {
        juce::SpinLock::ScopedLockType lock(flagLock);

        IRChanges irChanges = irManager.consumeIRChanges();
        irsSet = std::move(irChanges.irsSet);
        irsCleared = std::move(irChanges.irsCleared);
        irsActiveStateSet = std::move(irChanges.irsActiveStateSet);
        decayChanged = stateFlags.decayChanged;
        weightsChanged = stateFlags.weightsChanged;

        stateFlags.resetFlags();
    }

    // IR state
    std::shared_ptr<const ConvolutionIRBank> irBank = currentState->irBank;
    bool irChanged = !(irsSet.empty() && irsCleared.empty() && irsActiveStateSet.empty());
    if (irChanged) {
        auto nBank = std::make_shared<ConvolutionIRBank>(*currentState->irBank);

        std::vector<std::future<void>> irJobs; // Parallelize

        while (!irsSet.empty()) {
            // loadIR()
            int irIndex = irsSet.front();
            irsSet.pop_front();
            if (validateIRIndex(irIndex)) {
                DBG("Setting IR " << irIndex);
                auto buffer = irManager.applyWindow(irIndex);
                irJobs.push_back(std::async(std::launch::async,
                    [&nBank, buf = std::move(buffer), irIndex]() { nBank->setIR(irIndex, buf); }
                ));
            }
        }

        while (!irsCleared.empty()) {
            // clearIR()
            int irIndex = irsCleared.front();
            irsCleared.pop_front();
            if (validateIRIndex(irIndex)) {
                DBG("Clearing IR " << irIndex);
                nBank->clearIR(irIndex);
            }
        }

        while (!irsActiveStateSet.empty()) {
            // activateIR()
            int irIndex = irsActiveStateSet.front();
            irsActiveStateSet.pop_front();
            if (validateIRIndex(irIndex)) {
                DBG("Setting IR active state " << irIndex);
                const auto& active = irManager.getIRSlot(irIndex).active;
                nBank->setIRActive(irIndex, active);
            }
        }

        for (auto& job : irJobs) job.get();
        nBank->updateMaxPartitionCount();

        guiState.irChanged.store(true, std::memory_order_release);
        guiState.updatePosition.store(true, std::memory_order_release);
        guiState.updateField.store(true, std::memory_order_release);

        irBank = nBank;
    }

    nextState->irBank = irBank;
    jassert(nextState->irBank);
    nextState->prepare();

    // Mix state
    if (decayChanged || irChanged)
        nextState->mixState.setDecay(decay, nextState->irBank->getMaxPartitionCount());
    else
        nextState->mixState.irEnvelopes = currentState->mixState.irEnvelopes;

    if (weightsChanged)
        nextState->mixState.setWeights(irWeights);
    else
        nextState->mixState.irWeights = currentState->mixState.irWeights;

    if (weightsChanged || decayChanged || irChanged)
        nextState->mixState.mixSpectrum(*nextState->irBank);
    else {
        nextState->mixState.mixedSpectra = currentState->mixState.mixedSpectra;
    }

    {
        juce::SpinLock::ScopedLockType lock(flagLock);
        stateFlags.resetFlags();
    }

    return nextState;
}

std::shared_ptr<ConvolutionState> ControlThread::runControlCycle(float dt) {
    advancePhase(dt);

    if (!guiState.syncingSwap.load(std::memory_order_acquire)) irManager.advanceSwapTimers(dt);

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
        juce::SpinLock::ScopedLockType lock(flagLock);
        stateFlags.decayChanged = true;
    }

    // Weights
    if (motionController.hasPositionUpdated() || motionController.hasFieldUpdated()) {
        polarMap.computeRelatives();
        updateWeights();
        juce::SpinLock::ScopedLockType lock(flagLock);
        stateFlags.weightsChanged = true;
    }

    // Build state
    return buildConvolutionState();
}

/* PUBLIC */

ControlThread::ControlThread(const juce::AudioProcessorValueTreeState& a, IRManager& m, GUIState& g, std::shared_ptr<ConvolutionStateHolder> c) :
    juce::Thread("Control Thread"), 
    motionController(&polarMap, &positionTime, &fieldTime),
    irManager(m), 
    apvts(a),
    guiState(g),
    convolutionState(c) {}

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
        // DBG("Stored state");

        // eepy
        auto elapsedTime = std::chrono::steady_clock::now() - startTime;
        // DBG("Control cycle: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count() << " ms");
        auto remainingTime = std::chrono::duration<double>(1.0f / CONTROL_RATE) - elapsedTime;
        // DBG("Headroom: " << std::chrono::duration_cast<std::chrono::milliseconds>(remainingTime).count() << " ms");
        if (remainingTime > std::chrono::duration<double>::zero()) 
            wait(static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(remainingTime).count()));
    }
}