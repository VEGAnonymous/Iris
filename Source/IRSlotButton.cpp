#include "IRSlotButton.h"
#include "GUIUtilities.h"
#include "Theme.h"

/* PRIVATE */

void IRSlotButton::paintButton(juce::Graphics& g, bool isMouseOver, bool /*isButtonDown*/) {
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    // const auto& color = IR_SLOT_COLORS[irIndex];
    // const auto& color = Theme::Colors::main;

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
}

void IRSlotButton::resized() {
    auto bounds = getLocalBounds();
    waveformPreview.setBounds(bounds);
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

IRSlotButton::IRSlotButton(int index) : juce::Button("IR " + juce::String(index)), irIndex(index) {
    waveformPreview.setDimensions(8.0f, 8.0f, -8.0f, 0.4f);
    waveformPreview.setColor(Theme::Colors::highlight);
    waveformPreview.setActive(active);
    waveformPreview.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(waveformPreview, 0);
}

void IRSlotButton::setWaveform(const juce::AudioBuffer<float>* buffer, double sampleRate) {
    if (!buffer || buffer->getNumSamples() == 0) occupied = false;
    waveformPreview.setNumPoints(WAVEFORM_PREVIEW_POINTS);
    waveformPreview.setWaveform(buffer, sampleRate);
    repaint();
}

void IRSlotButton::setOccupied(bool nOccupied) {
    occupied = nOccupied;
    repaint();
}

void IRSlotButton::setActive(bool nActive) {
    active = nActive;
    waveformPreview.setActive(nActive);
    repaint();
}

int IRSlotButton::getIndex() const { return irIndex; }