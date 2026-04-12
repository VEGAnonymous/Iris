#include "ConvolutionReverb.h"
#include "Utilities.h"

#include <algorithm>

/* PRIVATE */

void ConvolutionReverb::updateMaxIRPartitionCount() {
	// Find # partitions in longest IR
	maxIRPartitionCount = 0;
	for (int ir = 0; ir < MAX_IR_COUNT; ++ir)
		maxIRPartitionCount = std::max(maxIRPartitionCount, irPartitionCounts[ir]);

	DBG("Updated max IR partition count");

	// DBG("Max partition count: " << maxIRPartitionCount);
}

void ConvolutionReverb::updateIRFFT(int irIndex) {
	if (!validateIRIndex(irIndex)) return;
	DBG("Updating IRFFT: " << irIndex);

	const auto& irBuffer = irBuffers[irIndex];
	const int channelCount = std::min(irBuffer.getNumChannels(), N_CHANNELS);
	const int partitionCount = (irBuffer.getNumSamples() + PARTITION_SIZE - 1) / PARTITION_SIZE;

	irChannelCounts[irIndex] = channelCount;
	irPartitionCounts[irIndex] = partitionCount;

	// Each channel
	for (int channel = 0; channel < channelCount; ++channel) {

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
			auto& spectraDest = irSpectra[irIndex][channel][partition];
			juce::FloatVectorOperations::copy(spectraDest.data(), irPartition.data(), FFT_SIZE);
		}
	}
	updateMaxIRPartitionCount();
	DBG("Computed FFT for buffer " << irIndex);
}

void ConvolutionReverb::mixSpectrum() {
	juce::ScopedReadLock lock(irLock);

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
	// Store input spectra in circular buffer
	const int historySize = 2 * maxIRPartitionCount;

	auto& spectraDest = inputSpectra[channel][spectraIndex[channel]];
	juce::FloatVectorOperations::copy(spectraDest.data(), inputPartitions[channel].data(), FFT_SIZE);

	// Accumulate in frequency domain
	auto& accumulator = accumulators[channel];
	std::fill(accumulator.begin(), accumulator.end(), 0.0f);

	// Each partition
	for (int partition = 0; partition < maxIRPartitionCount; ++partition) {
		// Multiply every *second* past input spectrum with mixedSpectra
		int pastIndex = spectraIndex[channel] - (2 * partition);
		pastIndex = wrapIndex(pastIndex, historySize);

		auto& pastSpectra = inputSpectra[channel][pastIndex];
		auto& mixSpectra = mixedSpectra[channel][partition];

		// Complex multiply Re-Im pairs
		// TODO: Optimize with SIMD
		for (int k = 0; k < 2; ++k) accumulator[k] += pastSpectra[k] * mixSpectra[k]; // DC & Nyquist
		for (int k = 2; k < FFT_SIZE; k += 2) {
			float reX = pastSpectra[k], imX = pastSpectra[k + 1];
			float reH = mixSpectra[k], imH = mixSpectra[k + 1];
			accumulator[k] += (reX * reH) - (imX * imH);
			accumulator[k + 1] += (reX * imH) + (imX * reH);
		}
	} 
	spectraIndex[channel] = (spectraIndex[channel] + 1) % historySize;
}

void ConvolutionReverb::overlapAdd(int channel) {
	auto& outputFrame = accumulators[channel];
	/* Partition overlap */
	// OLA IFFT block (4L samples) with previous IFFT block -> produces intermediate block (2L samples) per hop (every L samples)
	juce::FloatVectorOperations::add(outputFrame.data(), 
									 overlapBuffersA[channel][convSaveCount].data(), 
									 PARTITION_SIZE); // 2L

	// Save upper half
	juce::FloatVectorOperations::copy(overlapBuffersA[channel][convSaveCount].data(), 
									  outputFrame.data() + PARTITION_SIZE,
									  PARTITION_SIZE);
	
	/* Hop overlap */
	// OLA intermediate block with previous intermediate block -> produces L samples for output
	juce::FloatVectorOperations::add(outputFrame.data(), 
									 overlapBuffersB[channel].data(), 
									 HOP_SIZE); // L

	// Save tail
	juce::FloatVectorOperations::copy(overlapBuffersB[channel].data(), 
									  outputFrame.data() + HOP_SIZE, 
									  PARTITION_SIZE - HOP_SIZE);

	// Emit finalized HOP_SIZE (L) samples to output
	int& writeIndex = outputWriteIndex[channel];
	for (int i = 0; i < HOP_SIZE; ++i) {
		outputBuffers[channel][writeIndex] = outputFrame[i];
		writeIndex = (writeIndex + 1) & (OUTPUT_SIZE - 1);
	}
}

void ConvolutionReverb::processHop(int channel) { /* TVOLAP fast convolution */
	// DBG("Processing a hop");
	juce::ScopedReadLock lock(irLock);

	// Passthrough case (no IRs loaded)
	if (maxIRPartitionCount == 0) {
		int& writeIndex = outputWriteIndex[channel];
		for (int j = 0; j < HOP_SIZE; ++j) {
			outputBuffers[channel][writeIndex] = 0.0f;
			writeIndex = (writeIndex + 1) & (OUTPUT_SIZE - 1);
		} if (channel == 0) hopCounter++;
		return;
	}

	// Copy 2L samples from input buffer to partition buffer
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

	// DBG("Accumulated spectra");

	// IFFT in-place
	fft.performRealOnlyInverseTransform(accumulators[channel].data());

	// Scale
	constexpr float ifftScale = 0.15f;
	juce::FloatVectorOperations::multiply(accumulators[channel].data(), ifftScale, FFT_SIZE);

	// Overlap-add and output
	overlapAdd(channel);

	if (channel == 0) hopCounter++;
}

/* PUBLIC */

ConvolutionReverb::ConvolutionReverb() : irBuffers(), fft(FFT_ORDER), 
	hannWindow(PARTITION_SIZE, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, false) {

	// Preallocate spectra storage
	for (int ir = 0; ir < MAX_IR_COUNT; ++ir)
		for (int ch = 0; ch < N_CHANNELS; ++ch)
			irSpectra[ir][ch].resize(MAX_IR_PARTITIONS);

	for (int ch = 0; ch < N_CHANNELS; ++ch) {
		mixedSpectra[ch].resize(MAX_IR_PARTITIONS);
		inputSpectra[ch].resize(2 * MAX_IR_PARTITIONS);
	}

	irEnvelopes.resize(MAX_IR_PARTITIONS);
}

int ConvolutionReverb::wrapIndex(int index, int size) {
	if (index < 0) index += size * ((-index / size) + 1);
	return index % size;
}

void ConvolutionReverb::setIRBuffer(int index, juce::AudioBuffer<float> irBuffer) {
	if (validateIRIndex(index)) {
		irBuffers[index] = irBuffer;
		juce::ScopedWriteLock lock(irLock);
		updateIRFFT(index);
	}
}

void ConvolutionReverb::clearIRBuffer(int index) {
	if (validateIRIndex(index)) {
		DBG("Clearing internal IR spectra: " << index);
		juce::ScopedWriteLock lock(irLock);
		irPartitionCounts[index] = 0;
		irChannelCounts[index] = 0;
		updateMaxIRPartitionCount();
	}
}

void ConvolutionReverb::setWeights(std::array<std::array<float, MAX_IR_COUNT>, N_CHANNELS> weights) {
	irWeights = weights;
	mixSpectrum();
}

void ConvolutionReverb::setDecay(float decay) {
	decay = juce::jlimit(0.0f, 1.0f, decay);
	const float minDB = -180.0f, maxDB = -30.0f;
	float totalDB = minDB + (decay * (maxDB - minDB));

	float alpha = totalDB / static_cast<float>(maxIRPartitionCount); // dB
	float envelope = powf(10.0f, alpha / 20.0f); // Linear
	for (int partition = 0; partition < maxIRPartitionCount; ++partition)
		irEnvelopes[partition] = powf(envelope, static_cast<float>(partition));
}

void ConvolutionReverb::process(juce::AudioBuffer<float>& in) {
	const int blockSize = in.getNumSamples();

	// Each channel
	for (int channel = 0; channel < N_CHANNELS; ++channel) { 
		const float* start = in.getReadPointer(channel);

		// Each sample in input block
		for (int i = 0; i < blockSize; ++i) { 
			// Write to input buffer
			inputBuffers[channel][inputWriteIndex[channel]] = *start++;
			inputWriteIndex[channel] = (inputWriteIndex[channel] + 1) % PARTITION_SIZE;

			// Process a hop every L samples
			if (inputWriteIndex[channel] % HOP_SIZE == 0) processHop(channel); 
		}

		// Write output
		auto* outPtr = in.getWritePointer(channel);
		int& writeIndex = outputWriteIndex[channel];
		int& readIndex = outputReadIndex[channel];
		for (int i = 0; i < blockSize; ++i)
			if (readIndex != writeIndex) {
				outPtr[i] = outputBuffers[channel][readIndex];
				readIndex = (readIndex + 1) & (OUTPUT_SIZE - 1);
			} else outPtr[i] = 0.0f;

	}

	// Flip OLA state
	convSaveCount = (convSaveCount + hopCounter) % 2;
	hopCounter = 0;
}