#include "Core/Utilities.h"
#include "Core/Processors/ConvolutionReverbAudioProcessor.h"

/* PUBLIC */

ConvolutionReverbAudioProcessor::ConvolutionReverbAudioProcessor(std::shared_ptr<ConvolutionStateHolder> stateHolder) 
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    convolutionState(stateHolder), convolutionReverb(stateHolder)
{ }

ConvolutionReverbAudioProcessor::~ConvolutionReverbAudioProcessor() {}

// Boilerplate
double ConvolutionReverbAudioProcessor::getTailLengthSeconds() const {
    // HACK: May be innaccurate since the partition count can change over time
    auto state = std::atomic_load(&convolutionState->state);
    if (!state || state->irBank->getMaxPartitionCount() == 0) return 0.0;
    return (state->irBank->getMaxPartitionCount() * PARTITION_SIZE) / getSampleRate();
}

bool ConvolutionReverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() && // Output stereo
           (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())); // Input mono or stereo
}

// DSP
void ConvolutionReverbAudioProcessor::prepareToPlay(double /* sampleRate */, int /* maxBlockSize */) {}

void ConvolutionReverbAudioProcessor::releaseResources() { }

void ConvolutionReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (buffer.getNumChannels() == 1) {
        buffer.setSize(2, buffer.getNumSamples(), true, true);
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
    }

    convolutionReverb.process(buffer);
}