#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utilities.h"

/* PRIVATE */

// Parameters

juce::AudioProcessorValueTreeState::ParameterLayout MareverbAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::globalMix, "Global Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::decay, "Decay", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(ParamID::weightingMode, "Weighting Mode", weightingModes, static_cast<int>(WeightingMode::ABSOLUTE)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::strength, "Strength", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::spread, "Spread", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 1.0f));
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(ParamID::positionPattern, "Position Pattern", positionPatterns, static_cast<int>(PositionPattern::LISSAJOUS)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::positionRate, "Position Rate", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f, 1.0f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::positionModA, "Position Mod A", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::positionModB, "Position Mod B", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    
    layout.add(std::make_unique<juce::AudioParameterInt>(ParamID::fieldSelect, "Field Select", 0, MAX_IR_COUNT, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(ParamID::fieldPattern, "Field Pattern", fieldPatterns, static_cast<int>(FieldPattern::RING)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::fieldRate, "Field Rate", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f, 1.0f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::fieldModA, "Field Mod A", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::fieldModB, "Field Mod B", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));

    return layout;
}

void MareverbAudioProcessor::updateParameters() {
    Settings settings = getSettings(apvts);

    auto* convProcessor = getConvolutionReverbProcessor();
    if (convProcessor) {
        convProcessor->setMix(settings.globalMix);
        convProcessor->setDecay(settings.decay);
    }

    motionController.setPositionParameters({
        settings.positionPattern, 
        settings.positionRate, 
        settings.positionModA, 
        settings.positionModB}
    );
    motionController.setFieldParameters({
        MAX_IR_COUNT, 
        settings.fieldSelect, 
        settings.fieldPattern, 
        settings.fieldRate, 
        settings.fieldModA, 
        settings.fieldModB}
    );
}

// IR management

void MareverbAudioProcessor::saveDirectories() {
    auto* properties = applicationProperties.getUserSettings();
    if (!properties) return;

    int dirCount = static_cast<int>(irDirectories.size());
    properties->setValue("irDirectoryCount", dirCount);
    for (int i = 0; i < dirCount; ++i) {
        properties->setValue("irDirectory_" + juce::String(i), irDirectories[i].irDirectory.getFullPathName());
        properties->setValue("irDirectoryActive_" + juce::String(i), irDirectories[i].active);
    }
    properties->saveIfNeeded();
}

void MareverbAudioProcessor::loadDirectories() {
    auto* properties = applicationProperties.getUserSettings();
    if (!properties) return;

    int dirCount = properties->getIntValue("irDirectoryCount", 0);
    for (int i = 0; i < dirCount; ++i) {
        juce::File dir(properties->getValue("irDirectory_" + juce::String(i)));
        bool active = properties->getBoolValue("irDirectoryActive_" + juce::String(i), true);
        if (dir.isDirectory()) irDirectories.push_back({ dir, active });
    }
    collectIRs();
}

void MareverbAudioProcessor::chooseIR(int irIndex) {
    irFileChooser = std::make_unique<juce::FileChooser>(
        "Load an impulse response...", juce::File{}, formatManager.getWildcardForAllFormats());
    irFileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, irIndex](const juce::FileChooser& fileChooser) {
            if (fileChooser.getResults().isEmpty()) return;
            loadIR(irIndex, fileChooser.getResult());
        });
}

void MareverbAudioProcessor::chooseIRDirectory() {
    irDirectoryChooser = std::make_unique<juce::FileChooser>(
        "Choose a directory...", juce::File{});
    irDirectoryChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this](const juce::FileChooser& fileChooser) {
            if (fileChooser.getResults().isEmpty()) return;
            addIRDirectory(fileChooser.getResult());
        });
}

void MareverbAudioProcessor::addIRDirectory(juce::File dir) {
    if (dir.isDirectory()) {
        irDirectories.push_back({dir, true});
        collectIRs();
        saveDirectories();
    }
}
void MareverbAudioProcessor::removeIRDirectory(int index) { 
    if (index >= 1 && index < static_cast<int>(irDirectories.size())) { // index 0 = factory
        irDirectories.erase(irDirectories.begin() + index);
        collectIRs();
        saveDirectories();
    }
}
void MareverbAudioProcessor::activateIRDirectory(int index, bool nState) {
    if (index >= 0 && index < static_cast<int>(irDirectories.size())) {
        irDirectories[index].active = nState;
        collectIRs();
        saveDirectories();
    }
}

void MareverbAudioProcessor::collectIRs() {
    irFiles.clear();
    for (const auto& dir : irDirectories) {
        if (!dir.active || !dir.irDirectory.isDirectory()) continue;
        irFiles.addArray(dir.irDirectory.findChildFiles(juce::File::findFiles, true, formatManager.getWildcardForAllFormats()));
    }
}

bool MareverbAudioProcessor::loadIR(int irIndex, juce::File irFile) {
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(irFile));
    if (reader == nullptr) return false;

    // Read samples to buffer in activeIRBuffers
    int numChannels = (int)reader->numChannels; 
    int numSamples = (int)reader->lengthInSamples;
    activeIRBuffers[irIndex].setSize(numChannels, numSamples);
    if (reader->read(&activeIRBuffers[irIndex], 0, numSamples, 0, true, true)) {
        // Set IR buffer in convolution processor
        auto* convProcessor = getConvolutionReverbProcessor();
        if (convProcessor) {
            convProcessor->getConvolutionReverb()->setIRBuffer(irIndex, activeIRBuffers[irIndex]);
            return true;
        }
    }
    return false;
}

bool MareverbAudioProcessor::loadRandomIR(int irIndex) {
    if (irFiles.isEmpty() || !validateIRIndex(irIndex)) return false;

    int idx = irRNG.nextInt(irFiles.size());
    juce::File randomIR = irFiles[idx];
    activeIRFiles[irIndex] = randomIR;
    return loadIR(irIndex, randomIR);
}

bool MareverbAudioProcessor::loadRandomIRs() {
    if (irFiles.isEmpty()) return false;
    for (int irIndex = 0; irIndex < MAX_IR_COUNT; ++irIndex)
       if (!loadRandomIR(irIndex)) return false;
    return true;
}

void MareverbAudioProcessor::clearIR(int irIndex) {
    if (validateIRIndex(irIndex)) {
        activeIRFiles[irIndex] = juce::File{};
        activeIRBuffers[irIndex].setSize(0, 0);
        auto* convProcessor = getConvolutionReverbProcessor();
        if (convProcessor) convProcessor->getConvolutionReverb()->clearIRBuffer(irIndex);
    }
}

void MareverbAudioProcessor::clearIRs() {
    for (int irIndex = 0; irIndex < MAX_IR_COUNT; ++irIndex) clearIR(irIndex);
}

// Time

void MareverbAudioProcessor::advancePhase() {
    constexpr float positionRateScale = 0.2f, fieldRateScale = 0.05f;
    const float positionFreq = apvts.getRawParameterValue(ParamID::positionRate)->load() * positionRateScale,
                fieldFreq = apvts.getRawParameterValue(ParamID::fieldRate)->load() * fieldRateScale;

    const float blockDur = static_cast<float>(getBlockSize()) / static_cast<float>(getSampleRate());
    const float positionPhaseIncrement = juce::MathConstants<float>::twoPi * positionFreq * blockDur,
                fieldPhaseIncrement = juce::MathConstants<float>::twoPi * fieldFreq * blockDur;

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
        float gainL = std::sqrtf(0.5f * (1.0f - pan)) * gainFB, // L-R
              gainR = std::sqrtf(0.5f * (1.0f + pan)) * gainFB; 

        gainL = juce::jmap(spread, 1.0f, gainL); 
        gainR = juce::jmap(spread, 1.0f, gainR);

        irWeights[0][ir] = rawWeights[ir] * gainL;
        irWeights[1][ir] = rawWeights[ir] * gainR;
    }
}

// Polar map, motion

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

// Processor graph

void MareverbAudioProcessor::connectAudioNodes() {
    if (!mainProcessor->getConnections().empty()) return;
    int inputChannels = audioInputNode->getProcessor()->getTotalNumOutputChannels(); 
    int outputChannels = 2;
    for (int ch = 0; ch < inputChannels; ++ch)
        mainProcessor->addConnection({ { audioInputNode->nodeID, ch }, { convolutionVerbNode->nodeID, ch } });
    for (int ch = 0; ch < outputChannels; ++ch)
        mainProcessor->addConnection({ { convolutionVerbNode->nodeID, ch }, { audioOutputNode->nodeID, ch } });
}

ConvolutionReverbAudioProcessor* MareverbAudioProcessor::getConvolutionReverbProcessor() const {
    return dynamic_cast<ConvolutionReverbAudioProcessor*>(convolutionVerbNode->getProcessor());
}

/* PUBLIC */

MareverbAudioProcessor::MareverbAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    mainProcessor(new juce::AudioProcessorGraph()), motionController(&polarMap, &positionTime, &fieldTime), irWeights(2) {

    // Init properties
    juce::PropertiesFile::Options options;
    options.applicationName = "Mareverb";
    options.filenameSuffix = ".amre";
    options.osxLibrarySubFolder = "Application Support";
    applicationProperties.setStorageParameters(options);

    // Init nodes
    audioInputNode = mainProcessor->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode));
    convolutionVerbNode = mainProcessor->addNode(std::make_unique<ConvolutionReverbAudioProcessor>());
    audioOutputNode = mainProcessor->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode));

    // Init IRs
    // HACK: Should store factory IRs in common directory
    formatManager.registerBasicFormats(); // .wav, .aiff, .flac, .opus, .mp3
    juce::File srcDirectory = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    while (srcDirectory.getFileName() != "Debug" && srcDirectory.getParentDirectory() != srcDirectory)
        srcDirectory = srcDirectory.getParentDirectory();

    irDirectories.push_back({srcDirectory.getChildFile("IR Samples"), true});
    loadDirectories();
    bool loadedIRs = loadRandomIRs();
    jassert(loadedIRs);
    // clearIRs();

    // Init RNG
    irRNG = juce::Random();
    irRNG.setSeedRandomly();
}

MareverbAudioProcessor::~MareverbAudioProcessor() { }

// Boilerplate
double MareverbAudioProcessor::getTailLengthSeconds() const { return 0.0; }

#ifndef JucePlugin_PreferredChannelConfigurations
bool MareverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() && // Output stereo
           (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())); // Input mono or stereo
}
#endif

// DSP
void MareverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    mainProcessor->setPlayConfigDetails(getMainBusNumInputChannels(), getMainBusNumOutputChannels(), sampleRate, samplesPerBlock);
    mainProcessor->prepareToPlay(sampleRate, samplesPerBlock);
    connectAudioNodes();
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

    // Block rate updates
    advancePhase();
    updateParameters();

    // Control rate updates
    controlCounter += buffer.getNumSamples();
    if (controlCounter >= static_cast<int>(getSampleRate() / CONTROL_RATE)) {
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

        // Weights
        if (motionController.hasPositionUpdated() || motionController.hasFieldUpdated()) {
            polarMap.computeRelatives();
            updateWeights();
            auto* convProcessor = getConvolutionReverbProcessor();
            if (convProcessor) convProcessor->setWeights(irWeights);
        }

        controlCounter = 0;
    }

    mainProcessor->processBlock(buffer, midiMessages);
}

// GUI

juce::AudioProcessorEditor* MareverbAudioProcessor::createEditor() { return new MareverbAudioProcessorEditor (*this); }

// State

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

Settings MareverbAudioProcessor::getSettings(juce::AudioProcessorValueTreeState& parameters) {
    Settings settings;

    settings.globalMix = parameters.getRawParameterValue(ParamID::globalMix)->load();
    settings.decay = parameters.getRawParameterValue(ParamID::decay)->load();
    
    settings.weightingMode = static_cast<WeightingMode>(parameters.getRawParameterValue(ParamID::weightingMode)->load());
    settings.strength = parameters.getRawParameterValue(ParamID::strength)->load();
    settings.spread = parameters.getRawParameterValue(ParamID::spread)->load();

    settings.positionPattern = static_cast<PositionPattern>(parameters.getRawParameterValue(ParamID::positionPattern)->load());
    settings.positionRate = parameters.getRawParameterValue(ParamID::positionRate)->load();
    settings.positionModA = parameters.getRawParameterValue(ParamID::positionModA)->load();
    settings.positionModB = parameters.getRawParameterValue(ParamID::positionModB)->load();

    settings.fieldSelect = static_cast<int>(parameters.getRawParameterValue(ParamID::fieldSelect)->load());
    settings.fieldPattern = static_cast<FieldPattern>(parameters.getRawParameterValue(ParamID::fieldPattern)->load());
    settings.fieldRate = parameters.getRawParameterValue(ParamID::fieldRate)->load();
    settings.fieldModA = parameters.getRawParameterValue(ParamID::fieldModA)->load();
    settings.fieldModB = parameters.getRawParameterValue(ParamID::fieldModB)->load();

    return settings;
}

// INSTANCING

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MareverbAudioProcessor(); }
