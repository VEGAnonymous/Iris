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

    for (int ir = 0; ir < MAX_IR_COUNT; ++ir) {
        float nMin = apvts.getRawParameterValue(ParamID::irSwapMin(ir))->load();
        float nMax = apvts.getRawParameterValue(ParamID::irSwapMax(ir))->load();
        irManager.setSwapInterval(ir, nMin, nMax);
    }
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
    irManager(&applicationProperties) {

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

    // Init threads
    controlThread = std::make_unique<ControlThread>(apvts, irManager, guiState, convolutionState);
}

MareverbAudioProcessor::~MareverbAudioProcessor() { }

// Boilerplate

double MareverbAudioProcessor::getTailLengthSeconds() const { return 0.0; } // TODO: Replace with actual value

#ifndef JucePlugin_PreferredChannelConfigurations
bool MareverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() && // Output stereo
           (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo()));
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

    if (controlThread) controlThread->startThread();
}

void MareverbAudioProcessor::releaseResources() {
    mainProcessor->releaseResources();
    if (controlThread) controlThread->stopThread(-1);
}

void MareverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    updateParameters();

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