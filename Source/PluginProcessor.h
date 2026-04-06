#pragma once

#include "Defines.h"

#include <JuceHeader.h>
#include <array>
#include <vector>

class MareverbAudioProcessor : public juce::AudioProcessor {
private:
    // Parameters
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};

    struct Settings {
        float globalMix;

        Settings() : globalMix(0.5f) { }
    };

    Settings getSettings(juce::AudioProcessorValueTreeState& apvts);

    // IR management

    juce::Random irRNG;

    juce::File irDirectory;
    juce::Array<juce::File> irFiles;

    juce::AudioFormatManager formatManager;

    std::array<juce::File, MAX_IR_COUNT> activeIRFiles;
    std::array<juce::AudioBuffer<float>, MAX_IR_COUNT> activeIRBuffers;

    bool loadIR(int irIndex, int fileIndex = -1);
    bool loadRandomIR(int irIndex);

    // Processor graph

    std::unique_ptr<juce::AudioProcessorGraph> mainProcessor;
    juce::AudioProcessorGraph::Node::Ptr audioInputNode, audioOutputNode, convolutionVerbNode;

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
    void setStateInformation (const void* data, int sizeInBytes) override;
};
