#include "GUI/GUIUtilities.h"
#include "GUI/Components/Controls/IRSlotButton.h"
#include "GUI/Theme/Theme.h"

/* PRIVATE */

void IRSlotButton::paintButton(juce::Graphics& g, bool /*isMouseOver*/, bool /*isButtonDown*/) {
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    const bool selected = getToggleState();

    // Background
    g.setColour(selected ? Theme::Colors::background.brighter(0.126f)
          : Theme::Colors::background.brighter(0.0612f).interpolatedWith(juce::Colours::transparentBlack, hoverAnim.getValue()));
    g.fillRoundedRectangle(bounds, 3.0f);

    // Selection ring
    g.setColour(selected ? Theme::Colors::textLight : juce::Colours::darkgrey);
    g.drawRoundedRectangle(bounds, 3.0f, selected ? 2.0f : 1.0f);

    // Indicator dot
    const float indicatorX = bounds.getX() + 6.0f;
    const float indicatorY = bounds.getY() + 6.0f;

    float activeAlpha = indicatorActiveAnim.getValue();
    float indicatorAlpha = juce::jmap(activeAlpha, occupied ? 0.35f : 0.1f, occupied ? 1.0f : 0.2f);

    auto mareIndicator = &guiState.mareImages[irIndex];
    Paint::irIndicator(g, CartesianCoordinate{indicatorX, indicatorY}, indicatorRadius - (!mareIndicator->isNull() ? 2.0f : 0.0f),
        irIndex, occupied, active, false, indicatorAlpha, -1.0f, juce::Colours::transparentBlack, mareIndicator);

    // Drag and drop indication
    const float dragAlpha = dragHover.getValue();
    Paint::dragAndDropHover(g, getLocalBounds().toFloat(), dragAlpha, (Theme::Colors::highlight).withAlpha(0.5f));
}

void IRSlotButton::resized() {
    auto bounds = getLocalBounds();
    waveformPreview.setBounds(bounds);
}

void IRSlotButton::mouseEnter(const juce::MouseEvent& /*e*/) { hoverAnim.setValue(1.0f); }
void IRSlotButton::mouseExit(const juce::MouseEvent& /*e*/) { hoverAnim.setValue(0.0f); }
void IRSlotButton::mouseDown(const juce::MouseEvent& e) {
    // Check if clicked on indicator
    auto indicatorBounds = getIndicatorBounds(getLocalBounds(), indicatorRadius + 1.0f);
    if (indicatorBounds.contains(e.position) || e.getNumberOfClicks() >= 2)
        if (onActiveToggle) onActiveToggle(!active);
    else juce::Button::mouseDown(e);
}

BoundsF IRSlotButton::getIndicatorBounds(Bounds bounds, const float radius) const {
    auto b = bounds.toFloat().reduced(2.0f);
    // HACK SHIT ALERT
    return { b.getX() + 5.5f - radius, b.getY() + 5.5f - radius, radius * 2.0f, radius * 2.0f };
}

/* PUBLIC */

IRSlotButton::IRSlotButton(int index, juce::AnimatorUpdater& updater, GUIState& gState) : 
    guiState(gState), hoverAnim(*this, updater), indicatorActiveAnim(*this, updater, ACTIVE_ANIMATION_TIME_MS),
    dragHover(*this, updater), waveformPreview(updater),
    juce::Button("IR " + juce::String(index)), irIndex(index) {
    waveformPreview.setDimensions(8.0f, 8.0f, -8.0f, 0.25f);
    waveformPreview.setColor(Theme::Colors::irSlotColours[index]);
    waveformPreview.setActive(active);
    waveformPreview.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(waveformPreview, 0);

    dragHandler.typeAccepted = DragAndDropHandler::DragAndDropType::FILES;
    dragHandler.acceptsMultiple = false;

    setBufferedToImage(true);
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
    indicatorActiveAnim.setValue(nActive ? 1.0f : 0.0f);
    active = nActive;
    waveformPreview.setActive(nActive, true);
    repaint();
}

int IRSlotButton::getIndex() const { return irIndex; }