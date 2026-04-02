#include "PluginProcessor.h"
#include "PluginEditor.h"

/* PRIVATE */

juce::AudioProcessorValueTreeState::ParameterLayout MareverbAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("Global Mix", "Global Mix", 0.0f, 1.0f, 0.5f));

    return layout;
}

MareverbAudioProcessor::Settings MareverbAudioProcessor::getSettings(juce::AudioProcessorValueTreeState& apvts) {
    MareverbAudioProcessor::Settings settings;

    settings.globalMix = apvts.getRawParameterValue("Global Mix")->load();

    return settings;
}

/* PUBLIC */

MareverbAudioProcessor::MareverbAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{ }

MareverbAudioProcessor::~MareverbAudioProcessor() { }

// BOILERPLATE
const juce::String MareverbAudioProcessor::getName() const { return JucePlugin_Name; }
bool MareverbAudioProcessor::acceptsMidi() const { return false; }
bool MareverbAudioProcessor::producesMidi() const { return false; }
bool MareverbAudioProcessor::isMidiEffect() const { return false; }
double MareverbAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int MareverbAudioProcessor::getNumPrograms() { return 1; }
int MareverbAudioProcessor::getCurrentProgram() { return 0; }
void MareverbAudioProcessor::setCurrentProgram (int index) { }
const juce::String MareverbAudioProcessor::getProgramName (int index) { return {}; }
void MareverbAudioProcessor::changeProgramName (int index, const juce::String& newName) { }

#ifndef JucePlugin_PreferredChannelConfigurations
bool MareverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    #if JucePlugin_IsMidiEffect
        juce::ignoreUnused(layouts);
        return true;
    #else
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
            && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;

    #if ! JucePlugin_IsSynth
        if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) return false;
    #endif
        return true;
    #endif
}
#endif

// DSP
void MareverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;


}

void MareverbAudioProcessor::releaseResources() { }

void MareverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

/* GUI */

bool MareverbAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* MareverbAudioProcessor::createEditor() { 
    return new juce::GenericAudioProcessorEditor(*this); // TEMP
    // return new MareverbAudioProcessorEditor (*this);
}

/* STATE */

void MareverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData) { }
void MareverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes) { }

/* INSTANCING */

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MareverbAudioProcessor(); }
