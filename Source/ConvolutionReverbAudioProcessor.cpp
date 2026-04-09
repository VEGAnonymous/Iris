#include "Utilities.h"
#include "ConvolutionReverbAudioProcessor.h"

/* PRIVATE */

/* PUBLIC */

ConvolutionReverbAudioProcessor::ConvolutionReverbAudioProcessor() : AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true)), convolutionReverb(2), mixer(HOP_SIZE), weights(2) {
}

ConvolutionReverbAudioProcessor::~ConvolutionReverbAudioProcessor() {}

// BOILERPLATE

double ConvolutionReverbAudioProcessor::getTailLengthSeconds() const { return 0.0; } // TODO: Put actual value here

bool ConvolutionReverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() && // Output stereo
        (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())); // Input mono or stereo
}

// DSP

void ConvolutionReverbAudioProcessor::prepareToPlay(double sampleRate, int maxBlockSize) {
    juce::dsp::ProcessSpec spec {sampleRate, static_cast<juce::uint32>(maxBlockSize), 2};

    mixer.setMixingRule(juce::dsp::DryWetMixingRule::sin3dB);
    mixer.setWetLatency(HOP_SIZE);
    mixer.setWetMixProportion(mix);
    mixer.prepare(spec);

    setMix(mix); setDecay(decay); setWeights(weights);
}

void ConvolutionReverbAudioProcessor::releaseResources() { }

void ConvolutionReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (buffer.getNumChannels() == 1) {
        buffer.setSize(2, buffer.getNumSamples(), true, true);
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
    }

    mixer.pushDrySamples(buffer);
    convolutionReverb.process(buffer);
    mixer.mixWetSamples(buffer);
}

// STATE

void ConvolutionReverbAudioProcessor::setMix(float nMix) {
    if (nMix != mix) { mix = nMix; mixer.setWetMixProportion(nMix); }
}
void ConvolutionReverbAudioProcessor::setDecay(float nDecay) {
    if (nDecay != decay) { decay = nDecay; convolutionReverb.setDecay(nDecay); }
}

void ConvolutionReverbAudioProcessor::setWeights(std::vector<std::array<float, MAX_IR_COUNT>> nWeights) {
    if (nWeights != weights) weights = nWeights; convolutionReverb.setWeights(nWeights);
}

ConvolutionReverb* ConvolutionReverbAudioProcessor::getConvolutionReverb() { return &convolutionReverb; }