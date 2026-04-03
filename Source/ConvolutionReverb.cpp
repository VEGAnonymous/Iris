#include "ConvolutionReverb.h"

/* PUBLIC */

ConvolutionReverb::ConvolutionReverb() : irBuffers() {
	setUniformWeights();
}

void ConvolutionReverb::setIRBuffer(int index, juce::AudioBuffer<float> irBuffer) { irBuffers[index] = irBuffer; }

void ConvolutionReverb::setUniformWeights() { irWeights.fill(1.0f / MAX_IR_COUNT); }

void ConvolutionReverb::setWeights(std::array<float, MAX_IR_COUNT> weights) { irWeights = weights; }

void ConvolutionReverb::process(const juce::AudioBuffer<float>& in) {
	// Passthrough
}