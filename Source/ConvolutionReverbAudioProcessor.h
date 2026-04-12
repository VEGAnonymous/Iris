#pragma once

#include "ConvolutionReverb.h"
#include "PolarMap.h"
#include "MotionController.h"

#include <JuceHeader.h>

class ConvolutionReverbAudioProcessor : public juce::AudioProcessor {
private:
	// DSP
	ConvolutionReverb convolutionReverb;
	std::shared_ptr<ConvolutionStateHolder> convolutionState;

public:
	ConvolutionReverbAudioProcessor(std::shared_ptr<ConvolutionStateHolder> stateHolder);
	~ConvolutionReverbAudioProcessor() override;

	// Boilerplate
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

	void prepareToPlay(double sampleRate, int maxBlockSize) override;
	void releaseResources() override;
	void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

	juce::AudioProcessorEditor* createEditor() override { return nullptr; }
	bool hasEditor() const override { return false; }

    void getStateInformation (juce::MemoryBlock& /*destData*/) override {}
	void setStateInformation(const void* /*data*/, int /*sizeInBytes*/) override {}
};