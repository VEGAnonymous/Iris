#include "IRHeaderComponent.h"
#include "GUIUtilities.h"

/* PRIVATE */

BoundsF IRHeaderComponent::getIndicatorBounds(Bounds bounds, const float radius) const {
    auto b = bounds.toFloat().reduced(2.0f);
    // HACK SHIT ALERT
    return { b.getX() + 12.0f - radius, b.getCentreY() - 0.5f - radius, radius * 2.0f, radius * 2.0f};
}

void IRHeaderComponent::mouseDown(const juce::MouseEvent& e) {
    if (getIndicatorBounds(getLocalBounds(), indicatorRadius).contains(e.position)) {
        currentIR.active = !currentIR.active;
        if (onActiveToggle) onActiveToggle(currentIR.active);
        repaint();
    }
}

/* PUBLIC */

IRHeaderComponent::IRHeaderComponent() {
    addAndMakeVisible(clearButton);
    clearButton.onClick = [this]() { if (onClear) onClear(); };
}

void IRHeaderComponent::setSlot(int irIndex, const IRSlot& slot) {
    currentIndex = irIndex;
    currentIR = slot;
    currentPath = slot.occupied ? slot.file.getParentDirectory().getFileName() + "/" + slot.file.getFileName() : "-";
    repaint();
}

void IRHeaderComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();

    // Indicator
    const float indicatorX = bounds.getX() + 14.0f;
    const float indicatorY = bounds.getCentreY();
    const float indicatorAlpha = currentIR.occupied ? (currentIR.active ? 1.0f : 0.35f) : (currentIR.active ? 0.2f : 0.1f);
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