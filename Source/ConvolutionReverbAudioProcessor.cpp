#include "ConvolutionReverbAudioProcessor.h"

/* PRIVATE */

void ConvolutionReverbAudioProcessor::advancePhase() {
    float freq = motionRate * 0.2f;
    float phaseIncrement = juce::MathConstants<float>::twoPi * freq * (getBlockSize() / getSampleRate());
    t += phaseIncrement;
}

PolarCoordinate ConvolutionReverbAudioProcessor::computeDistanceDirection(PolarCoordinate p1, PolarCoordinate p2, Axis reference) {
    float d = 0.0f, phi = 0.0f;
    float dTheta = p2.theta - p1.theta;
    d = sqrtf((p1.r * p1.r) + (p2.r * p2.r) - (2 * p1.r * p2.r * cosf(dTheta)));

    if (reference == Axis::X_AXIS)
        phi = atan2f((p2.r * sinf(p2.theta)) - (p1.r * sinf(p1.theta)), (p2.r * cosf(p2.theta)) - (p1.r * cosf(p1.theta)));
    else // Y_AXIS
        phi = atan2f((p2.r * cosf(p2.theta)) - (p1.r * cosf(p1.theta)), (p2.r * sinf(p2.theta)) - (p1.r * sinf(p1.theta)));

    return { d, phi };
}

void ConvolutionReverbAudioProcessor::updateIRCoordinates() {
    // newWeights = true;
}

void ConvolutionReverbAudioProcessor::updatePosition() {
    switch (motionPattern) {
        case MotionPattern::RANDOM_WALK: {
            break;
        }
        case MotionPattern::LISSAJOUS: {
            float b = 1.0f, phi = juce::MathConstants<float>::halfPi;
            int p = 3, q = 5; // TODO: Parameterize p and q?

            float r = b * sinf(static_cast<float>(p) * t);
            float theta = phi * sinf((static_cast<float>(q) * t) + phi);
            position = { r, theta };
            // DBG("Position: " << r << ", " << theta);
            newWeights = true;
            break;
        }
    }
}

void ConvolutionReverbAudioProcessor::updateWeights() {
    if (!newWeights) return;

    std::array<float, MAX_IR_COUNT> tempWeights{};
    std::vector<std::array<float, MAX_IR_COUNT>> weights(2);

    const float distanceFactor = 2.0f;
    for (int ir = 0; ir < MAX_IR_COUNT; ++ir) {
        PolarCoordinate rel = computeDistanceDirection(position, irCoordinates[ir], Axis::Y_AXIS);
        tempWeights[ir] = 1.0f / powf(rel.r + 1e-6f, distanceFactor); // Inverse distance weights
        // TODO: Do binaural stuff here
    }

    float sum = std::accumulate(tempWeights.begin(), tempWeights.end(), 0.0f);
    if (sum > 0.0f) for (int ir = 0; ir < MAX_IR_COUNT; ++ir) tempWeights[ir] /= sum; // Normalize weights sum to 1

    for (int channel = 0; channel < 2; ++channel)
        for (int ir = 0; ir < MAX_IR_COUNT; ++ir) weights[channel][ir] = tempWeights[ir];
    
    convolutionReverb.setWeights(weights);
    newWeights = false;
}

/* PUBLIC */

ConvolutionReverbAudioProcessor::ConvolutionReverbAudioProcessor() : AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true)), convolutionReverb(2), mixer(HOP_SIZE) {
    
    // Initialize IR coordinates
    for (int ir = 0; ir < MAX_IR_COUNT; ++ir)
        irCoordinates[ir] = {1.0f, (ir * juce::MathConstants <float>::twoPi) / MAX_IR_COUNT};
    newWeights = true;
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
    mixer.setWetMixProportion(mix);
    mixer.prepare(spec);

    updateParameters();
    updateIRCoordinates();
    updatePosition();
    updateWeights();
}

void ConvolutionReverbAudioProcessor::releaseResources() {

}

void ConvolutionReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (buffer.getNumChannels() == 1) {
        buffer.setSize(2, buffer.getNumSamples(), true, true);
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
    }

    updateParameters();
    advancePhase();

    // Control rate updates
    controlCounter += buffer.getNumSamples();
    if (controlCounter >= static_cast<int>(getSampleRate() / CONTROL_RATE)) {
        updateIRCoordinates();
        updatePosition();
        updateWeights();
        controlCounter = 0;
    }

    mixer.pushDrySamples(buffer);
    convolutionReverb.process(buffer);
    mixer.mixWetSamples(buffer);
}

// STATE

void ConvolutionReverbAudioProcessor::setAPVTS(juce::AudioProcessorValueTreeState* apvts) { parameters = apvts; }

void ConvolutionReverbAudioProcessor::updateParameters() {
    if (!parameters) return;

    const float& nMix = parameters->getRawParameterValue("Global Mix")->load();
    const float& nDecay = parameters->getRawParameterValue("Decay")->load();
    const MotionPattern& nMotionPattern = static_cast<MotionPattern>(parameters->getRawParameterValue("Motion Pattern")->load());
    const float& nMotionRate = parameters->getRawParameterValue("Motion Rate")->load();

    if (nMix != mix) { mix = nMix; mixer.setWetMixProportion(nMix); }
    if (nDecay != decay) { decay = nDecay; convolutionReverb.setDecay(nDecay); }
    if (nMotionPattern != motionPattern) { motionPattern = nMotionPattern; }
    if (nMotionRate != motionRate) { motionRate = nMotionRate; }
}

ConvolutionReverb* ConvolutionReverbAudioProcessor::getConvolutionReverb() { return &convolutionReverb; }