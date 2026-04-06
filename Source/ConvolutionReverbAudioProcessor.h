#pragma once

#include "ConvolutionReverb.h"

#include <JuceHeader.h>

class ConvolutionReverbAudioProcessor : public juce::AudioProcessor {
private:
	// DSP
	ConvolutionReverb convolutionReverb;
	juce::dsp::DryWetMixer<float> mixer;
	// Parameters
	juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
	juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };

	struct Settings {
		float globalMix;

		Settings() : globalMix(0.5f) { }
	};

	Settings getSettings(juce::AudioProcessorValueTreeState& apvts);

public:
	ConvolutionReverbAudioProcessor();
	~ConvolutionReverbAudioProcessor() override;

	// BOILERPLATE
	const juce::String getName() const override { return "Convolution Reverb"; }
	bool acceptsMidi() const override { return false; }
	bool producesMidi() const override { return false; }
	bool isMidiEffect() const override { return false; }
	double getTailLengthSeconds() const override;
	int getNumPrograms() override { return 1; }
	int getCurrentProgram() override { return 0; }
	void setCurrentProgram(int) override {}
	const juce::String getProgramName(int) override { return {}; }
	void changeProgramName(int, const juce::String&) override {}
#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

	// DSP
	void prepareToPlay(double sampleRate, int maxBlockSize) override;
	void releaseResources() override;
	void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

	// GUI
	juce::AudioProcessorEditor* createEditor() override { return nullptr; }
	bool hasEditor() const override { return false; }

	// STATE
    void getStateInformation (juce::MemoryBlock& destData) override {}
	void setStateInformation(const void* data, int sizeInBytes) override {}

	ConvolutionReverb* getConvolutionReverb();
};