#include "IRSlotButton.h"
#include "GUIUtilities.h"

/* PRIVATE */

void IRSlotButton::paintButton(juce::Graphics& g, bool isMouseOver, bool /*isButtonDown*/) {
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    const auto& colour = IR_SLOT_COLORS[irIndex];
    const bool selected = getToggleState();

    // Background
    g.setColour(selected ? juce::Colours::white.withAlpha(0.1f)
          : (isMouseOver ? juce::Colours::white.withAlpha(0.05f) : juce::Colours::transparentBlack));
    g.fillRoundedRectangle(bounds, 3.0f);

    // Selection ring
    g.setColour(selected ? juce::Colours::white : juce::Colours::darkgrey);
    g.drawRoundedRectangle(bounds, 3.0f, selected ? 1.5f : 0.5f);

    // Indicator dot
    const float indicatorX = bounds.getX() + 6.0f;
    const float indicatorY = bounds.getY() + 6.0f;
    const float indicatorAlpha = occupied ? (active ? 1.0f : 0.35f) : (active ? 0.2f : 0.1f);
    Paint::irIndicator(g, CartesianCoordinate{indicatorX, indicatorY}, indicatorRadius, irIndex, occupied, active, indicatorAlpha);

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

            const float channelOpacity = (channel == 0) ? 0.0f : 0.2f;
            g.setColour(colour.withAlpha(getIRAlpha(occupied, active) - channelOpacity));
            g.fillPath(waveform);
        }
    }
}

void IRSlotButton::mouseDown(const juce::MouseEvent& e) {
    // Check if clicked on indicator
    auto indicatorBounds = getIndicatorBounds(getLocalBounds(), indicatorRadius + 1.0f);
    if (indicatorBounds.contains(e.position) || e.getNumberOfClicks() >= 2) {
        active = !active;
        DBG("Slot " << irIndex << " indicator clicked; setting state " << static_cast<int>(active));
        if (onActiveToggle) onActiveToggle(active);
        repaint();
    }
    else juce::Button::mouseDown(e);
}

BoundsF IRSlotButton::getIndicatorBounds(Bounds bounds, const float radius) const {
    auto b = bounds.toFloat().reduced(2.0f);
    // HACK SHIT ALERT
    return { b.getX() + 5.5f - radius, b.getY() + 5.5f - radius, radius * 2.0f, radius * 2.0f };
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

void IRSlotButton::setOccupied(bool nOccupied) {
    occupied = nOccupied;
    repaint();
}

void IRSlotButton::setActive(bool nActive) {
    active = nActive;
    repaint();
}

int IRSlotButton::getIndex() const { return irIndex; }