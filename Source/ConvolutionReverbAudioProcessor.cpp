#include "Utilities.h"
#include "ConvolutionReverbAudioProcessor.h"

/* PUBLIC */

ConvolutionReverbAudioProcessor::ConvolutionReverbAudioProcessor() : AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true)), convolutionReverb(2), weights(2) {
}

ConvolutionReverbAudioProcessor::~ConvolutionReverbAudioProcessor() {}

// Boilerplate
double ConvolutionReverbAudioProcessor::getTailLengthSeconds() const { return 0.0; } // TODO: Put actual value here

bool ConvolutionReverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() && // Output stereo
           (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())); // Input mono or stereo
}

// DSP
void ConvolutionReverbAudioProcessor::prepareToPlay(double sampleRate, int maxBlockSize) {
    setDecay(decay); 
    setWeights(weights);
}

void ConvolutionReverbAudioProcessor::releaseResources() { }

void ConvolutionReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (buffer.getNumChannels() == 1) {
        buffer.setSize(2, buffer.getNumSamples(), true, true);
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
    }

    convolutionReverb.process(buffer);
}

// State

void ConvolutionReverbAudioProcessor::setDecay(float nDecay) {
    if (nDecay != decay) { 
        decay = nDecay; 
        convolutionReverb.setDecay(nDecay); 
    }
}

void ConvolutionReverbAudioProcessor::setWeights(std::vector<std::array<float, MAX_IR_COUNT>> nWeights) {
    if (nWeights != weights) {
        weights = nWeights;
        convolutionReverb.setWeights(nWeights);
    }
}

ConvolutionReverb* ConvolutionReverbAudioProcessor::getConvolutionReverb() { return &convolutionReverb; }