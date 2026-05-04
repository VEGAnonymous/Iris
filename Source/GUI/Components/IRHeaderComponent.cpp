#include "GUI/GUIUtilities.h"
#include "GUI/Components/IRHeaderComponent.h"

/* PRIVATE */

BoundsF IRHeaderComponent::getIndicatorBounds(Bounds bounds, const float radius) const {
    auto b = bounds.toFloat().reduced(2.0f);
    // HACK SHIT ALERT
    return { b.getX() + 12.0f - radius, b.getCentreY() - 0.5f - radius, radius * 2.0f, radius * 2.0f};
}

void IRHeaderComponent::mouseMove(const juce::MouseEvent& e) {
    auto indicatorBounds = getIndicatorBounds(getLocalBounds(), indicatorRadius);
    if (indicatorBounds.contains(e.position))
        setTooltip("Enables/disables the selected IR. Disabled IRs don't contribute to the mix.");
}

void IRHeaderComponent::mouseDown(const juce::MouseEvent& e) {
    auto indicatorBounds = getIndicatorBounds(getLocalBounds(), indicatorRadius);
    if (indicatorBounds.contains(e.position)) {
        IRCommand cmd = { IRCommand::IR_SET_ACTIVE_STATE };
        cmd.irIndex = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        cmd.irActiveState = !currentIR.active;
        audioProcessor.getIRManager()->enqueueCommand(cmd);
        audioProcessor.guiState.updateField.store(true, std::memory_order_release);
    }
}

void IRHeaderComponent::mouseExit(const juce::MouseEvent& /*e*/) { setTooltip(""); }

/* PUBLIC */

IRHeaderComponent::IRHeaderComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater) 
    : audioProcessor(processor), indicatorActiveAnim(*this, updater, ACTIVE_ANIMATION_TIME_MS) {
    setBufferedToImage(true);
}

void IRHeaderComponent::setSlot(int irIndex, const IRSlotLite slot) {
    currentIndex = irIndex;
    currentIR = slot;

    const int maxPathLength = 62;
    const int maxParentLength = 21;

    const auto parent = slot.file.getParentDirectory();
    const auto parentPath = formatPath(parent.getFileName(), maxParentLength, Ellipsis::FRONT);
    auto fullPath = parentPath + "/" + formatPath(slot.file.getFileName(), maxPathLength - parentPath.length(), Ellipsis::MIDDLE);
    currentPath = slot.occupied ? fullPath : "---";
    repaint();
}

void IRHeaderComponent::setActive(bool nActive, bool animate) {
    indicatorActiveAnim.setValue(nActive ? 1.0f : 0.0f, animate);
    currentIR.active = nActive;
    repaint();
}

void IRHeaderComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    g.setColour(Theme::Colors::section);
    g.fillRoundedRectangle(bounds, PANEL_CORNER_SIZE);

    // Indicator
    const float indicatorX = bounds.getX() + 14.0f;
    const float indicatorY = bounds.getCentreY();

    float activeAlpha = indicatorActiveAnim.getValue();
    float indicatorAlpha = juce::jmap(activeAlpha, currentIR.occupied ? 0.35f : 0.1f, currentIR.occupied ? 1.0f : 0.2f);

    auto mareIndicator = &audioProcessor.guiState.mareImages[currentIndex];
    Paint::irIndicator(g, { indicatorX, indicatorY }, indicatorRadius - (!mareIndicator->isNull() ? 1.0f : 0.0f),
        currentIndex, currentIR.occupied, currentIR.active, false, indicatorAlpha, -1.0f, juce::Colours::transparentBlack, mareIndicator);

    // IR #
    const float labelX = (indicatorX + indicatorRadius) + 16.0f;
    const float labelWidth = 36.0f;
    g.setColour(Theme::Colors::textLight);
    g.setFont(Theme::Fonts::getEquestriaBoldFont(juce::FontOptions().withHeight(14.0f).withKerningFactor(0.1f)));
    g.drawText("IR " + juce::String(currentIndex), 
        BoundsF(labelX, 0, labelWidth, bounds.getHeight()), juce::Justification::centredLeft);

    // Filepath
    const float pathX = (labelX + labelWidth) + 16.0f;
    const float pathWidth = bounds.getWidth() - pathX - 10.0f;
    g.setColour(Theme::Colors::textLight);
    g.setFont(Theme::Fonts::getEquestriaNeueFont(juce::FontOptions().withHeight(12.0f).withKerningFactor(0.005f)));
    g.drawFittedText(currentPath, 
        BoundsF(pathX, 0, pathWidth, bounds.getHeight()).toNearestInt(), juce::Justification::centredLeft, 1, 1.0f);
}