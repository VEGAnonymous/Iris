#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utilities.h"

/* PRIVATE */

// Parameters

juce::AudioProcessorValueTreeState::ParameterLayout MareverbAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::globalMix, "Global Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::decay, "Decay", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::lowCut, "Low Cut", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 20.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::highCut, "High Cut", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 20000.0f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(ParamID::weightingMode, "Weighting Mode", weightingModes, static_cast<int>(WeightingMode::ABSOLUTE)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::strength, "Strength", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::spread, "Spread", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 1.0f));

    // IR swap intervals
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        juce::String minID = ParamID::irSwapMin(i);
        juce::String maxID = ParamID::irSwapMax(i);
        layout.add(std::make_unique<juce::AudioParameterFloat>(minID, minID, juce::NormalisableRange<float>(5.0f, 60.0f, 0.1f), 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(maxID, maxID, juce::NormalisableRange<float>(5.0f, 60.0f, 0.1f), 0.0f));
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>(ParamID::positionPattern, "Position Pattern", positionPatterns, static_cast<int>(PositionPattern::LISSAJOUS)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::positionRate, "Position Rate", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f, 1.0f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::positionModA, "Position Mod A", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::positionModB, "Position Mod B", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(ParamID::fieldPattern, "Field Pattern", fieldPatterns, static_cast<int>(FieldPattern::RING)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::fieldRate, "Field Rate", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f, 1.0f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::fieldModA, "Field Mod A", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::fieldModB, "Field Mod B", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));

    return layout;
}

void MareverbAudioProcessor::updateParameters() {
    Settings settings = getSettings(apvts);

    mixer.setWetMixProportion(settings.globalMix);

    auto* cutProcessor = getCutFilterProcessor();
    if (cutProcessor) {
        if (settings.lowCut != cutProcessor->getLowCutCutoff()) cutProcessor->setLowCutCutoff(settings.lowCut);
        if (settings.highCut != cutProcessor->getHighCutCutoff()) cutProcessor->setHighCutCutoff(settings.highCut);
    }

    motionController.setPositionParameters({
        settings.positionPattern, 
        settings.positionRate, 
        settings.positionModA, 
        settings.positionModB}
    );
    motionController.setFieldParameters({
        MAX_IR_COUNT,
        apvts.state.getProperty(PropertyID::selectedIR),
        settings.fieldPattern, 
        settings.fieldRate, 
        settings.fieldModA, 
        settings.fieldModB}
    );

    for (int ir = 0; ir < MAX_IR_COUNT; ++ir) {
        float nMin = apvts.getRawParameterValue(ParamID::irSwapMin(ir))->load();
        float nMax = apvts.getRawParameterValue(ParamID::irSwapMax(ir))->load();
        irManager.setSwapInterval(ir, nMin, nMax);
    }
}

// Time

void MareverbAudioProcessor::advancePhase(float dt) {
    constexpr float positionRateScale = 0.2f, fieldRateScale = 0.05f;

    const float positionFreq = apvts.getRawParameterValue(ParamID::positionRate)->load() * positionRateScale;
    const float fieldFreq = apvts.getRawParameterValue(ParamID::fieldRate)->load() * fieldRateScale;

    const float positionPhaseIncrement = juce::MathConstants<float>::twoPi * positionFreq * dt;
    const float fieldPhaseIncrement = juce::MathConstants<float>::twoPi * fieldFreq * dt;

    positionTime += positionPhaseIncrement; 
    fieldTime += fieldPhaseIncrement;
}

// Binaural processing

void MareverbAudioProcessor::processBinaural(const std::array<float, MAX_IR_COUNT>& rawWeights, 
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

void MareverbAudioProcessor::updateWeights() {
    std::array<float, MAX_IR_COUNT> distanceWeights{};

    // Parameterize
    WeightingMode weightingMode = static_cast<WeightingMode>(apvts.getRawParameterValue(ParamID::weightingMode)->load());
    const float& strength = apvts.getRawParameterValue(ParamID::strength)->load();

    float minDistance = juce::jmap(strength, 0.05f, 0.25f), maxWeight = 1.0f / (minDistance * minDistance), trim = 0.5f; // Absolute
    float distanceFactor = juce::jmap(strength * strength, 0.5f, 3.5f); // Relative

    // Compute inverse-distance weights
    auto relatives = polarMap.getRelatives();
    if (weightingMode == WeightingMode::RELATIVE) {
        for (int ir = 0; ir < MAX_IR_COUNT; ++ir) distanceWeights[ir] = 1.0f / powf(relatives[ir].r + EPSILON, distanceFactor);
        float sum = std::accumulate(distanceWeights.begin(), distanceWeights.end(), 0.0f);
        if (sum > 0.0f) for (int ir = 0; ir < MAX_IR_COUNT; ++ir) distanceWeights[ir] /= sum; // Normalize weights sum to 1
    } else { // WeightingMode::ABSOLUTE
        for (int ir = 0; ir < MAX_IR_COUNT; ++ir) { 
            float d = std::max(relatives[ir].r, minDistance);
            distanceWeights[ir] = ((1.0f / (d * d)) / maxWeight) * trim;
        }
    }

    processBinaural(distanceWeights, relatives); // Inject binaural cues, mutate irWeights
}

std::shared_ptr<ConvolutionState> MareverbAudioProcessor::buildConvolutionState() {
    auto currentState = std::atomic_load(&convolutionState->state);
    jassert(currentState && currentState->irBank);
    auto nextState = std::make_shared<ConvolutionState>();

    // DBG("Building convolution state");

    // Get flags
    std::deque<int> irsChanged, irsCleared;
    bool decayChanged, weightsChanged;
    {
        juce::SpinLock::ScopedLockType lock(flagLock);

        IRChanges irChanges = irManager.consumeIRChanges();
        irsChanged = std::move(irChanges.irsChanged);
        irsCleared = std::move(irChanges.irsCleared);
        decayChanged = stateFlags.decayChanged;
        weightsChanged = stateFlags.weightsChanged;

        stateFlags.resetFlags();
    }
    
    // IR state
    std::shared_ptr<const ConvolutionIRBank> irBank = currentState->irBank;
    bool irChanged = !(irsChanged.empty() && irsCleared.empty());
    if (irChanged) {
        auto nBank = std::make_shared<ConvolutionIRBank>(*currentState->irBank);

        while (!irsChanged.empty()) {
            int irIndex = irsChanged.front();
            DBG("Setting IR " << irIndex);
            irsChanged.pop_front();
            if (validateIRIndex(irIndex)) nBank->setIR(irIndex, irManager.getIRSlot(irIndex).buffer);
        }

        while (!irsCleared.empty()) {
            int irIndex = irsCleared.front();
            DBG("Clearing IR " << irIndex);
            irsCleared.pop_front();
            if (validateIRIndex(irIndex)) nBank->clearIR(irIndex);
        }

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
        nextState->mixState.mixedSpectra = currentState->mixState.mixedSpectra; }

    {
        juce::SpinLock::ScopedLockType lock(flagLock);
        stateFlags.resetFlags();
    }

    return nextState;
}

// Concurrency

std::shared_ptr<ConvolutionState> MareverbAudioProcessor::runControlCycle() {
    // 
    const float dt = static_cast<float>(getBlockSize()) / static_cast<float>(getSampleRate()); // TODO: Swap for real timer
    advancePhase(dt);
    irManager.advanceSwapTimers(dt);

    // Position + field
    motionController.updateField();
    motionController.updatePosition();

    // Pass state to GUI
    if (motionController.hasPositionUpdated()) {
        position.store(polarMap.getPosition(), std::memory_order_relaxed);
        positionChanged.store(true, std::memory_order_release);
    }

    if (motionController.hasFieldUpdated() || updateField.exchange(false, std::memory_order_relaxed)) {
        if (fieldMutex.try_lock()) {
            fieldCoordinates = polarMap.getCoordinates();
            fieldMutex.unlock();
            fieldChanged.store(true, std::memory_order_release);
        }
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

// Processor graph

void MareverbAudioProcessor::connectAudioNodes() {
    if (!mainProcessor->getConnections().empty()) return;
    int inputChannels = audioInputNode->getProcessor()->getTotalNumOutputChannels(); 
    for (int ch = 0; ch < inputChannels; ++ch) {
        mainProcessor->addConnection({ { audioInputNode->nodeID, ch }, { convolutionVerbNode->nodeID, ch } });
        mainProcessor->addConnection({ { convolutionVerbNode->nodeID, ch }, { cutFilterNode->nodeID, ch } });
        mainProcessor->addConnection({ { cutFilterNode->nodeID, ch }, { audioOutputNode->nodeID, ch } });
    }
}

ConvolutionReverbAudioProcessor* MareverbAudioProcessor::getConvolutionReverbProcessor() const {
    return dynamic_cast<ConvolutionReverbAudioProcessor*>(convolutionVerbNode->getProcessor());
}

CutFilterAudioProcessor* MareverbAudioProcessor::getCutFilterProcessor() const {
    return dynamic_cast<CutFilterAudioProcessor*>(cutFilterNode->getProcessor());
}

/* PUBLIC */

MareverbAudioProcessor::MareverbAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),

    mainProcessor(new juce::AudioProcessorGraph()), 
    mixer(HOP_SIZE),
    irManager(&applicationProperties),
    polarMap(), 
    motionController(&polarMap, &positionTime, &fieldTime) {

    // Init properties
    if (!apvts.state.hasProperty(PropertyID::selectedIR))
        apvts.state.setProperty(PropertyID::selectedIR, 0, nullptr);

    juce::PropertiesFile::Options options;
    options.applicationName = "Mareverb";
    options.filenameSuffix = ".amre";
    options.osxLibrarySubFolder = "Application Support";
    applicationProperties.setStorageParameters(options);

    // Init IRs
    irManager.prepare();

    // Init DSP state
    convolutionState = std::make_shared<ConvolutionStateHolder>();

    auto initialState = std::make_shared<ConvolutionState>();
    initialState->irBank = std::make_shared<ConvolutionIRBank>();
    initialState->prepare();
    std::atomic_store(&convolutionState->state, initialState);

    // Init nodes
    audioInputNode = mainProcessor->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode));
    convolutionVerbNode = mainProcessor->addNode(std::make_unique<ConvolutionReverbAudioProcessor>(convolutionState));
    cutFilterNode = mainProcessor->addNode(std::make_unique<CutFilterAudioProcessor>());
    audioOutputNode = mainProcessor->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode));
}

MareverbAudioProcessor::~MareverbAudioProcessor() { }

// Boilerplate

double MareverbAudioProcessor::getTailLengthSeconds() const { return 0.0; } // TODO: Replace with actual value

#ifndef JucePlugin_PreferredChannelConfigurations
bool MareverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() && // Output stereo
           (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())); // Input mono or stereo
}
#endif

// DSP

void MareverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::dsp::ProcessSpec spec { 
        sampleRate, 
        static_cast<juce::uint32>(samplesPerBlock), 
        static_cast<juce::uint32>(getTotalNumOutputChannels()) 
    };

    mainProcessor->setPlayConfigDetails(getMainBusNumInputChannels(), getMainBusNumOutputChannels(), sampleRate, samplesPerBlock);
    mainProcessor->prepareToPlay(sampleRate, samplesPerBlock);

    connectAudioNodes();

    mixer.setMixingRule(juce::dsp::DryWetMixingRule::sin3dB);
    mixer.setWetLatency(HOP_SIZE);
    mixer.prepare(spec);

    updateParameters();
}

void MareverbAudioProcessor::releaseResources() {
    mainProcessor->releaseResources();
}

void MareverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    updateParameters();

    auto nextState = runControlCycle();

    // Atomic swap in new state
    std::atomic_store(&convolutionState->state, nextState);

    // Process + mix chain
    mixer.pushDrySamples(buffer);
    mainProcessor->processBlock(buffer, midiMessages);
    mixer.mixWetSamples(buffer);
}

juce::AudioProcessorEditor* MareverbAudioProcessor::createEditor() { return new MareverbAudioProcessorEditor (*this); }

void MareverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData) { 
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void MareverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        apvts.replaceState(tree);
        updateParameters();
    }
}

// Parameters

Settings MareverbAudioProcessor::getSettings(juce::AudioProcessorValueTreeState& parameters) {
    Settings settings;

    settings.globalMix = parameters.getRawParameterValue(ParamID::globalMix)->load();
    settings.decay = parameters.getRawParameterValue(ParamID::decay)->load();
    settings.lowCut = parameters.getRawParameterValue(ParamID::lowCut)->load();
    settings.highCut = parameters.getRawParameterValue(ParamID::highCut)->load();
    
    settings.weightingMode = static_cast<WeightingMode>(parameters.getRawParameterValue(ParamID::weightingMode)->load());
    settings.strength = parameters.getRawParameterValue(ParamID::strength)->load();
    settings.spread = parameters.getRawParameterValue(ParamID::spread)->load();

    settings.positionPattern = static_cast<PositionPattern>(parameters.getRawParameterValue(ParamID::positionPattern)->load());
    settings.positionRate = parameters.getRawParameterValue(ParamID::positionRate)->load();
    settings.positionModA = parameters.getRawParameterValue(ParamID::positionModA)->load();
    settings.positionModB = parameters.getRawParameterValue(ParamID::positionModB)->load();

    settings.fieldPattern = static_cast<FieldPattern>(parameters.getRawParameterValue(ParamID::fieldPattern)->load());
    settings.fieldRate = parameters.getRawParameterValue(ParamID::fieldRate)->load();
    settings.fieldModA = parameters.getRawParameterValue(ParamID::fieldModA)->load();
    settings.fieldModB = parameters.getRawParameterValue(ParamID::fieldModB)->load();

    return settings;
}

IRManager* MareverbAudioProcessor::getIRManager() { return &irManager; }

// Instancing

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MareverbAudioProcessor(); }