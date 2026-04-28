#pragma once

#include "Core/Defines.h"

#include <JuceHeader.h>

class CutFilterAudioProcessor : public juce::AudioProcessor {
private:
	enum class CutFilterIndex { LOW_CUT, HIGH_CUT };
	using StereoFilter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
	using FilterChain = juce::dsp::ProcessorChain<StereoFilter, StereoFilter>;

	FilterChain chain;

	// Parameters
	float lowCutCutoff = 20.0f, highCutCutoff = 20000.0f;
public:
	CutFilterAudioProcessor();
	~CutFilterAudioProcessor() override;

	// Boilerplate
	const juce::String getName() const override { return "Convolution Reverb"; }
	bool acceptsMidi() const override { return false; }
	bool producesMidi() const override { return false; }
	bool isMidiEffect() const override { return false; }
	double getTailLengthSeconds() const override { return 0.0f; }
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

	void getStateInformation(juce::MemoryBlock& /*destData*/) override {}
	void setStateInformation(const void* /*data*/, int /*sizeInBytes*/) override {}

	// State
	void setLowCutCutoff(float nCutoff, double sampleRate);
	void setHighCutCutoff(float nCutoff, double sampleRate);
	
	float getLowCutCutoff() const;
	float getHighCutCutoff() const;
};