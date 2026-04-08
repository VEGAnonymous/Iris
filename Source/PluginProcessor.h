#pragma once

#include "Defines.h"
#include "PolarMap.h"
#include "MotionController.h"

#include <JuceHeader.h>
#include <array>
#include <vector>

class MareverbAudioProcessor : public juce::AudioProcessor {
private:
    // Parameters

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateParameters();

    // IR management

    juce::Random irRNG;

    juce::File irDirectory;
    juce::Array<juce::File> irFiles;

    juce::AudioFormatManager formatManager;

    std::array<juce::File, MAX_IR_COUNT> activeIRFiles;
    std::array<juce::AudioBuffer<float>, MAX_IR_COUNT> activeIRBuffers;

    // Time
    float globalTime = 0.0f;
    int controlCounter = 0;

    void advancePhase();

    // Polar map, motion, weights

    PolarMap polarMap;
    MotionController motionController;
    std::vector<std::array<float, MAX_IR_COUNT>> irWeights {};

    void updateWeights();

    // Processor graph

    std::unique_ptr<juce::AudioProcessorGraph> mainProcessor;
    juce::AudioProcessorGraph::Node::Ptr audioInputNode, audioOutputNode, convolutionVerbNode;

    void connectAudioNodes();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MareverbAudioProcessor)

public:
    MareverbAudioProcessor();
    ~MareverbAudioProcessor() override;

    bool loadIR(int irIndex, int fileIndex = -1);
    bool loadRandomIR(int irIndex);

    // Boilerplate
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {};
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {};
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    // DSP
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // GUI
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // State
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };

    static Settings getSettings(juce::AudioProcessorValueTreeState& parameters);

    std::atomic<PolarCoordinate> position {{0.0f, 0.0f}};
    std::atomic<bool> positionChanged {false};
};