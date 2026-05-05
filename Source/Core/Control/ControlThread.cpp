#include "Core/Control/IRDefines.h"
#include "Core/Control/ControlThread.h"
#include "Core/Control/State/ParameterSettings.h"
#include "Core/Utilities.h"

/* PRIVATE */

// Time

void ControlThread::advancePhase(float dt) {
    constexpr float positionRateScale = 0.1f, fieldRateScale = 0.035f;

    const float positionFreq = apvts.getRawParameterValue(ParamID::positionRate)->load() * positionRateScale;
    const float fieldFreq = apvts.getRawParameterValue(ParamID::fieldRate)->load() * fieldRateScale;

    const float positionPhaseIncrement = juce::MathConstants<float>::twoPi * positionFreq * dt;
    const float fieldPhaseIncrement = juce::MathConstants<float>::twoPi * fieldFreq * dt;

    positionTime += positionPhaseIncrement;
    fieldTime += fieldPhaseIncrement;
}

// Parameters

void ControlThread::updateSwapParameters() {
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        float nMin = apvts.getRawParameterValue(ParamID::irSwapMin(i))->load();
        float nMax = apvts.getRawParameterValue(ParamID::irSwapMax(i))->load();
        bool nActive = apvts.getRawParameterValue(ParamID::irSwapActive(i))->load();
        irManager.setSwapInterval(i, nMin, nMax);
        irManager.setSwapActive(i, nActive);
    }
}

// Motion state

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

// Weights

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

// Convolution state

void ControlThread::updateConvolutionState() {
    auto currentState = std::atomic_load(&convolutionState->state);
    auto nextState = convolutionStateBuilder.build(currentState, decay, irWeights);
    std::atomic_store(&convolutionState->state, nextState);
}

// Concurrency

void ControlThread::processIRCommands() {
    auto canLoad = [this]() {
        auto& irDirectoryFiles = *(irManager.getIRDirectoryFiles());
        bool anyLoadable = false;
        for (const auto& directoryFiles : irDirectoryFiles) {
            if (directoryFiles.dir.active && !directoryFiles.files.isEmpty()) {
                anyLoadable = true;
                break;
            }
        }
        return anyLoadable;
    };

    auto loadIR = [this](int irIndex, const juce::File file) {
        juce::AudioBuffer<float> irBuffer = irManager.readIR(file);

        auto waveformBuffer = std::make_shared<juce::AudioBuffer<float>>(irBuffer);
        {
            juce::SpinLock::ScopedLockType lock(guiState.irWaveformLock);
            guiState.irWaveforms[irIndex] = std::move(waveformBuffer);
        }
        {
            juce::SpinLock::ScopedLockType lock(guiState.mareLock);
            guiState.mareImages[irIndex] =
                Theme::Mares::getMare<Theme::Mares::MainMares>(file.getFileNameWithoutExtension());
        }

        guiState.indicatorStyleChanged.store(true, std::memory_order_release);
        irManager.getResultQueue()->enqueue(
            IRResult{ irIndex, file, std::move(irBuffer) }
        );
    };


    IRCommand cmd;
    auto* commandQueue = irManager.getCommandQueue();
    while (commandQueue->try_dequeue(cmd)) {
        switch (cmd.type) {
        case IRCommand::IR_LOAD: {
            const int irIndex = cmd.irIndex;
            const juce::File file = cmd.irFile;
            irManager.irThreadPool.addJob([this, loadIR, irIndex, file]() { loadIR(irIndex, file); });
            break;
        }
        case IRCommand::IR_LOAD_RANDOM: {
            if (!canLoad()) return;
            const int irIndex = cmd.irIndex;
            const auto randomFile = irManager.sampleRandomIR();
            if (!randomFile.existsAsFile()) return;
            irManager.irThreadPool.addJob([this, loadIR, irIndex, randomFile]() { loadIR(irIndex, randomFile); });
            break;
        }
        case IRCommand::IR_LOAD_RANDOM_ALL: {
            if (!canLoad()) return;
            for (int i = 0; i < MAX_IR_COUNT; ++i) {
                const int irIndex = i;
                const auto randomFile = irManager.sampleRandomIR();
                if (!randomFile.existsAsFile()) return;
                irManager.getBusyLoading().store(true, std::memory_order_release);
                irManager.irThreadPool.addJob([this, loadIR, irIndex, randomFile]() { loadIR(irIndex, randomFile); });
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
        case IRCommand::IR_DIRECTORY_COLLECT: {
            irManager.collectIRs();
            break;
        }
        case IRCommand::IR_DIRECTORY_REFRESH: {
            irManager.loadDirectories();
            break;
        }
        case IRCommand::SET_FILE_FILTER: {
            irManager.setFileFilter(cmd.fileFilter);
            break;
        }
        case IRCommand::SET_DIRECTORY_FILTER: {
            irManager.setDirectoryFilter(cmd.directoryFilter);
            break;
        }
        case IRCommand::SET_SAMPLING_MODE: {
            irManager.setSamplingMode(cmd.samplingMode);

            juce::String samplingMode = (cmd.samplingMode == IRSamplingMode::UNIFORM_ACROSS_ALL_FILES) ? "uniform across all files" : "uniform across directories";
            DBG("IR: Set sampling mode to " << samplingMode);
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
    convolutionStateBuilder.clearUpdates();

    IRUpdate update;
    auto* updateQueue = irManager.getUpdateQueue();

    while (updateQueue->try_dequeue(update))
        convolutionStateBuilder.pushIRUpdate(update);
}

// Control cycle

void ControlThread::runControlCycle(float dt) {
    auto startTime = std::chrono::steady_clock::now();
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
        convolutionStateBuilder.notifyDecayChanged();
    }

    // Weights
    if (motionController.hasPositionUpdated() || motionController.hasFieldUpdated() 
        || guiState.updateWeights.exchange(false, std::memory_order_acquire)) {

        polarMap.computeRelatives();
        updateWeights();
        convolutionStateBuilder.notifyWeightsChanged();
    }

    // Process IR state
    processIRCommands();
    processIRResults();
    processIRUpdates();

    updateConvolutionState();
}

/* PUBLIC */

ControlThread::ControlThread(const juce::AudioProcessorValueTreeState& apvts, IRManager& manager, 
    GUIState& gState, std::shared_ptr<ConvolutionStateHolder> cState) : juce::Thread("Control Thread"),
    apvts(apvts), irManager(manager), 
    guiState(gState), convolutionState(cState),
    motionController(&polarMap, &positionTime, &fieldTime),
    convolutionStateBuilder(irManager, guiState) {
    convolutionStateBuilder.prepare();
}

void ControlThread::setControlRate(float nRate) { controlRate = nRate; DBG("SYNC: Set control rate to " << nRate); }

void ControlThread::run() {
    lastTick = std::chrono::steady_clock::now();
    while (!threadShouldExit()) {
        auto startTime = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(startTime - lastTick).count();
        lastTick = startTime;
        // DBG("dt: " << dt);

        runControlCycle(dt);

        auto elapsedTime = std::chrono::steady_clock::now() - startTime;
        // DBG("Control cycle: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsedTime).count() << " ms");

        std::chrono::duration<double, std::nano> remainingTime {};
        {
            juce::SpinLock::ScopedLockType lock(controlLock);
            remainingTime = std::chrono::duration<double>(1.0f / controlRate) - elapsedTime;
        }
        // auto headroomMs = std::chrono::duration_cast<std::chrono::milliseconds>(remainingTime).count();
        // if (headroomMs < 0) DBG("WARNING: Control deadline(s) missed: " << headroomMs << " ms");

        // eepy
        if (remainingTime > std::chrono::duration<double>::zero()) 
            wait(static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(remainingTime).count()));
    }
}