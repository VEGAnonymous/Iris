#pragma once

#include "Core/Defines.h"
#include "Core/Utilities.h"
#include "Core/Processors/ConvolutionReverbAudioProcessor.h"
#include "Core/Processors/CutFilterAudioProcessor.h"
#include "Core/Control/ControlThread.h"
#include "Core/Control/IRManager.h"
#include "Core/Control/State/GUIState.h"
#include "Core/Control/State/PatternState.h"
#include "Core/Control/State/ParameterSettings.h"

#include <JuceHeader.h>
#include <array>
#include <map>
#include <vector>

class MareverbAudioProcessor : public juce::AudioProcessor {
private:
    // Properties
    juce::ApplicationProperties applicationProperties;

    // Parameters
    float mix = 0.5f; 

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateParameters();

    // IR management
    IRManager irManager;

    // Concurrency
    std::unique_ptr<ControlThread> controlThread;

    // Processor graph
    std::unique_ptr<juce::AudioProcessorGraph> mainProcessor;
    juce::AudioProcessorGraph::Node::Ptr audioInputNode, audioOutputNode, convolutionVerbNode, cutFilterNode;
    juce::dsp::DryWetMixer<float> mixer;

    void connectAudioNodes();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MareverbAudioProcessor)

public:
    MareverbAudioProcessor();
    ~MareverbAudioProcessor() override;

    // Boilerplate
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int /*index*/) override {};
    const juce::String getProgramName(int /*index*/) override { return {}; }
    void changeProgramName(int /*index*/, const juce::String& /*newName*/) override {};
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameters
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };

    // Concurrency
    void setControlRate(float nRate);

    // State
    std::shared_ptr<ConvolutionStateHolder> convolutionState;
    GUIState guiState;
    PatternState patternState;

    void storePersistentState();
    void initState();

    IRManager* getIRManager();
};