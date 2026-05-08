#include "Core/Utilities.h"
#include "GUI/API/MareAlert.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"

/* PRIVATE */

// Parameters

juce::AudioProcessorValueTreeState::ParameterLayout MareverbAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const auto percentFormat = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float value, int) { return Format::percent(value, 4); });
    const auto frequencyFormat = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float value, int) { return Format::frequency(value, 4); });
    const auto timeFormat = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float value, int) { return Format::seconds(value, 4); });
    // const auto decibelFormat = juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float value, int) { return Format::decibels(value, 4); });

    // Global controls
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::globalMix, "Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 1e-5f, 1.0f), 0.5f, percentFormat));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::decay, "Decay", juce::NormalisableRange<float>(0.0f, 1.0f, 1e-5f, 1.0f), 0.5f, percentFormat));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::crossfadeTime, "Crossfade", juce::NormalisableRange<float>(0.0f, 5.0f, 1e-2f, 1.2f), 1.0f, timeFormat));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::lowCut, "Low Cut", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 20.0f, frequencyFormat));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::highCut, "High Cut", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 20000.0f, frequencyFormat));
    
    // Interaction controls
    layout.add(std::make_unique<juce::AudioParameterChoice>(ParamID::weightingMode, "Weighting Mode", weightingModes, static_cast<int>(WeightingMode::WEIGHTING_ABSOLUTE)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::strength, "Strength", juce::NormalisableRange<float>(0.0f, 1.0f, 1e-5f, 1.0f), 0.5f, percentFormat));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::spread, "Spread", juce::NormalisableRange<float>(0.0f, 1.0f, 1e-5f, 1.0f), 1.0f, percentFormat));

    // Position controls
    layout.add(std::make_unique<juce::AudioParameterChoice>(ParamID::positionPattern, "Position Pattern", positionPatterns, static_cast<int>(PositionPattern::EYES)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::positionRate, "Position Rate", juce::NormalisableRange<float>(-1.0f, 1.0f, 1e-5f, 1.0f), 0.5f, percentFormat));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::positionModA, "Position Mod A", juce::NormalisableRange<float>(0.0f, 1.0f, 1e-3f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::positionModB, "Position Mod B", juce::NormalisableRange<float>(0.0f, 1.0f, 1e-3f, 1.0f), 0.5f));
    
    // Field controls
    layout.add(std::make_unique<juce::AudioParameterChoice>(ParamID::fieldPattern, "Field Pattern", fieldPatterns, static_cast<int>(FieldPattern::RING)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::fieldRate, "Field Rate", juce::NormalisableRange<float>(-1.0f, 1.0f, 1e-5f, 1.0f), 0.5f, percentFormat));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::fieldModA, "Field Mod A", juce::NormalisableRange<float>(0.0f, 1.0f, 1e-3f, 1.0f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID::fieldModB, "Field Mod B", juce::NormalisableRange<float>(0.0f, 1.0f, 1e-3f, 1.0f), 0.5f));

    return layout;
}

void MareverbAudioProcessor::updateParameters() {
    ParameterSettings settings = getParameterSettings(apvts);

    mixer.setWetMixProportion(settings.globalMix);

    if (cutFilterNode) {
        if (auto* cutProcessor = dynamic_cast<CutFilterAudioProcessor*>(cutFilterNode->getProcessor())) {
            if (settings.lowCut != cutProcessor->getLowCutCutoff()) cutProcessor->setLowCutCutoff(settings.lowCut, getSampleRate());
            if (settings.highCut != cutProcessor->getHighCutCutoff()) cutProcessor->setHighCutCutoff(settings.highCut, getSampleRate());
        }
    }
}

// Concurrency

void MareverbAudioProcessor::setControlRate(float nRate) { if (controlThread) controlThread->setControlRate(nRate); }

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

/* PUBLIC */

MareverbAudioProcessor::MareverbAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),

    mainProcessor(new juce::AudioProcessorGraph()), 
    mixer(PARTITION_SIZE),
    irManager(&applicationProperties) {

    auto startTime = std::chrono::steady_clock::now();

    // Init properties
    juce::PropertiesFile::Options options;
    options.applicationName = ProjectInfo::projectName;
    options.filenameSuffix = ".amre";
    options.osxLibrarySubFolder = "Application Support";
    applicationProperties.setStorageParameters(options);

    // Check version
    versionChecker.outdatedCallback = [this](const juce::String& /*currentVersion*/, const juce::String& latestVersion) {
        auto* properties = applicationProperties.getUserSettings();
        if (!properties) return;

        const juce::String lastNotified = properties->getValue(PropertyID::lastNotified);
        if (lastNotified != latestVersion) {
            guiState.outdated.store(true, std::memory_order_release);
            properties->setValue(PropertyID::lastNotified, latestVersion);
            DBG("VERSION: Notifying for " << latestVersion);
            return;
        }
        DBG("VERSION: Already notified about " << latestVersion);
    };
    versionChecker.upToDateCallback = [this](const juce::String& currentVersion) {
        auto* properties = applicationProperties.getUserSettings();
        if (properties) properties->setValue(PropertyID::lastNotified, currentVersion);
        DBG("VERSION: Up to date!");
    };

    juce::Thread::launch([this] { versionChecker.checkForUpdates(ProjectInfo::projectName); });

    // Init IR manager
    irManager.prepare();

    profileTime("INIT: Initialized IR manager; ", startTime);

    // Init pattern states
    patternState.lastPositionPattern = static_cast<PositionPattern>(apvts.getParameter(ParamID::positionPattern)->getDefaultValue());
    patternState.lastFieldPattern = static_cast<FieldPattern>(apvts.getParameter(ParamID::fieldPattern)->getDefaultValue());

    for (int i = 0; i < positionPatterns.size(); ++i)
        patternState.positionParamStates[static_cast<PositionPattern>(i)] = { 
            apvts.getParameter(ParamID::positionRate)->getDefaultValue(),
            apvts.getParameter(ParamID::positionModA)->getDefaultValue(),
            apvts.getParameter(ParamID::positionModB)->getDefaultValue()
        };

    for (int i = 0; i < fieldPatterns.size(); ++i)
        patternState.fieldParamStates[static_cast<FieldPattern>(i)] = {
            apvts.getParameter(ParamID::fieldRate)->getDefaultValue(),
            apvts.getParameter(ParamID::fieldModA)->getDefaultValue(),
            apvts.getParameter(ParamID::fieldModB)->getDefaultValue()
        };

    // Init everything
    initState();

    profileTime("INIT: Initialized pattern states; ", startTime);

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

    profileTime("INIT: Initialized DSP state; ", startTime);

    // Init threads
    controlThread = std::make_unique<ControlThread>(apvts, irManager, guiState, convolutionState);

    profileTime("INIT: Initialized control thread; ", startTime);

    profileTime("INIT: Processor initialization complete in ", startTime);
}

MareverbAudioProcessor::~MareverbAudioProcessor() { }

// Boilerplate

double MareverbAudioProcessor::getTailLengthSeconds() const {
    if (!mainProcessor) return 0.0;
    double tailLength = 0.0;
    for (const auto& node : mainProcessor->getNodes())
        if (node && node->getProcessor()) 
            tailLength = juce::jmax(node->getProcessor()->getTailLengthSeconds(), tailLength);
    
    return tailLength;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MareverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo()) return false;
    else if (!in.isDisabled() && in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo()) return false;
    else return true;
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
    mixer.setWetLatency(PARTITION_SIZE);
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
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    
    updateParameters();

    if (numSamples == 0) return;

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);  

    // Process + mix chain
    mixer.pushDrySamples(buffer);
    mainProcessor->processBlock(buffer, midiMessages);
    mixer.mixWetSamples(buffer);
}

juce::AudioProcessorEditor* MareverbAudioProcessor::createEditor() { return new MareverbAudioProcessorEditor(*this); }

void MareverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData) { 
    auto state = apvts.copyState();

    // Store GUI state
    juce::ValueTree guiTree(TreeID::guiState);

    auto position = guiState.position.load();
    guiTree.setProperty(PropertyID::Position::r, position.r, nullptr);
    guiTree.setProperty(PropertyID::Position::theta, position.theta, nullptr);

    juce::ValueTree fieldTree(TreeID::GUIState::fieldCoordinates);
    {
        juce::SpinLock::ScopedLockType lock(guiState.fieldLock);
        for (int i = 0; i < guiState.fieldCoordinates.size(); ++i) {
            const auto& coordinate = guiState.fieldCoordinates[i];
            juce::ValueTree point(TreeID::GUIState::FieldCoordinates::point);
            point.setProperty(PropertyID::Point::r, coordinate.r, nullptr);
            point.setProperty(PropertyID::Point::theta, coordinate.theta, nullptr);
            fieldTree.addChild(point, -1, nullptr);
        }
    }

    guiTree.addChild(fieldTree, -1, nullptr);
    state.addChild(guiTree, -1, nullptr);

    // Store pattern state
    juce::ValueTree patternStateTree(TreeID::patternState);

    juce::ValueTree positionParamsTree(TreeID::PatternState::positionParamStates);
    juce::ValueTree fieldParamsTree(TreeID::PatternState::fieldParamStates);

    {
        juce::SpinLock::ScopedLockType lock(patternState.patternStateLock);
        for (int i = 0; i < positionPatterns.size(); ++i) {
            juce::String id = TreeID::PatternState::paramState(i);
            juce::ValueTree patternTree(id);
            auto& patternParams = patternState.positionParamStates[static_cast<PositionPattern>(i)];
            patternTree.setProperty(PropertyID::ParameterState::pattern, positionPatterns[i], nullptr);
            patternTree.setProperty(PropertyID::ParameterState::rate, patternParams.rate, nullptr);
            patternTree.setProperty(PropertyID::ParameterState::modA, patternParams.modA, nullptr);
            patternTree.setProperty(PropertyID::ParameterState::modB, patternParams.modB, nullptr);
            positionParamsTree.addChild(patternTree, -1, nullptr);
        }

        for (int i = 0; i < fieldPatterns.size(); ++i) {
            juce::String id = TreeID::PatternState::paramState(i);
            juce::ValueTree patternTree(id);
            auto& patternParams = patternState.fieldParamStates[static_cast<FieldPattern>(i)];
            patternTree.setProperty(PropertyID::ParameterState::pattern, fieldPatterns[i], nullptr);
            patternTree.setProperty(PropertyID::ParameterState::rate, patternParams.rate, nullptr);
            patternTree.setProperty(PropertyID::ParameterState::modA, patternParams.modA, nullptr);
            patternTree.setProperty(PropertyID::ParameterState::modB, patternParams.modB, nullptr);
            fieldParamsTree.addChild(patternTree, -1, nullptr);
        }
    }

    patternStateTree.addChild(positionParamsTree, -1, nullptr);
    patternStateTree.addChild(fieldParamsTree, -1, nullptr);
    state.addChild(patternStateTree, -1, nullptr);

    // Store IR slots
    juce::ValueTree irManagerTree(TreeID::irManagerState);

    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        const auto& slot = irManager.getIRSlot(i);
        juce::ValueTree slotTree(TreeID::IRManager::irSlot);

        if (slot.file.existsAsFile())
            slotTree.setProperty(PropertyID::IRSlot::filePath, slot.file.getFullPathName(), nullptr);

        slotTree.setProperty(PropertyID::IRSlot::occupied, slot.occupied, nullptr);
        slotTree.setProperty(PropertyID::IRSlot::active, slot.active, nullptr);
        slotTree.setProperty(PropertyID::IRSlot::gainDB, slot.gain, nullptr);

        slotTree.setProperty(PropertyID::IRSlot::Window::start, slot.window.start, nullptr);
        slotTree.setProperty(PropertyID::IRSlot::Window::length, slot.window.length, nullptr);
        slotTree.setProperty(PropertyID::IRSlot::Window::Envelope::type, static_cast<int>(slot.window.envelope.type), nullptr);
        slotTree.setProperty(PropertyID::IRSlot::Window::Envelope::attack, slot.window.envelope.attack, nullptr);
        slotTree.setProperty(PropertyID::IRSlot::Window::Envelope::release, slot.window.envelope.release, nullptr);

        slotTree.setProperty(PropertyID::IRSlot::AutoSwap::swapActive, slot.autoSwap.active, nullptr);
        slotTree.setProperty(PropertyID::IRSlot::AutoSwap::swapMin, slot.autoSwap.minTime, nullptr);
        slotTree.setProperty(PropertyID::IRSlot::AutoSwap::swapMax, slot.autoSwap.maxTime, nullptr);

        irManagerTree.addChild(slotTree, -1, nullptr);
    }

    state.addChild(irManagerTree, -1, nullptr);

    juce::MemoryOutputStream mos(destData, true);
    state.writeToStream(mos);

    storePersistentState();

    DBG("RECALL: Stored state information");
}

void MareverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (!tree.isValid()) return;

    apvts.replaceState(tree);
    initState();
}

void MareverbAudioProcessor::storePersistentState() {
    auto* properties = applicationProperties.getUserSettings();
    if (properties) {
        properties->setValue(PropertyID::tooltipsEnabled, apvts.state.getProperty(PropertyID::tooltipsEnabled));
        properties->setValue(PropertyID::positionIndicatorStyle, apvts.state.getProperty(PropertyID::positionIndicatorStyle));
        properties->setValue(PropertyID::fieldIndicatorStyle, apvts.state.getProperty(PropertyID::fieldIndicatorStyle));
        // ...
        properties->saveIfNeeded();
    }
}

void MareverbAudioProcessor::initState() {
    auto startTime = std::chrono::steady_clock::now();
    auto& tree = apvts.state;
   
    DBG("RECALL: Restoring state information...");

    // updateParameters();

    // Restore GUI state
    auto guiTree = tree.getChildWithName(TreeID::guiState);
    if (guiTree.isValid()) {
        PolarCoordinate position{};
        position.r = guiTree.getProperty(PropertyID::Position::r, 0.0f);
        position.theta = guiTree.getProperty(PropertyID::Position::theta, 0.0f);
        guiState.position.store(position);

        auto fieldTree = guiTree.getChildWithName(TreeID::GUIState::fieldCoordinates);
        if (fieldTree.isValid()) {
            juce::SpinLock::ScopedLockType lock(guiState.fieldLock);

            guiState.fieldCoordinates.clear();

            for (int i = 0; i < fieldTree.getNumChildren(); ++i) {
                auto point = fieldTree.getChild(i);
                PolarCoordinate coordinate{};
                coordinate.r = point.getProperty(PropertyID::Point::r, 0.0f);
                coordinate.theta = point.getProperty(PropertyID::Point::theta, 0.0f);
                guiState.fieldCoordinates.push_back(coordinate);
            }
        }
    }

    profileTime("RECALL: Restored GUI (coordinates) state: ", startTime);

    // Restore pattern state
    patternState.lastPositionPattern = static_cast<PositionPattern>(apvts.getRawParameterValue(ParamID::positionPattern)->load());
    patternState.lastFieldPattern = static_cast<FieldPattern>(apvts.getRawParameterValue(ParamID::fieldPattern)->load());
    auto patternStateTree = tree.getChildWithName(TreeID::patternState);
    if (patternStateTree.isValid()) {
        juce::SpinLock::ScopedLockType lock(patternState.patternStateLock);
        auto positionParamTree = patternStateTree.getChildWithName(TreeID::PatternState::positionParamStates);
        for (int i = 0; i < positionPatterns.size(); ++i) {
            auto patternTree = positionParamTree.getChildWithProperty(PropertyID::ParameterState::pattern, positionPatterns[i]);
            if (patternTree.isValid()) {
                auto& patternParams = patternState.positionParamStates[static_cast<PositionPattern>(i)];
                patternParams.rate = patternTree.getProperty(PropertyID::ParameterState::rate, patternParams.rate);
                patternParams.modA = patternTree.getProperty(PropertyID::ParameterState::modA, patternParams.modA);
                patternParams.modB = patternTree.getProperty(PropertyID::ParameterState::modB, patternParams.modB);
            }
        }

        auto fieldParamTree = patternStateTree.getChildWithName(TreeID::PatternState::fieldParamStates);
        for (int i = 0; i < fieldPatterns.size(); ++i) {
            auto patternTree = fieldParamTree.getChildWithProperty(PropertyID::ParameterState::pattern, fieldPatterns[i]);
            if (patternTree.isValid()) {
                auto& patternParams = patternState.fieldParamStates[static_cast<FieldPattern>(i)];
                patternParams.rate = patternTree.getProperty(PropertyID::ParameterState::rate, patternParams.rate);
                patternParams.modA = patternTree.getProperty(PropertyID::ParameterState::modA, patternParams.modA);
                patternParams.modB = patternTree.getProperty(PropertyID::ParameterState::modB, patternParams.modB);
            }
        }
    }

    profileTime("RECALL: Restored pattern state: ", startTime);

    // Restore IR manager state
    auto irManagerTree = tree.getChildWithName(TreeID::irManagerState);
    if (irManagerTree.isValid()) {
        for (int i = 0; i < irManagerTree.getNumChildren(); ++i) {
            const auto& slotTree = irManagerTree.getChild(i);

            // Retrieve values
            const auto filePath = slotTree.getProperty(PropertyID::IRSlot::filePath).toString();
            const bool occupied = slotTree.getProperty(PropertyID::IRSlot::occupied, false);
            const bool active = slotTree.getProperty(PropertyID::IRSlot::active, true);
            const float gainDB = slotTree.getProperty(PropertyID::IRSlot::gainDB, 1.0f);

            const float windowStart = slotTree.getProperty(PropertyID::IRSlot::Window::start, 0.0f);
            const float windowLength = slotTree.getProperty(PropertyID::IRSlot::Window::length, 1.0f);

            const int envelopeIndex = static_cast<int>(slotTree.getProperty(PropertyID::IRSlot::Window::Envelope::type, static_cast<int>(EnvelopeType::HANN)));
            const EnvelopeType envelopeType = static_cast<EnvelopeType>(juce::jmax(0, envelopeIndex));
                
            const float envelopeAttack = slotTree.getProperty(PropertyID::IRSlot::Window::Envelope::attack, 0.0f);
            const float envelopeRelease = slotTree.getProperty(PropertyID::IRSlot::Window::Envelope::release, 0.0f);

            const bool swapActive = slotTree.getProperty(PropertyID::IRSlot::AutoSwap::swapActive, false);
            const float swapMin = slotTree.getProperty(PropertyID::IRSlot::AutoSwap::swapMin, 12.6f);
            const float swapMax = slotTree.getProperty(PropertyID::IRSlot::AutoSwap::swapMax, 21.6f);

            // Enqueue commands
            IRCommand cmd;

            if (occupied && filePath.isNotEmpty()) {
                cmd.type = IRCommand::IR_LOAD;
                cmd.irIndex = i;
                cmd.irFile = juce::File(filePath);
                cmd.shouldCrossfade = false;
                irManager.enqueueCommand(cmd);
            }
 
            cmd.type = IRCommand::IR_SET_ACTIVE_STATE;
            cmd.irIndex = i;
            cmd.irActiveState = active;
            irManager.enqueueCommand(cmd);

            cmd.type = IRCommand::IR_SET_GAIN_DB;
            cmd.irIndex = i;
            cmd.gainDB = gainDB;
            irManager.enqueueCommand(cmd);

            cmd.type = IRCommand::IR_SET_WINDOW;
            cmd.irIndex = i;
            cmd.windowStart = windowStart;
            cmd.windowLength = windowLength;
            irManager.enqueueCommand(cmd);

            cmd.type = IRCommand::IR_SET_ENVELOPE;
            cmd.irIndex = i;
            cmd.envelopeType = envelopeType;
            cmd.envelopeAttack = envelopeAttack;
            cmd.envelopeRelease = envelopeRelease;
            irManager.enqueueCommand(cmd);

            cmd.type = IRCommand::IR_SET_SWAP_INTERVAL;
            cmd.irIndex = i;
            cmd.swapMinTime = swapMin;
            cmd.swapMaxTime = swapMax;
            irManager.enqueueCommand(cmd);

            cmd.type = IRCommand::IR_SET_SWAP_ACTIVE;
            cmd.irIndex = i;
            cmd.swapActiveState = swapActive;
            irManager.enqueueCommand(cmd);
        }
    }

    profileTime("RECALL: Restored IR manager state: ", startTime);

    IRCommand cmd;
    cmd.type = IRCommand::SET_FILE_FILTER;
    cmd.fileFilter = apvts.state.getProperty(PropertyID::fileFilter);
    irManager.enqueueCommand(cmd);

    cmd.type = IRCommand::SET_DIRECTORY_FILTER;
    cmd.directoryFilter = apvts.state.getProperty(PropertyID::directoryFilter);
    irManager.enqueueCommand(cmd);

    cmd.type = IRCommand::SET_SAMPLING_MODE;
    cmd.samplingMode = static_cast<IRSamplingMode>(static_cast<int>(
        apvts.state.getProperty(PropertyID::samplingMode, static_cast<int>(IRSamplingMode::UNIFORM_ACROSS_DIRECTORIES))));;
    irManager.enqueueCommand(cmd);

    const int controlRateIndex = apvts.state.getProperty(PropertyID::controlRate, 3 /* 30 Hz */);
    setControlRate(controlRates[controlRateIndex - 1].removeCharacters(" Hz").getFloatValue());

    // Recall persistent settings
    auto* properties = applicationProperties.getUserSettings();
    if (properties) {
        apvts.state.setProperty(PropertyID::tooltipsEnabled, 
            properties->getBoolValue(PropertyID::tooltipsEnabled, true), nullptr);

        apvts.state.setProperty(PropertyID::positionIndicatorStyle, 
            properties->getValue(PropertyID::positionIndicatorStyle, PositionIndicatorStyle::Anon), nullptr);

        apvts.state.setProperty(PropertyID::fieldIndicatorStyle, 
            properties->getValue(PropertyID::fieldIndicatorStyle, FieldIndicatorStyle::Mareful), nullptr);
    }

    profileTime("RECALL: Restored general settings: ", startTime);

    DBG("RECALL: State recall complete");
}

IRManager* MareverbAudioProcessor::getIRManager() { return &irManager; }

// Instancing

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MareverbAudioProcessor(); }