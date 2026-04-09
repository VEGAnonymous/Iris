#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ConvolutionReverbAudioProcessor.h"
#include "Utilities.h"

/* PRIVATE */

// Parameters

juce::AudioProcessorValueTreeState::ParameterLayout MareverbAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("Global Mix", "Global Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Decay", "Decay", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Position Pattern", "Position Pattern", positionPatterns, static_cast<int>(PositionPattern::LISSAJOUS)));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Position Rate", "Position Rate", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f, 1.0f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Position Mod A", "Position Mod A", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Position Mod B", "Position Mod B", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Field Pattern", "Field Pattern", fieldPatterns, static_cast<int>(FieldPattern::RING)));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Field Rate", "Field Rate", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f, 1.0f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Field Mod A", "Field Mod A", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Field Mod B", "Field Mod B", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));

    return layout;
}

void MareverbAudioProcessor::updateParameters() {
    Settings settings = getSettings(apvts);

    auto* convProcessor = dynamic_cast<ConvolutionReverbAudioProcessor*>(convolutionVerbNode->getProcessor());
    if (convProcessor != nullptr) {
        convProcessor->setMix(settings.globalMix);
        convProcessor->setDecay(settings.decay);
    }

    motionController.setPositionParameters({settings.positionPattern, settings.positionRate, settings.positionModA, settings.positionModB});
    motionController.setFieldParameters({MAX_IR_COUNT, settings.fieldPattern, settings.fieldRate, settings.fieldModA, settings.fieldModB});
}

// Time

void MareverbAudioProcessor::advancePhase() {
    float positionFreq = apvts.getRawParameterValue("Position Rate")->load() * 0.2f;
    float fieldFreq = apvts.getRawParameterValue("Field Rate")->load() * 0.05f;
    float positionPhaseIncrement = juce::MathConstants<float>::twoPi * positionFreq
        * (static_cast<float>(getBlockSize()) / static_cast<float>(getSampleRate()));
    float fieldPhaseIncrement = juce::MathConstants<float>::twoPi * fieldFreq
        * (static_cast<float>(getBlockSize()) / static_cast<float>(getSampleRate()));
    positionTime += positionPhaseIncrement; fieldTime += fieldPhaseIncrement;
}

// Polar map, motion

void MareverbAudioProcessor::updateWeights() {
    std::array<float, MAX_IR_COUNT> rawWeights{};

    const float distanceFactor = 2.0f;
    auto relatives = polarMap.getRelatives();

    for (int ir = 0; ir < MAX_IR_COUNT; ++ir) {
        rawWeights[ir] = 1.0f / powf(relatives[ir].r + 1e-6f, distanceFactor); // Inverse-distance weights
        // TODO: Do binaural stuff here
    }

    float sum = std::accumulate(rawWeights.begin(), rawWeights.end(), 0.0f);
    if (sum > 0.0f) for (int ir = 0; ir < MAX_IR_COUNT; ++ir) rawWeights[ir] /= sum; // Normalize weights sum to 1

    for (int channel = 0; channel < 2; ++channel)
        for (int ir = 0; ir < MAX_IR_COUNT; ++ir) irWeights[channel][ir] = rawWeights[ir];
}

void MareverbAudioProcessor::connectAudioNodes() {
    if (!mainProcessor->getConnections().empty()) return;
    int inputChannels = audioInputNode->getProcessor()->getTotalNumOutputChannels(); 
    int outputChannels = 2;
    for (int ch = 0; ch < inputChannels; ++ch)
        mainProcessor->addConnection({ { audioInputNode->nodeID, ch }, { convolutionVerbNode->nodeID, ch } });
    for (int ch = 0; ch < outputChannels; ++ch)
        mainProcessor->addConnection({ { convolutionVerbNode->nodeID, ch }, { audioOutputNode->nodeID, ch } });
}

/* PUBLIC */

MareverbAudioProcessor::MareverbAudioProcessor()
     : AudioProcessor (BusesProperties().withInput  ("Input", juce::AudioChannelSet::stereo(), true)
                                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       mainProcessor(new juce::AudioProcessorGraph()), motionController(&polarMap, &positionTime, &fieldTime), irWeights(2) {

    // Init nodes
    audioInputNode = mainProcessor->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode));
    convolutionVerbNode = mainProcessor->addNode(std::make_unique<ConvolutionReverbAudioProcessor>());
    audioOutputNode = mainProcessor->addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode));

    // Load IRs
    // HACK: Should store IRs in common directory
    juce::File srcDirectory = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    while (srcDirectory.getFileName() != "Debug" && srcDirectory.getParentDirectory() != srcDirectory)
        srcDirectory = srcDirectory.getParentDirectory();
    irDirectory = srcDirectory.getChildFile("IR Samples");
    irFiles = irDirectory.findChildFiles(juce::File::findFiles, true, "*");
    jassert(irFiles.size() != 0);

    formatManager.registerBasicFormats();
    // formatManager.registerFormat(new juce::FlacAudioFormat(), true);

    irRNG = juce::Random();
    irRNG.setSeedRandomly();
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        bool loaded = loadRandomIR(i);
        jassert(loaded);
    }

    // Init IR coordinates
    for (int ir = 0; ir < MAX_IR_COUNT; ++ir)
        polarMap.setCoordinate(ir, { 1.0f, (ir * juce::MathConstants <float>::twoPi) / MAX_IR_COUNT });
}

MareverbAudioProcessor::~MareverbAudioProcessor() { }

bool MareverbAudioProcessor::loadIR(int irIndex, int fileIndex) {
    if (irIndex < 0 || irIndex >= MAX_IR_COUNT) return false;
    if (fileIndex < 0 && !activeIRFiles[irIndex].existsAsFile()) return false; // Empty, must supply file index
    if (fileIndex >= 0 && fileIndex < irFiles.size()) activeIRFiles[irIndex] = irFiles[fileIndex]; // Set file

    // Load file from activeIRFiles and create reader
    juce::File ir = activeIRFiles[irIndex];
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(ir));
    if (reader == nullptr) return false;

    // Read samples to buffer in activeIRBuffers
    int numChannels = (int)reader->numChannels; int numSamples = (int)reader->lengthInSamples;
    activeIRBuffers[irIndex].setSize(numChannels, numSamples);
    if (reader->read(&activeIRBuffers[irIndex], 0, numSamples, 0, true, true)) {
        // Set IR buffer in convolution processor
        auto* convProcessor = dynamic_cast<ConvolutionReverbAudioProcessor*>(convolutionVerbNode->getProcessor());
        if (convProcessor != nullptr) {
            convProcessor->getConvolutionReverb()->setIRBuffer(irIndex, activeIRBuffers[irIndex]);
            return true;
        }
    } return false;
}

bool MareverbAudioProcessor::loadRandomIR(int irIndex) {
    if (irFiles.isEmpty()) return false;
    int idx = irRNG.nextInt(irFiles.size());
    return loadIR(irIndex, idx);
}

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
            auto* convProcessor = dynamic_cast<ConvolutionReverbAudioProcessor*>(convolutionVerbNode->getProcessor());
            if (convProcessor != nullptr) convProcessor->setWeights(irWeights);
        }

        controlCounter = 0;
    }

    mainProcessor->processBlock(buffer, midiMessages);
}

// GUI

juce::AudioProcessorEditor* MareverbAudioProcessor::createEditor() { 
    // return new juce::GenericAudioProcessorEditor(*this); // TEMP
    return new MareverbAudioProcessorEditor (*this);
}

// State

void MareverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData) { 
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void MareverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        apvts.replaceState(tree);
        // TODO: Update parameters
    }
}

Settings MareverbAudioProcessor::getSettings(juce::AudioProcessorValueTreeState& parameters) {
    Settings settings;

    settings.globalMix = parameters.getRawParameterValue("Global Mix")->load();
    settings.decay = parameters.getRawParameterValue("Decay")->load();
    settings.positionPattern = static_cast<PositionPattern>(parameters.getRawParameterValue("Position Pattern")->load());
    settings.positionRate = parameters.getRawParameterValue("Position Rate")->load();
    settings.positionModA = parameters.getRawParameterValue("Position Mod A")->load();
    settings.positionModB = parameters.getRawParameterValue("Position Mod B")->load();
    settings.fieldPattern = static_cast<FieldPattern>(parameters.getRawParameterValue("Field Pattern")->load());
    settings.fieldRate = parameters.getRawParameterValue("Field Rate")->load();
    settings.fieldModA = parameters.getRawParameterValue("Field Mod A")->load();
    settings.fieldModB = parameters.getRawParameterValue("Field Mod B")->load();

    return settings;
}

// INSTANCING

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MareverbAudioProcessor(); }
