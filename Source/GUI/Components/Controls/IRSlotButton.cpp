#include "GUI/GUIUtilities.h"
#include "GUI/Components/Controls/IRSlotButton.h"
#include "GUI/Theme/Theme.h"

/* PRIVATE */

void IRSlotButton::paintButton(juce::Graphics& g, bool /*isMouseOver*/, bool /*isButtonDown*/) {
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    const bool selected = getToggleState();

    const juce::Colour slotColor = Theme::Colors::irSlotColours[irIndex];

    // Background
    const juce::Colour baseColor = selected
        ? Theme::Colors::section.interpolatedWith(slotColor, 0.0621f)
        : Theme::Colors::section.interpolatedWith((Theme::Colors::section).brighter(0.0421f), hoverAnim.getValue());
    g.setColour(baseColor);
    g.fillRoundedRectangle(bounds, 3.0f);

    // Selection ring
    g.setColour(selected ? slotColor.withAlpha(0.821f) : Theme::Colors::outline.withAlpha(0.621f));
    g.drawRoundedRectangle(bounds, 3.0f, selected ? 1.5f : 0.75f);

    // Indicator
    const float indicatorX = bounds.getX() + 6.0f;
    const float indicatorY = bounds.getY() + 6.0f;

    float activeAlpha = indicatorActiveAnim.getValue();
    float indicatorAlpha = juce::jmap(activeAlpha, occupied ? 0.35f : 0.1f, occupied ? 1.0f : 0.2f);

    juce::Image mareIcon;
    {
        juce::SpinLock::ScopedLockType lock(guiState.mareLock);
        mareIcon = guiState.mareImages[irIndex];
    }

    // Poll crossfade state
    const float crossfadeProgress = guiState.crossfadeStates[irIndex].load(std::memory_order_acquire);
    const bool crossfadeActive = guiState.crossfadeActives[irIndex].load(std::memory_order_acquire);

    juce::Image* oldMare = nullptr;
    juce::Image* newMare = nullptr;

    updateIconCrossfade(
        incomingIcon, outgoingIcon, stagedIcon, 
        mareIcon,
        oldMare, newMare,
        crossfadeActive, crossfadeWasActive, indicatorStyle
    );

    Paint::irIndicator(g, CartesianCoordinate{indicatorX, indicatorY}, 
        indicatorRadius,
        irIndex, occupied, active, false, 
        indicatorAlpha, 
        -1.0f, 
        juce::Colours::transparentBlack, 
        0, 
        1.0f,
        oldMare, 
        newMare,
        crossfadeActive ? crossfadeProgress : 1.0f
    );

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
    waveformPreview.setNumPoints(WAVEFORM_PREVIEW_POINTS);
    waveformPreview.setColor(Theme::Colors::irSlotColours[index]);
    waveformPreview.setActive(active);
    waveformPreview.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(waveformPreview, 0);

    dragHandler.typeAccepted = DragAndDropHandler::DragAndDropType::FILES;
    dragHandler.acceptsMultiple = false;

    setBufferedToImage(true);
    setTooltip("Select an IR slot by clicking on it. Clicking the indicator in the top left (or double-clicking a slot) will enable/disable the IR.\n\nCan accept audio files via drag-and-drop.");
}

void IRSlotButton::setOccupied(bool nOccupied) {
    occupied = nOccupied;
    repaint();
}

void IRSlotButton::setActive(bool nActive, bool animate) {
    indicatorActiveAnim.setValue(nActive ? 1.0f : 0.0f, animate);
    active = nActive;
    waveformPreview.setActive(nActive, animate);
    repaint();
}

void IRSlotButton::setIndicatorStyle(const juce::String nStyle) {
    indicatorStyle = nStyle;
    updateIndicator();
}

void IRSlotButton::updateIndicator() {
    juce::Image mareIcon;
    {
        juce::SpinLock::ScopedLockType lock(guiState.mareLock);
        mareIcon = guiState.mareImages[irIndex];
    }

    updateFieldIndicatorStyle(outgoingIcon, mareIcon, indicatorStyle);
    repaint();
}

int IRSlotButton::getIndex() const { return irIndex; }

WaveformComponent* IRSlotButton::getWaveformComponent() { return &waveformPreview; }