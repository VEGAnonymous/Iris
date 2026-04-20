#include "IRHeaderComponent.h"
#include "GUIUtilities.h"

/* PRIVATE */

BoundsF IRHeaderComponent::getIndicatorBounds(Bounds bounds, const float radius) const {
    auto b = bounds.toFloat().reduced(2.0f);
    // HACK SHIT ALERT
    return { b.getX() + 12.0f - radius, b.getCentreY() - 0.5f - radius, radius * 2.0f, radius * 2.0f};
}

void IRHeaderComponent::mouseDown(const juce::MouseEvent& e) {
    auto indicatorBounds = getIndicatorBounds(getLocalBounds(), indicatorRadius);
    if (indicatorBounds.contains(e.position)) 
        if (onActiveToggle) onActiveToggle(!currentIR.active);
}

/* PUBLIC */

IRHeaderComponent::IRHeaderComponent(juce::AnimatorUpdater& updater) 
    : indicatorActiveAnim(*this, updater, true, ACTIVE_ANIMATION_TIME_MS) {
    addAndMakeVisible(clearButton);
    clearButton.onClick = [this]() { if (onClear) onClear(); };
    setBufferedToImage(true);
}

void IRHeaderComponent::setSlot(int irIndex, const IRSlot& slot) {
    currentIndex = irIndex;
    currentIR = slot;
    currentPath = slot.occupied ? formatPath(slot.file.getFileName(), 42, Ellipsis::MIDDLE) : "-";
    repaint();
}

void IRHeaderComponent::setActive(bool nActive, bool animate) {
    if (currentIR.active != nActive && animate) {
        if (nActive) indicatorActiveAnim.animateIn();
        else indicatorActiveAnim.animateOut();
    } else indicatorActiveAnim.setAnimateAlpha(nActive ? 1.0f : 0.0f);

    currentIR.active = nActive;
    repaint();
}

void IRHeaderComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();

    // Indicator
    const float indicatorX = bounds.getX() + 14.0f;
    const float indicatorY = bounds.getCentreY();

    float activeAlpha = indicatorActiveAnim.getAnimateAlpha();
    float indicatorAlpha = juce::jmap(activeAlpha, currentIR.occupied ? 0.35f : 0.1f, currentIR.occupied ? 1.0f : 0.2f);

    Paint::irIndicator(g, {indicatorX, indicatorY}, indicatorRadius, currentIndex, currentIR.occupied, currentIR.active, indicatorAlpha);

    // IR #
    const float labelX = (indicatorX + indicatorRadius) + 12.0f;
    const float labelWidth = 36.0f;
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions{14.0f, juce::Font::bold}));
    g.drawText("IR " + juce::String(currentIndex), 
        BoundsF(labelX, 0, labelWidth, bounds.getHeight()), juce::Justification::centredLeft);

    // Filepath
    const float pathX = (labelX + labelWidth) + 16.0f;
    const float pathWidth = (bounds.getWidth() - pathX) - (clearButton.getWidth() + 32.0f);
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(juce::FontOptions{12.0f, juce::Font::bold}));
    g.drawFittedText(currentPath, 
        BoundsF(pathX, 0, pathWidth, bounds.getHeight()).toNearestInt(), juce::Justification::centredLeft, 1, 1.0f);
}

void IRHeaderComponent::resized() {
    const int clearButtonWidth = 54;
    clearButton.setBounds(getWidth() - clearButtonWidth - 8, (getHeight() - 22) / 2, clearButtonWidth, 22);
}