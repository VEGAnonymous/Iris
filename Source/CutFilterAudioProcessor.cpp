#include "CutFilterAudioProcessor.h"

/* PUBLIC */

CutFilterAudioProcessor::CutFilterAudioProcessor() : AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

CutFilterAudioProcessor::~CutFilterAudioProcessor() {}

// Boilerplate
bool CutFilterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::stereo() && out == juce::AudioChannelSet::stereo()); // I/O stereo
}

// DSP
void CutFilterAudioProcessor::prepareToPlay(double sampleRate, int maxBlockSize) {
    juce::dsp::ProcessSpec spec { 
        sampleRate, 
        static_cast<juce::uint32>(maxBlockSize), 
        static_cast<juce::uint32>(getTotalNumOutputChannels()) 
    };
    
    setLowCutCutoff(lowCutCutoff, sampleRate);
    setHighCutCutoff(highCutCutoff, sampleRate);

    chain.prepare(spec);
}

void CutFilterAudioProcessor::releaseResources() { chain.reset(); }

void CutFilterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    chain.process(context);
}

// State
void CutFilterAudioProcessor::setLowCutCutoff(float nCutoff, double sampleRate) {
    lowCutCutoff = nCutoff;
    auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(lowCutCutoff, sampleRate, 1);
    *chain.get<static_cast<int>(CutFilterIndex::LOW_CUT)>().state = *lowCutCoefficients[0];
}

void CutFilterAudioProcessor::setHighCutCutoff(float nCutoff, double sampleRate) {
    highCutCutoff = nCutoff;
    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(highCutCutoff, sampleRate, 1);
    *chain.get<static_cast<int>(CutFilterIndex::HIGH_CUT)>().state = *highCutCoefficients[0];
}

float CutFilterAudioProcessor::getLowCutCutoff() const { return lowCutCutoff; }
float CutFilterAudioProcessor::getHighCutCutoff() const { return highCutCutoff; }