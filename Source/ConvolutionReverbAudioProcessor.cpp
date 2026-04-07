#include "Utilities.h"
#include "ConvolutionReverbAudioProcessor.h"

/* PRIVATE */

void ConvolutionReverbAudioProcessor::advancePhase() {
    float freq = motionRate * 0.2f;
    float phaseIncrement = juce::MathConstants<float>::twoPi * freq * (getBlockSize() / getSampleRate());
    t += phaseIncrement;
}

void ConvolutionReverbAudioProcessor::updateIRCoordinates() {
    // newWeights = true;
}

void ConvolutionReverbAudioProcessor::updatePosition() {
    switch (motionPattern) {
        case MotionPattern::MANUAL: {
            float r = motionModA, theta = motionModB * juce::MathConstants<float>::twoPi;
            if (r - position.r < 1e-3 && theta - position.theta < 1e-3) return;
            position = { r, theta };
            newWeights = true;
            break;
        }
        case MotionPattern::ORBIT: {
            float radius = motionModA;

            float r = radius;
            float theta = t;
            position = { r, theta };
            newWeights = true;
            break;
        }
        case MotionPattern::SPIRAL: {
            float swirliness = juce::jmap(motionModA, 1.0f, 10.0f);

            float r = sinf(0.1f * t);
            float theta = swirliness * t;
            position = { r, theta };
            newWeights = true;
            break;
        }
        case MotionPattern::FLORAL: {
            float p = floorf(juce::jmap(motionModA, 1.0f, 10.0f)), q = floorf(juce::jmap(motionModB, 1.0f, 10.0f)); // p, q

            float r = sinf(p * t);
            float theta = q * t;
            position = { r, theta };
            newWeights = true;
            break;
        }
        case MotionPattern::LISSAJOUS: {
            float p = floorf(juce::jmap(motionModA, 1.0f, 10.0f)), q = floorf(juce::jmap(motionModB, 1.0f, 10.0f)); // p, q
            const float phi = juce::MathConstants<float>::halfPi;

            float r = sinf(p * t);
            float theta = phi * sinf((q * t) + phi);
            position = { r, theta };
            newWeights = true;
            break;
        }
        case MotionPattern::RANDOM_DISCRETE: {
            float radius = motionModA, smoothing = motionModB; // radius, smoothing

            float probability = motionRate * 0.1f;
            if (randFloat() < probability) { 
                // Set new target
                float r = std::sqrt(randFloat()) * radius;
                float theta = randFloat() * juce::MathConstants<float>::twoPi;
                randomTarget = { r, theta };
            }

            if (position.r - randomTarget.r < 1e-3 && position.theta - randomTarget.theta < 1e-3) return;

            // Smooth toward target position
            float nextR = juce::jmap(smoothing, position.r, randomTarget.r);
            float dTheta = std::fmod(randomTarget.theta - position.theta + (juce::MathConstants<float>::pi * 3),
                juce::MathConstants<float>::twoPi) - juce::MathConstants<float>::pi;
            float nextTheta = position.theta + (dTheta * (1.0f - smoothing));

            position = { nextR, nextTheta };
            newWeights = true;
            break;
        }
        case MotionPattern::RANDOM_WALK: {
            float radius = motionModA;

            float step = 0.02f * motionRate;
            // Convert to Cartesian
            float x = position.r * std::cos(position.theta), y = position.r * std::sin(position.theta);
            // Step and constrain radius
            x += randSigned() * step; y += randSigned() * step;
            float mag = std::sqrt((x * x) + (y * y));
            if (mag > radius) { x /= mag; y /= mag; }
            // Convert to polar
            float r = std::sqrt((x * x) + (y * y));
            float theta = std::atan2(y, x);

            position = { r, theta };
            newWeights = true;
            break;
        }
    } // DBG("Position: " << r << ", " << theta);
}

void ConvolutionReverbAudioProcessor::updateWeights() {
    if (!newWeights) return;

    std::array<float, MAX_IR_COUNT> tempWeights{};
    std::vector<std::array<float, MAX_IR_COUNT>> weights(2);

    const float distanceFactor = 2.0f;
    for (int ir = 0; ir < MAX_IR_COUNT; ++ir) {
        PolarCoordinate rel = computeDistanceDirection(position, irCoordinates[ir], Axis::Y_AXIS, false);
        tempWeights[ir] = 1.0f / powf(rel.r + 1e-6f, distanceFactor); // Inverse-distance weights
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

    // Block rate updates
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
    const float& nMotionModA = parameters->getRawParameterValue("Motion Mod A")->load();
    const float& nMotionModB = parameters->getRawParameterValue("Motion Mod B")->load();

    if (nMix != mix) { mix = nMix; mixer.setWetMixProportion(nMix); }
    if (nDecay != decay) { decay = nDecay; convolutionReverb.setDecay(nDecay); }
    if (nMotionPattern != motionPattern) { motionPattern = nMotionPattern; }
    if (nMotionRate != motionRate) { motionRate = nMotionRate; }
    if (nMotionModA != motionModA) { motionModA = nMotionModA; }
    if (nMotionModB != motionModB) { motionModB = nMotionModB; }
}

ConvolutionReverb* ConvolutionReverbAudioProcessor::getConvolutionReverb() { return &convolutionReverb; }