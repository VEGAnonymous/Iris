#include "WaveformComponent.h"

/* PRIVATE */

void WaveformComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    for (int channel = 0; channel < N_CHANNELS; ++channel) {
        if (!waveformPoints[channel].empty()) {
            const float waveformX = bounds.getX() + offsetX;
            const float waveformY = bounds.getCentreY() + offsetY;
            const float waveformWidth = bounds.getRight() - waveformX + offsetWidth;
            const float waveformHeight = bounds.getHeight() * heightScale;

            juce::Path waveform;
            const int numPoints = static_cast<int>(waveformPoints[channel].size());
            for (int p = 0; p < numPoints; ++p) {
                float x = waveformX + ((p / static_cast<float>(numPoints - 1)) * waveformWidth);
                float y = waveformY - (waveformPoints[channel][p] * waveformHeight);
                if (p == 0) waveform.startNewSubPath(x, y);
                else waveform.lineTo(x, y);
            }

            // Mirror
            for (int p = numPoints - 1; p >= 0; --p) {
                float x = waveformX + ((p / static_cast<float>(numPoints - 1)) * waveformWidth);
                float y = waveformY + (waveformPoints[channel][p] * waveformHeight);
                waveform.lineTo(x, y);
            }
            waveform.closeSubPath();

            const float channelOpacity = (channel == 0) ? 0.0f : 0.2f;
            g.setColour(color.withAlpha((active ? 1.0f : 0.25f) - channelOpacity));
            g.fillPath(waveform);
        }
    }
}

/* PUBLIC */

WaveformComponent::WaveformComponent() {}

void WaveformComponent::setWaveform(const juce::AudioBuffer<float>* buffer, const int numPoints) {
    for (int channel = 0; channel < N_CHANNELS; ++channel) waveformPoints[channel].clear();

    // Downsample
    const int numSamples = buffer->getNumSamples();
    for (int channel = 0; channel < buffer->getNumChannels(); ++channel) {
        if (channel >= N_CHANNELS) break;

        const float* data = buffer->getReadPointer(channel);
        waveformPoints[channel].resize(numPoints);

        for (int p = 0; p < numPoints; ++p) {
            // Search each downsampled "point" region for peak sample
            int pointStart = (p * numSamples) / numPoints;
            int pointEnd = ((p + 1) * numSamples) / numPoints;
            float peak = 0.0f;
            for (int s = pointStart; s < pointEnd; ++s)
                peak = std::max(peak, std::abs(data[s]));

            waveformPoints[channel][p] = peak;
        }
    }

    repaint();
}

void WaveformComponent::setDimensions(float nOffsetX, float nOffsetY, float nOffsetWidth, float nHeightScale) {
    offsetX = nOffsetX; // Offset
    offsetY = nOffsetY; // Offset
    offsetWidth = nOffsetWidth; // Offset
    heightScale = nHeightScale; // Scale
    repaint();
}

void WaveformComponent::setColor(juce::Colour nColor) { 
    color = nColor;
    repaint();
}

void WaveformComponent::setActive(bool nActive) { 
    active = nActive;
    repaint(); 
}