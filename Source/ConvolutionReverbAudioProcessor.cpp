#include "ConvolutionReverbAudioProcessor.h"

/* PRIVATE */

juce::AudioProcessorValueTreeState::ParameterLayout ConvolutionReverbAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("Global Mix", "Global Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f), 0.5f));

    return layout;
}

ConvolutionReverbAudioProcessor::Settings ConvolutionReverbAudioProcessor::getSettings(juce::AudioProcessorValueTreeState& parameters) {
    ConvolutionReverbAudioProcessor::Settings settings;

    settings.globalMix = parameters.getRawParameterValue("Global Mix")->load();

    return settings;
}

/* PUBLIC */

ConvolutionReverbAudioProcessor::ConvolutionReverbAudioProcessor() : AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true)), convolutionReverb(2), mixer(HOP_SIZE) {
}

ConvolutionReverbAudioProcessor::~ConvolutionReverbAudioProcessor() {}

// BOILERPLATE

double ConvolutionReverbAudioProcessor::getTailLengthSeconds() const { return 0.0; }

bool ConvolutionReverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() && // Output stereo
        (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())); // Input mono or stereo
}

// DSP

void ConvolutionReverbAudioProcessor::prepareToPlay(double sampleRate, int maxBlockSize) {
    juce::dsp::ProcessSpec spec;
    spec.numChannels = 2;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = maxBlockSize;

    mixer.setMixingRule(juce::dsp::DryWetMixingRule::sin3dB);
    mixer.setWetLatency(HOP_SIZE);
    mixer.setWetMixProportion(1.0f); // TEMP: Test
    mixer.prepare(spec);
}

void ConvolutionReverbAudioProcessor::releaseResources() {

}

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

ConvolutionReverb* ConvolutionReverbAudioProcessor::getConvolutionReverb() { return &convolutionReverb; }