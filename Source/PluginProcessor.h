#pragma once

#include <JuceHeader.h>

class MareverbAudioProcessor : public juce::AudioProcessor {
private:

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};

    struct Settings {
        float globalMix;

        Settings() : globalMix(0.5f) { }
    };

    Settings getSettings(juce::AudioProcessorValueTreeState& apvts);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MareverbAudioProcessor)

public:
    MareverbAudioProcessor();
    ~MareverbAudioProcessor() override;

    // BOILERPLATE
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    // DSP
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // GUI
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    // STATE
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
};
