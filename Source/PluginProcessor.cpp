#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ConvolutionReverbAudioProcessor.h"

/* PRIVATE */

juce::AudioProcessorValueTreeState::ParameterLayout MareverbAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("Global Mix", "Global Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Decay", "Decay", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    
    const juce::StringArray motionPatterns {"Vanilla", "Orbit", "Spiral", "Floral", "Lissajous", "Discrete", "Walk"};
    layout.add(std::make_unique<juce::AudioParameterChoice>("Motion Pattern", "Motion Pattern", motionPatterns, MotionPattern::LISSAJOUS));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Motion Rate", "Motion Rate", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Motion Mod A", "Motion Mod A", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Motion Mod B", "Motion Mod B", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));

    return layout;
}

MareverbAudioProcessor::Settings MareverbAudioProcessor::getSettings(juce::AudioProcessorValueTreeState& parameters) {
    MareverbAudioProcessor::Settings settings;

    settings.globalMix = parameters.getRawParameterValue("Global Mix")->load();
    settings.decay = parameters.getRawParameterValue("Decay")->load();

    // TODO: Fill this out

    return settings;
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
       mainProcessor(new juce::AudioProcessorGraph()) {

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

    // Init convolution reverb
    auto* convProcessor = dynamic_cast<ConvolutionReverbAudioProcessor*>(convolutionVerbNode->getProcessor());
    jassert(convProcessor != nullptr);
    convProcessor->setAPVTS(&apvts); // Pass parameter layout down
    auto& reverb = *convProcessor->getConvolutionReverb();
    reverb.setDecay(0.5f);
    reverb.setUniformWeights();
    // reverb.setWeights(std::array<float, MAX_IR_COUNT> {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}); // Init weights
}

bool MareverbAudioProcessor::loadRandomIR(int irIndex) {
    if (irFiles.isEmpty()) return false;
    int idx = irRNG.nextInt(irFiles.size());
    return loadIR(irIndex, idx);
}

bool MareverbAudioProcessor::loadIR(int irIndex, int fileIndex) {
    if (irIndex < 0 || irIndex >= MAX_IR_COUNT) return false;
    if (fileIndex < 0 && !activeIRFiles[irIndex].existsAsFile()) return false; // Empty, must supply file index
    if (fileIndex >= 0 && fileIndex < irFiles.size()) activeIRFiles[irIndex] = irFiles[fileIndex]; // Set file

    // Load file from activeIRFiles and create reader
    juce::File ir = activeIRFiles[irIndex];
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor(ir));
    if (reader == nullptr) return false;

    // Read samples to buffer in activeIRBuffers
    int numChannels = (int)reader->numChannels; int numSamples = (int)reader->lengthInSamples;
    activeIRBuffers[irIndex].setSize(numChannels, numSamples);
    if (reader->read(&activeIRBuffers[irIndex], 0, numSamples, 0, true, true)) {
        // Set IR buffer in convolution processor
        auto* convProcessor = dynamic_cast<ConvolutionReverbAudioProcessor*>(convolutionVerbNode->getProcessor());
        if (convProcessor != nullptr) {
            convProcessor->getConvolutionReverb()->setIRBuffer(irIndex, activeIRBuffers[irIndex]);
            return true; }
    } return false;
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
void MareverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    mainProcessor->setPlayConfigDetails(getMainBusNumInputChannels(), getMainBusNumOutputChannels(), sampleRate, samplesPerBlock);
    mainProcessor->prepareToPlay(sampleRate, samplesPerBlock);
    connectAudioNodes();
}

void MareverbAudioProcessor::releaseResources() {
    mainProcessor->releaseResources();
}

void MareverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    mainProcessor->processBlock(buffer, midiMessages);
}

// GUI

juce::AudioProcessorEditor* MareverbAudioProcessor::createEditor() { 
    return new juce::GenericAudioProcessorEditor(*this); // TEMP
    // return new MareverbAudioProcessorEditor (*this);
}

// State

void MareverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData) { 

}

void MareverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes) { 

}

// INSTANCING

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MareverbAudioProcessor(); }
