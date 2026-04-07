#pragma once

#include "ConvolutionReverb.h"

#include <JuceHeader.h>

class ConvolutionReverbAudioProcessor : public juce::AudioProcessor {
private:
	// DSP
	ConvolutionReverb convolutionReverb;
	juce::dsp::DryWetMixer<float> mixer;
	float t = 0.0f; // Time
	int controlCounter = 0;

	// Parameters
	juce::AudioProcessorValueTreeState* parameters = nullptr;
	float mix = 0.5f, decay = 0.5f;
	float positionR = 0.0f, positionTheta = 0.0f;

	// Weights
	PolarCoordinate position = {0.0f, 0.0f};
	std::array<PolarCoordinate, MAX_IR_COUNT> irCoordinates {}; // Location of each IR
	bool newWeights = false;

	// Motion
	MotionPattern motionPattern = MotionPattern::LISSAJOUS;
	float motionRate = 0.5f;
	float motionModA = 0.5f, motionModB = 0.5f;
	PolarCoordinate randomTarget {0.0f, 0.0f};
	
	// Methods
	void advancePhase();
	void updateIRCoordinates();
	void updatePosition();
	void updateWeights();

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

	void setAPVTS(juce::AudioProcessorValueTreeState* apvts);

	void updateParameters();

	ConvolutionReverb* getConvolutionReverb();
};