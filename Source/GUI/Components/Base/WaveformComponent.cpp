#include "GUI/Components/Base/WaveformComponent.h"

/* PRIVATE */

void WaveformComponent::paint(juce::Graphics& g) {
    if (thumbnail.getNumChannels() == 0) return;
    if (numPoints <= 0) return;

    auto bounds = getLocalBounds().toFloat();
    const float waveformX = bounds.getX() + offsetX;
    const float waveformY = bounds.getCentreY() + offsetY;
    const float waveformWidth = bounds.getRight() - waveformX + offsetWidth;
    const float waveformHeight = bounds.getHeight() * heightScale;
    const double duration = thumbnail.getTotalLength();
    const int numChannels = juce::jmin(thumbnail.getNumChannels(), N_CHANNELS);

    for (int channel = 0; channel < numChannels; ++channel) {
        juce::Path waveform;
        for (int p = 0; p < numPoints; ++p) {
            const double t = (p / static_cast<double>(numPoints - 1)) * duration;
            float minValue, maxValue;
            thumbnail.getApproximateMinMax(t, t + (duration / static_cast<double>(numPoints)), channel, minValue, maxValue);

            float x = waveformX + ((p / static_cast<float>(numPoints - 1)) * waveformWidth);
            float y = waveformY - (maxValue * waveformHeight);
            if (p == 0) waveform.startNewSubPath(x, y);
            else waveform.lineTo(x, y);
        }

        // Mirror
        for (int p = numPoints - 1; p >= 0; --p) {
            const double t = (p / static_cast<double>(numPoints - 1)) * duration;
            float minValue, maxValue;
            thumbnail.getApproximateMinMax(t, t + (duration / static_cast<double>(numPoints)), channel, minValue, maxValue);
            
            float x = waveformX + ((p / static_cast<float>(numPoints - 1)) * waveformWidth);
            float y = waveformY + (maxValue * waveformHeight);
            waveform.lineTo(x, y);
        }

        waveform.closeSubPath();

        float waveformAlpha = juce::jmap(activeAnim.getAlpha(), 0.3f, 1.0f);
        const float channelAlphaOffset = (channel == 0) ? 0.0f : 0.2f;
        waveformAlpha = juce::jmax(0.0f, waveformAlpha - channelAlphaOffset);
        g.setColour(color.withAlpha(waveformAlpha));
        g.fillPath(waveform);
    }
}

/* PUBLIC */

WaveformComponent::WaveformComponent(juce::AnimatorUpdater& updater) : activeAnim(*this, updater, ACTIVE_ANIMATION_TIME_MS) {
    formatManager.registerBasicFormats(); // .wav, .aiff, .flac, .opus, .mp3
    setInterceptsMouseClicks(false, false);
    setBufferedToImage(true);
}

void WaveformComponent::setNumPoints(int nPoints) { 
    numPoints = nPoints;
    repaint();
}

void WaveformComponent::setWaveform(const juce::AudioBuffer<float>* buffer, double sampleRate) {
    thumbnail.clear();
    if (buffer) {
        thumbnail.reset(buffer->getNumChannels(), sampleRate, buffer->getNumSamples());
        thumbnail.addBlock(0, *buffer, 0, buffer->getNumSamples()); }
    repaint();
}

void WaveformComponent::setDimensions(float nOffsetX, float nOffsetY, float nOffsetWidth, float nHeightScale) {
    offsetX = nOffsetX;
    offsetY = nOffsetY;
    offsetWidth = nOffsetWidth;
    heightScale = nHeightScale;
    repaint();
}

void WaveformComponent::setColor(juce::Colour nColor) { 
    color = nColor;
    repaint();
}

void WaveformComponent::setActive(bool nActive, bool animate) { 
    if (nActive) activeAnim.setAlpha(1.0f, animate);
    else activeAnim.setAlpha(0.0f, animate);

    active = nActive;
    repaint();
}