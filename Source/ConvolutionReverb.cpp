#include "ConvolutionReverb.h"

#include <algorithm>

/* PRIVATE */

void ConvolutionReverb::updateIRFFT(int irIndex) {
	const auto& irBuffer = irBuffers[irIndex];
	const int channelCount = irBuffer.getNumChannels();
	irSpectra[irIndex].resize(channelCount);

	for (int channel = 0; channel < channelCount; ++channel) { // Each channel
		int partitionCount = (irBuffer.getNumSamples() + (FFT_SIZE / 2) - 1) / (FFT_SIZE / 2); // # samples per partition
		irSpectra[irIndex][channel].resize(partitionCount);

		for (int sampleIndex = 0; sampleIndex < irBuffer.getNumSamples(); sampleIndex += (FFT_SIZE / 2)) { // Each partition
			std::fill(partitionBuffer.begin(), partitionBuffer.end(), 0.0f); // Clear buffer
			std::copy_n(irBuffer.getReadPointer(channel) + sampleIndex, // From partition start sample index
				std::min(irBuffer.getNumSamples() - sampleIndex, FFT_SIZE / 2), // Copy P = N/2 samples, rest zero
				partitionBuffer.begin());

			fft.performRealOnlyForwardTransform(partitionBuffer.data()); // Forward FFT in-place

			int partitionIndex = sampleIndex / (FFT_SIZE / 2);
			irSpectra[irIndex][channel][partitionIndex] = partitionBuffer; // Store FFT spectra
		}
	}
	DBG("Computed FFT for buffer " << irIndex);
}

void ConvolutionReverb::mixSpectrum() {
	maxIRPartitionCount = 0;
	for (const auto& ir : irSpectra) if (!ir.empty()) maxIRPartitionCount = std::max(maxIRPartitionCount, static_cast<int>(ir[0].size()));
	mixedSpectra.resize(inputChannels);
	for (int channel = 0; channel < inputChannels; ++channel) { // Each channel
		mixedSpectra[channel].assign(maxIRPartitionCount, std::array<float, FFT_SIZE*2>{});
		for (int partition = 0; partition < maxIRPartitionCount; ++partition) { // Each partition
			for (int ir = 0; ir < MAX_IR_COUNT; ++ir) { // Each IR
				if (irSpectra[ir].empty()) continue;
				int irChannel = std::min(channel, static_cast<int>(irSpectra[ir].size()) - 1); // Use only existing IR channels
				if (partition >= irSpectra[ir][irChannel].size()) continue;

				// Store weighted sum of IRs in irSpectra to mixedSpectra
				const float* irSrc = irSpectra[ir][irChannel][partition].data();
				float* irMix = mixedSpectra[channel][partition].data();
				juce::FloatVectorOperations::addWithMultiply(irMix, irSrc, irWeights[ir], FFT_SIZE * 2);
			}
		}
	}
	DBG("Mixed spectrum");
}

void ConvolutionReverb::processInput(std::array<float, FFT_SIZE*2>& partition) {
	
}

/* PUBLIC */

ConvolutionReverb::ConvolutionReverb(int channels) : irBuffers(), fft(FFT_ORDER), 
	hannWindow(FFT_SIZE, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, true) {
	setInputChannels(channels);
}

void ConvolutionReverb::setInputChannels(int n) {
	inputChannels = n;
	// TODO: Handle syncing
	inputBuffers.assign(n, std::array<float, FFT_SIZE>{0.0f});
	writeIndex.resize(n); spectraIndex.resize(n);
	inputSpectra.resize(n); mixedSpectra.resize(n);
	overlapBuffers.assign(n, std::array<float, FFT_SIZE/2>{0.0f});
	outputQueues.resize(n);
	// Call mixSpectrum safely?
}

void ConvolutionReverb::setIRBuffer(int index, juce::AudioBuffer<float> irBuffer) { 
	irBuffers[index] = irBuffer;
	updateIRFFT(index);
}

void ConvolutionReverb::setUniformWeights() { 
	irWeights.fill(1.0f / MAX_IR_COUNT);
	mixSpectrum();
}

void ConvolutionReverb::setWeights(std::array<float, MAX_IR_COUNT> weights) {
	float sum = std::accumulate(weights.begin(), weights.end(), 0.0f);
	jassert(std::abs(sum - 1.0f) < 1e-6f);
	irWeights = weights;
	mixSpectrum();
}

void ConvolutionReverb::process(juce::AudioBuffer<float>& in) {
	auto pop = [](std::deque<float>& q) { float v = q.front(); q.pop_front(); return v; };
	const int bufSize = in.getNumSamples();
	if (in.getNumChannels() != inputChannels) setInputChannels(in.getNumChannels());

	for (int channel = 0; channel < inputChannels; ++channel) { // Each channel
		const float* start = in.getReadPointer(channel);
		for (int i = 0; i < bufSize; ++i) { // Each sample in input buffer
			inputBuffers[channel][writeIndex[channel]] = *start++; // Write to circular buffer
			writeIndex[channel] = (writeIndex[channel] + 1) % FFT_SIZE;
			if (writeIndex[channel] % HOP_SIZE == 0) { // HOP ready!
				// Copy HOP_SIZE samples to partition buffer
				std::array<float, FFT_SIZE*2> partition{ 0.0f };
				int tailStart = writeIndex[channel];
				std::copy(inputBuffers[channel].begin() + tailStart, inputBuffers[channel].end(), partition.begin());
				std::copy(inputBuffers[channel].begin(), inputBuffers[channel].begin() + tailStart, partition.begin() + FFT_SIZE - tailStart);
				// Analysis windowing
				hannWindow.multiplyWithWindowingTable(partition.data(), FFT_SIZE);
				// Forward FFT in-place
				fft.performRealOnlyForwardTransform(partition.data());
				// Store spectra in circular buffer
				if (inputSpectra[channel].size() != maxIRPartitionCount)
					inputSpectra[channel].assign(maxIRPartitionCount, std::array<float, FFT_SIZE*2>{});
				inputSpectra[channel][spectraIndex[channel]] = partition;
				// Accumulate in frequency domain
				std::array<float, FFT_SIZE*2> accumulator {0};
				for (int partitionIndex = 0; partitionIndex < maxIRPartitionCount; ++partitionIndex) { // Each partition
					int pastIndex = (spectraIndex[channel] - partitionIndex + maxIRPartitionCount) % maxIRPartitionCount;
					auto& pastSpectra = inputSpectra[channel][pastIndex]; // Multiply all past spectra with mixedSpectra
					auto& mixSpectra = mixedSpectra[channel][partitionIndex];
					for (int k = 0; k < 2; ++k) accumulator[k] += pastSpectra[k] * mixSpectra[k]; // DC and Nyquist
					for (int k = 2; k < FFT_SIZE * 2; k += 2) { // Complex multiply Re-Im pairs
						float reX = pastSpectra[k], imX = pastSpectra[k + 1];
						float reH = mixSpectra[k], imH = mixSpectra[k + 1];
						accumulator[k] += reX * reH - imX * imH;
						accumulator[k + 1] += reX * imH + imX * reH;
					}
				} spectraIndex[channel] = (spectraIndex[channel] + 1) % maxIRPartitionCount;
				// IFFT in-place
				fft.performRealOnlyInverseTransform(accumulator.data());
				// juce::FloatVectorOperations::multiply(accumulator.data(), 1.0f, FFT_SIZE); // Scale
				// Overlap-add
				auto& outputFrame = accumulator;
				// hannWindow.multiplyWithWindowingTable(outputFrame.data(), FFT_SIZE);
				juce::FloatVectorOperations::add(outputFrame.data(), overlapBuffers[channel].data(), HOP_SIZE); // Overlap-add saved tail
				std::copy(outputFrame.begin() + HOP_SIZE, outputFrame.begin() + FFT_SIZE, overlapBuffers[channel].begin()); // Copy new tail to buffer
				// Emit HOP_SIZE samples to output queue
				for (int j = 0; j < HOP_SIZE; ++j) outputQueues[channel].push_back(outputFrame[j]);
			}
		} // for (buffer)

		// Write output
		auto* outPtr = in.getWritePointer(channel);
		for (int i = 0; i < bufSize; ++i)
			outPtr[i] = outputQueues[channel].empty() ? 0.0f : pop(outputQueues[channel]);
		
	} // for (channel)
}