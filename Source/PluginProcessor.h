#pragma once

#include "ConvolutionReverbAudioProcessor.h"
#include "Defines.h"
#include "MotionController.h"
#include "PolarMap.h"

#include <JuceHeader.h>
#include <array>
#include <vector>

class MareverbAudioProcessor : public juce::AudioProcessor {
private:
    // Settings
    juce::ApplicationProperties applicationProperties;

    // Parameters
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateParameters();

    // IR management
    std::unique_ptr<juce::FileChooser> irFileChooser, irDirectoryChooser;

    juce::Random irRNG;

    struct IRDirectory {
        juce::File irDirectory;
        bool active = true;
    };
    std::vector<IRDirectory> irDirectories;
    juce::Array<juce::File> irFiles;

    juce::AudioFormatManager formatManager;

    std::array<juce::File, MAX_IR_COUNT> activeIRFiles;
    std::array<juce::AudioBuffer<float>, MAX_IR_COUNT> activeIRBuffers;

    void saveDirectories();
    void loadDirectories();

    void chooseIR(int irIndex);
    void chooseIRDirectory();

    void addIRDirectory(juce::File dir);
    void removeIRDirectory(int index);
    void activateIRDirectory(int index, bool nState);

    void collectIRs();

    bool loadIR(int irIndex, juce::File file);
    bool loadRandomIR(int irIndex);
    bool loadRandomIRs();
    void clearIR(int irIndex);
    void clearIRs();

    // Time
    float positionTime = 0.0f, fieldTime = 0.0f;
    int controlCounter = 0;

    void advancePhase();

    // Binaural processing
    void processBinaural(const std::array<float, MAX_IR_COUNT>& rawWeights, const std::vector<PolarCoordinate>& relatives);

    // Polar map, motion
    PolarMap polarMap;
    MotionController motionController;

    // Weights
    std::vector<std::array<float, MAX_IR_COUNT>> irWeights {};
    void updateWeights();

    // Processor graph
    std::unique_ptr<juce::AudioProcessorGraph> mainProcessor;
    juce::AudioProcessorGraph::Node::Ptr audioInputNode, audioOutputNode, convolutionVerbNode;

    void connectAudioNodes();

    ConvolutionReverbAudioProcessor* getConvolutionReverbProcessor() const;

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

    // GUI concurrency
    std::atomic<PolarCoordinate> position { {0.0f, 0.0f} };
    std::atomic<bool> positionChanged { false };

    std::vector<PolarCoordinate> fieldCoordinates;
    std::mutex fieldMutex;
    std::atomic<bool> fieldChanged { false }; // Notify editor
    std::atomic<bool> updateField { false }; // Editor forced update (e.g., parameter changes)

    // Parameters
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };

    static Settings getSettings(juce::AudioProcessorValueTreeState& parameters);
};