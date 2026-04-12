#include "ConvolutionState.h"
#include "Utilities.h"

/* PUBLIC */

ConvolutionState::ConvolutionState() : fft(FFT_SIZE) {
	// Preallocate spectra storage
	for (int ir = 0; ir < MAX_IR_COUNT; ++ir) {
		irSpectra[ir] = std::make_shared<SpectraData>();
		for (int channel = 0; channel < N_CHANNELS; ++channel)
			(*irSpectra[ir])[channel].resize(MAX_IR_PARTITIONS);
	}

	for (int channel = 0; channel < N_CHANNELS; ++channel)
		mixedSpectra[channel].resize(MAX_IR_PARTITIONS);

	irEnvelopes.resize(MAX_IR_PARTITIONS);
}

ConvolutionState::ConvolutionState(const ConvolutionState& other) : fft(FFT_ORDER),
	irPartition(other.irPartition),
	irSpectra(other.irSpectra),
	irWeights(other.irWeights),
	irEnvelopes(other.irEnvelopes),
	irPartitionCounts(other.irPartitionCounts),
	irChannelCounts(other.irChannelCounts),
	maxIRPartitionCount(other.maxIRPartitionCount),
	mixedSpectra(other.mixedSpectra) {}

void ConvolutionState::updateMaxIRPartitionCount() {
	// Find # partitions in longest IR
	maxIRPartitionCount = 0;
	for (int ir = 0; ir < MAX_IR_COUNT; ++ir)
		maxIRPartitionCount = std::max(maxIRPartitionCount, irPartitionCounts[ir]);

	DBG("Max partition count: " << maxIRPartitionCount);
}

void ConvolutionState::setIR(int irIndex, const juce::AudioBuffer<float>& irBuffer) {
	DBG("Updating IRFFT: " << irIndex);

	auto nSpectra = std::make_shared<SpectraData>();
	

	const int channelCount = std::min(irBuffer.getNumChannels(), N_CHANNELS);
	const int partitionCount = (irBuffer.getNumSamples() + PARTITION_SIZE - 1) / PARTITION_SIZE;

	irChannelCounts[irIndex] = channelCount;
	irPartitionCounts[irIndex] = partitionCount;

	// Each channel
	for (int channel = 0; channel < channelCount; ++channel) {
		(*nSpectra)[channel].resize(MAX_IR_PARTITIONS);

		// Each partition
		for (int partition = 0; partition < partitionCount; ++partition) {
			std::fill(irPartition.begin(), irPartition.end(), 0.0f);

			// Copy 2L samples from IR buffer to partition buffer (rest are 0.0f)
			int sampleOffset = partition * PARTITION_SIZE;
			juce::FloatVectorOperations::copy(irPartition.data(),
				irBuffer.getReadPointer(channel) + sampleOffset,
				std::min(irBuffer.getNumSamples() - sampleOffset, PARTITION_SIZE));

			// Forward FFT in-place
			fft.performRealOnlyForwardTransform(irPartition.data());

			// Store FFT spectra
			auto& spectraDest = (*nSpectra)[channel][partition];
			juce::FloatVectorOperations::copy(spectraDest.data(), irPartition.data(), FFT_SIZE);
		}
	}
	irSpectra[irIndex] = nSpectra;
	updateMaxIRPartitionCount();
	DBG("Computed FFT for buffer " << irIndex);
}

void ConvolutionState::setDecay(float decay) {
	decay = juce::jlimit(0.0f, 1.0f, decay);
	const float minDB = -180.0f, maxDB = -30.0f;
	float totalDB = minDB + (decay * (maxDB - minDB));

	float alpha = totalDB / static_cast<float>(maxIRPartitionCount); // dB
	float envelope = powf(10.0f, alpha / 20.0f); // Linear
	for (int partition = 0; partition < maxIRPartitionCount; ++partition)
		irEnvelopes[partition] = powf(envelope, static_cast<float>(partition));
}

void ConvolutionState::mixSpectrum() {
	// Each channel
	for (int channel = 0; channel < N_CHANNELS; ++channel) {
		juce::FloatVectorOperations::clear(mixedSpectra[channel].data()->data(), maxIRPartitionCount * FFT_SIZE);

		// Each partition
		for (int partition = 0; partition < maxIRPartitionCount; ++partition) {
			float envelope = irEnvelopes[partition];

			// Each IR
			for (int ir = 0; ir < MAX_IR_COUNT; ++ir) {
				// Use only existing IRs, channels, and partitions
				if (irPartitionCounts[ir] == 0) continue;
				int irChannel = std::min(channel, static_cast<int>(irChannelCounts[ir] - 1));
				if (partition >= irPartitionCounts[ir]) continue;

				// Store weighted sum of IRs in irSpectra to mixedSpectra
				const float* irSrc = (*irSpectra[ir])[irChannel][partition].data();
				float* irMix = mixedSpectra[channel][partition].data();
				float weight = irWeights[channel][ir] * envelope;
				juce::FloatVectorOperations::addWithMultiply(irMix, irSrc, weight, FFT_SIZE);
			}
		}
	}
	// DBG("Mixed spectrum");
}