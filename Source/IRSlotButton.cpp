#include "IRSlotButton.h"
#include "Utilities.h"

/* PRIVATE */

void IRSlotButton::paintButton(juce::Graphics& g, bool isMouseOver, bool /*isButtonDown*/) {
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    const auto colour = juce::Colours::gold;
    const bool selected = getToggleState();

    // Background
    g.setColour(selected ? juce::Colours::white.withAlpha(0.1f)
          : (isMouseOver ? juce::Colours::white.withAlpha(0.05f) : juce::Colours::transparentBlack));
    g.fillRoundedRectangle(bounds, 3.0f);

    // Selection ring
    g.setColour(selected ? juce::Colours::white : juce::Colours::darkgrey);
    g.drawRoundedRectangle(bounds, 3.0f, selected ? 1.5f : 0.5f);

    // Indicator dot
    const float indicatorRadius = 5.0f;
    const float indicatorX = bounds.getX() + 6.0f;
    const float indicatorY = bounds.getY() + 6.0f;
    g.setColour(occupied ? colour : colour.withAlpha(0.3f));
    g.fillEllipse(indicatorX, indicatorY, indicatorRadius * 2.0f, indicatorRadius * 2.0f);

    // Waveform preview
    for (int channel = 0; channel < N_CHANNELS; ++channel) {
        if (!waveformPoints[channel].empty()) {
            const float waveformX = bounds.getX() + 8.0f;
            const float waveformY = bounds.getCentreY() + 8.0f;
            const float waveformWidth = bounds.getRight() - waveformX - 8.0f;
            const float waveformHeight = bounds.getHeight() * 0.5f;

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

            const float channelOpacity = (channel == 0) ? 0.1f : 0.2f;
            g.setColour(colour.withAlpha((occupied ? 0.8f : 0.3f) - channelOpacity));
            g.fillPath(waveform);
        }
    }
}

/* PUBLIC */

IRSlotButton::IRSlotButton(int index) : juce::Button("IR " + juce::String(index)), irIndex(index) {}

void IRSlotButton::setWaveform(const juce::AudioBuffer<float>* buffer) {
    for (int channel = 0; channel < N_CHANNELS; ++channel) waveformPoints[channel].clear();

    if (!buffer || buffer->getNumSamples() == 0) {
        occupied = false;
        repaint();
        return;
    }

    // Downsample
    const int numPoints = 100;
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

void IRSlotButton::setOccupied(bool isOccupied) {
    occupied = isOccupied;
    repaint();
}

int IRSlotButton::getIndex() const { return irIndex; }
bool IRSlotButton::isOccupied() const { return occupied; }