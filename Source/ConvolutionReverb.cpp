#include "ConvolutionReverb.h"

#include <algorithm>

/* PRIVATE */

void ConvolutionReverb::updateMaxIRPartitionCount() {
	// Find # partitions in longest IR
	maxIRPartitionCount = 0;
	for (const auto& ir : irSpectra) if (!ir.empty()) maxIRPartitionCount = std::max(maxIRPartitionCount, static_cast<int>(ir[0].size()));

	for (int channel = 0; channel < inputChannels; ++channel) mixedSpectra[channel].resize(maxIRPartitionCount);
	irEnvelopes.resize(maxIRPartitionCount);
	// DBG("Max partition count: " << maxIRPartitionCount);
}

void ConvolutionReverb::updateIRFFT(int irIndex) {
	const auto& irBuffer = irBuffers[irIndex];
	const int channelCount = irBuffer.getNumChannels();
	irSpectra[irIndex].resize(channelCount);

	for (int channel = 0; channel < channelCount; ++channel) { // Each channel
		int partitionCount = (irBuffer.getNumSamples() + PARTITION_SIZE - 1) / PARTITION_SIZE; // # partitions
		irSpectra[irIndex][channel].resize(partitionCount);

		for (int sampleIndex = 0; sampleIndex < irBuffer.getNumSamples(); sampleIndex += PARTITION_SIZE) { // Each partition
			std::fill(irPartition.begin(), irPartition.end(), 0.0f); // Clear buffer
			juce::FloatVectorOperations::copy(irPartition.data(),
				irBuffer.getReadPointer(channel) + sampleIndex,
				std::min(irBuffer.getNumSamples() - sampleIndex, PARTITION_SIZE)); // Copy 2L samples, rest zero

			fft.performRealOnlyForwardTransform(irPartition.data()); // Forward FFT in-place

			int partitionIndex = sampleIndex / PARTITION_SIZE;
			juce::FloatVectorOperations::copy(irSpectra[irIndex][channel][partitionIndex].data(), irPartition.data(), FFT_SIZE); // Store FFT spectra
		}
	}
	updateMaxIRPartitionCount();
	// DBG("Computed FFT for buffer " << irIndex);
}

void ConvolutionReverb::mixSpectrum() {
	for (int channel = 0; channel < inputChannels; ++channel) { // Each channel
		juce::FloatVectorOperations::clear(mixedSpectra[channel].data()->data(), maxIRPartitionCount * FFT_SIZE);
		
		for (int partition = 0; partition < maxIRPartitionCount; ++partition) { // Each partition
			float envelope = irEnvelopes[partition];

			for (int ir = 0; ir < MAX_IR_COUNT; ++ir) { // Each IR
				if (irSpectra[ir].empty()) continue;
				int irChannel = std::min(channel, static_cast<int>(irSpectra[ir].size()) - 1); // Use only existing IR channels
				if (partition >= irSpectra[ir][irChannel].size()) continue;

				// Store weighted sum of IRs in irSpectra to mixedSpectra
				const float* irSrc = irSpectra[ir][irChannel][partition].data();
				float* irMix = mixedSpectra[channel][partition].data();
				float weight = irWeights[channel][ir] * envelope;
				juce::FloatVectorOperations::addWithMultiply(irMix, irSrc, weight, FFT_SIZE);
			}
		}
	}
	// DBG("Mixed spectrum");
}

void ConvolutionReverb::accumulateSpectra(int channel) {
	// Store spectra in circular buffer
	int historySize = 2 * maxIRPartitionCount;
	if (inputSpectra[channel].size() != historySize) inputSpectra[channel].assign(historySize, std::array<float, FFT_SIZE>{0.0f});
	juce::FloatVectorOperations::copy(inputSpectra[channel][spectraIndex[channel]].data(), inputPartitions[channel].data(), FFT_SIZE);

	// Accumulate in frequency domain
	auto& accumulator = accumulators[channel];
	std::fill(accumulator.begin(), accumulator.end(), 0.0f);

	for (int partitionIndex = 0; partitionIndex < maxIRPartitionCount; ++partitionIndex) { // Each partition
		// Multiply *every second* past input spectrum with mixedSpectra
		int pastIndex = spectraIndex[channel] - 2 * partitionIndex;
		if (pastIndex < 0) pastIndex += historySize * ((-pastIndex / historySize) + 1);
		pastIndex %= historySize;

		auto& pastSpectra = inputSpectra[channel][pastIndex];
		auto& mixSpectra = mixedSpectra[channel][partitionIndex];

		// Complex multiply Re-Im pairs
		// TODO: Optimize with SIMD
		for (int k = 0; k < 2; ++k) accumulator[k] += pastSpectra[k] * mixSpectra[k]; // DC & Nyquist
		for (int k = 2; k < FFT_SIZE; k += 2) {
			float reX = pastSpectra[k], imX = pastSpectra[k + 1];
			float reH = mixSpectra[k], imH = mixSpectra[k + 1];
			accumulator[k] += (reX * reH) - (imX * imH);
			accumulator[k + 1] += (reX * imH) + (imX * reH);
		}
	} spectraIndex[channel] = (spectraIndex[channel] + 1) % historySize;
}

void ConvolutionReverb::overlapAdd(int channel) {
	auto& outputFrame = accumulators[channel];
	// Paritition overlap
	// OLA IFFT block (4L samples) with previous IFFT block -> produces intermediate block (2L samples) per hop (every L samples)
	juce::FloatVectorOperations::add(outputFrame.data(), overlapBuffersA[channel][convSaveCount].data(), PARTITION_SIZE);
	juce::FloatVectorOperations::copy(overlapBuffersA[channel][convSaveCount].data(), outputFrame.data() + PARTITION_SIZE, PARTITION_SIZE);
	// Hop overlap
	// OLA intermediate block with previous intermediate block -> produces L samples for output
	juce::FloatVectorOperations::add(outputFrame.data(), overlapBuffersB[channel].data(), HOP_SIZE);
	juce::FloatVectorOperations::copy(overlapBuffersB[channel].data(), outputFrame.data() + HOP_SIZE, PARTITION_SIZE - HOP_SIZE);

	// Emit the final HOP_SIZE = L samples to output
	int& writeIndex = outputWriteIndex[channel];
	for (int j = 0; j < HOP_SIZE; ++j) {
		outputBuffers[channel][writeIndex] = outputFrame[j];
		writeIndex = (writeIndex + 1) & (outputSize - 1);
	}
}

void ConvolutionReverb::processHop(int channel) { /* TVOLAP fast convolution */
	// Copy 2L samples to partition buffer
	auto& partition = inputPartitions[channel];
	std::fill(partition.begin(), partition.end(), 0.0f);

	int tailStart = inputWriteIndex[channel], headStart = PARTITION_SIZE - tailStart;
	juce::FloatVectorOperations::copy(partition.data(), inputBuffers[channel].data() + tailStart, headStart);
	juce::FloatVectorOperations::copy(partition.data() + headStart, inputBuffers[channel].data(), tailStart);

	// Analysis windowing
	hannWindow.multiplyWithWindowingTable(partition.data(), PARTITION_SIZE);

	// Forward FFT in-place
	fft.performRealOnlyForwardTransform(partition.data());

	// Accumulate convolution
	accumulateSpectra(channel);

	// IFFT in-place
	fft.performRealOnlyInverseTransform(accumulators[channel].data());
	juce::FloatVectorOperations::multiply(accumulators[channel].data(), 0.15f, FFT_SIZE); // Scale // HACK: Stop hardcoding this

	// Overlap-add and output
	overlapAdd(channel);

	if (channel == 0) hopCounter++;
}

/* PUBLIC */

ConvolutionReverb::ConvolutionReverb(int channels) : irBuffers(), fft(FFT_ORDER), 
	hannWindow(PARTITION_SIZE, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, false) {
	setInputChannels(channels);
}

void ConvolutionReverb::setInputChannels(int n) {
	inputChannels = n;

	irWeights.assign(n, std::array<float, MAX_IR_COUNT> {0.0f});

	inputBuffers.assign(n, std::array<float, PARTITION_SIZE> {0.0f});
	inputWriteIndex.resize(n);

	inputPartitions.assign(n, {});

	spectraIndex.resize(n);
	inputSpectra.resize(n); 
	mixedSpectra.resize(n);

	accumulators.assign(inputChannels, {});
	overlapBuffersA.assign(n, {std::array<float, PARTITION_SIZE>{0.0f}, std::array<float, PARTITION_SIZE>{0.0f} });
	overlapBuffersB.assign(n, std::array<float, HOP_SIZE> {0.0f});
	convSaveCount = 0;

	outputBuffers.assign(inputChannels, std::vector<float>(outputSize, 0.0f));
	outputWriteIndex.resize(n); 
	outputReadIndex.resize(n);
}

void ConvolutionReverb::setIRBuffer(int index, juce::AudioBuffer<float> irBuffer) { 
	irBuffers[index] = irBuffer;
	updateIRFFT(index);
}

void ConvolutionReverb::setUniformWeights() { 
	for (int channel = 0; channel < irWeights.size(); ++channel) irWeights[channel].fill(1.0f / MAX_IR_COUNT);
	mixSpectrum();
}

void ConvolutionReverb::setWeights(std::vector<std::array<float, MAX_IR_COUNT>> weights) {
	irWeights = weights;
	mixSpectrum();
}

void ConvolutionReverb::setDecay(float decay) {
	const float minDB = -180.0f, maxDB = -30.0f;
	float totalDB = minDB + (decay * (maxDB - minDB));

	float alpha = totalDB / static_cast<float>(maxIRPartitionCount); // dB
	float envelope = powf(10.0f, alpha / 20.0f); // Linear
	for (int partition = 0; partition < maxIRPartitionCount; ++partition)
		irEnvelopes[partition] = powf(envelope, static_cast<float>(partition));
}

void ConvolutionReverb::process(juce::AudioBuffer<float>& in) {
	const int bufSize = in.getNumSamples();
	if (in.getNumChannels() != inputChannels) setInputChannels(in.getNumChannels());

	for (int channel = 0; channel < inputChannels; ++channel) { // Each channel
		const float* start = in.getReadPointer(channel);
		for (int i = 0; i < bufSize; ++i) { // Each sample in input buffer
			inputBuffers[channel][inputWriteIndex[channel]] = *start++; // Write to circular buffer
			inputWriteIndex[channel] = (inputWriteIndex[channel] + 1) % PARTITION_SIZE;
			if (inputWriteIndex[channel] % HOP_SIZE == 0) processHop(channel); // Process a hop every L samples
		}

		// Write output
		auto* outPtr = in.getWritePointer(channel);
		int& writeIndex = outputWriteIndex[channel];
		int& readIndex = outputReadIndex[channel];
		for (int i = 0; i < bufSize; ++i)
			if (readIndex != writeIndex) {
				outPtr[i] = outputBuffers[channel][readIndex];
				readIndex = (readIndex + 1) & (outputSize - 1);
			} else outPtr[i] = 0.0f;

	} // for (channel)

	// Flip OLA state
	convSaveCount = (convSaveCount + hopCounter) % 2;
	hopCounter = 0;
}