#include "Core/Processors/State/ConvolutionMixState.h"

ConvolutionMixState::ConvolutionMixState() {
	resize(MAX_IR_PARTITIONS);
}

void ConvolutionMixState::resize(int partitionCount) {
	for (int channel = 0; channel < N_CHANNELS; ++channel)
		mixedSpectra[channel].resize(partitionCount);

	irEnvelopes.resize(partitionCount);
}

void ConvolutionMixState::setWeights(std::array<std::array<float, MAX_IR_BANK_SLOTS>, N_CHANNELS> weights) { 
	irWeights = weights; 
}

void ConvolutionMixState::setDecay(float decay, int maxPartitions) {
	decay = juce::jlimit(0.0f, 1.0f, decay);

	const float minDB = -180.0f, maxDB = -30.0f;
	float totalDB = minDB + (decay * (maxDB - minDB));

	float alpha = totalDB / static_cast<float>(maxPartitions); // dB
	float step = powf(10.0f, alpha / 20.0f); // Linear

	float currentEnvelope = 1.0f;
	for (int partition = 0; partition < maxPartitions; ++partition) {
		// powf(envelope, static_cast<float>(partition));
		irEnvelopes[partition] = currentEnvelope;
		currentEnvelope *= step;
	}
}

void ConvolutionMixState::mixSpectrum(const ConvolutionIRBank& irBank) {
	const int maxPartitions = irBank.getMaxPartitionCount();

	// Each channel
	auto mixChannel = [&](int channel) {
		juce::FloatVectorOperations::clear(mixedSpectra[channel].data()->data(), maxPartitions * FFT_SIZE);

		// Each partition
		for (int partition = 0; partition < maxPartitions; ++partition) {
			float envelope = irEnvelopes[partition];

			// Each IR
			for (int ir = 0; ir < MAX_IR_BANK_SLOTS; ++ir) {
				// Use only existing + active IRs, channels, and partitions
				if (!irBank.isActive(ir)) continue;

				const int partitionCount = irBank.getPartitionCount(ir);
				if (partitionCount == 0 || partition >= partitionCount) continue;

				int irChannel = std::min(channel, static_cast<int>(irBank.getChannelCount(ir) - 1));

				// Store weighted sum of IRs in irSpectra to mixedSpectra
				const float* irSrc = (irBank.getSpectra(ir))[irChannel][partition].data();
				float* irMix = mixedSpectra[channel][partition].data();
				float weight = irWeights[channel][ir] * envelope * irBank.getGain(ir);
				juce::FloatVectorOperations::addWithMultiply(irMix, irSrc, weight, FFT_SIZE);
			}
		}
	};

	// Parallelization didn't help much here, so perform sequentially
	for (int ch = 0; ch < N_CHANNELS; ch++) mixChannel(ch);

	// DBG("CONV: Mixed spectrum");
}