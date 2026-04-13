#include "ConvolutionIRBank.h"
#include "Utilities.h"

ConvolutionIRBank::ConvolutionIRBank() : fft(FFT_ORDER) {
	// Preallocate spectra storage
	for (int ir = 0; ir < MAX_IR_COUNT; ++ir) {
		irSpectra[ir] = std::make_shared<SpectraData>();
		for (int channel = 0; channel < N_CHANNELS; ++channel)
			(*irSpectra[ir])[channel].resize(MAX_IR_PARTITIONS);
	}
}

ConvolutionIRBank::ConvolutionIRBank(const ConvolutionIRBank& other) : fft(FFT_ORDER) {
	this->irSpectra = other.irSpectra;
	this->irPartition = other.irPartition;
	this->irPartitionCounts = other.irPartitionCounts;
	this->irChannelCounts = other.irChannelCounts;
	this->maxPartitionCount = other.maxPartitionCount;
}

void ConvolutionIRBank::updateMaxPartitionCount() {
	// Find # partitions in longest IR
	maxPartitionCount = 0;
	for (int ir = 0; ir < MAX_IR_COUNT; ++ir)
		maxPartitionCount = std::max(maxPartitionCount, irPartitionCounts[ir]);

	DBG("Max partition count: " << maxPartitionCount);
}

void ConvolutionIRBank::setIR(int irIndex, const juce::AudioBuffer<float>& irBuffer) {
	if (!validateIRIndex(irIndex)) return;

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
	updateMaxPartitionCount();
	DBG("Computed FFT for buffer " << irIndex);
}

void ConvolutionIRBank::clearIR(int irIndex) {
	irPartitionCounts[irIndex] = 0;
	irChannelCounts[irIndex] = 0;
	updateMaxPartitionCount();
}

const SpectraData& ConvolutionIRBank::getSpectra(int irIndex) const { 
	jassert(validateIRIndex(irIndex)); 
	return *irSpectra[irIndex]; 
}

int ConvolutionIRBank::getMaxPartitionCount() const { return maxPartitionCount; }

int ConvolutionIRBank::getPartitionCount(int irIndex) const {
	jassert(validateIRIndex(irIndex));
	return irPartitionCounts[irIndex];
}

int ConvolutionIRBank::getChannelCount(int irIndex) const {
	jassert(validateIRIndex(irIndex));
	return irChannelCounts[irIndex];
}