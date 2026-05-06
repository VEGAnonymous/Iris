#include "Core/Defines.h"
#include "GUI/Components/Controls/EnvelopeControl.h"

/* PRIVATE */

float EnvelopeControl::map(float x) const {
    const auto bounds = curveBounds.toFloat().reduced(2.0f);
    return juce::jlimit(0.0f, 1.0f, (x - bounds.getX()) / bounds.getWidth());
}

EnvelopeControl::DragTarget EnvelopeControl::hitSection(float x) const {
    const bool isPerc = (envelope.type == EnvelopeType::PERC);
    const float arCutoff = isPerc ? juce::jlimit(0.05f, 1.0f, envelope.attack) : 0.5f;
    return (map(x) < arCutoff) ? DragTarget::ATTACK : DragTarget::RELEASE;
}

void EnvelopeControl::drawCurve(juce::Graphics& g, juce::Rectangle<int> area) {
    auto bounds = area.toFloat().reduced(2.0f);

    // Background + border
    g.setColour(Theme::Colors::background);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(Theme::Colors::outline);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

    // Default curve
    if (previewCurve.empty()) {
        g.setColour(Theme::Colors::highlight.withAlpha(0.5f));
        g.drawLine(bounds.getX(), bounds.getCentreY(), bounds.getRight(), bounds.getCentreY(), 1.0f);
        return;
    }

    const int length = static_cast<int>(previewCurve.size());
    bounds = bounds.reduced(3.0f);

    // Build fill path
    juce::Path path;
    path.startNewSubPath(bounds.getX(), bounds.getBottom());
    for (int i = 0; i < length; ++i) {
        const float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(length - 1)) * bounds.getWidth();
        const float y = bounds.getBottom() - (previewCurve[i] * bounds.getHeight());
        if (i == 0) path.lineTo(x, bounds.getBottom());
        path.lineTo(x, y);
    }
    path.lineTo(bounds.getRight(), bounds.getBottom());
    path.closeSubPath();
    g.setColour(Theme::Colors::highlight.withAlpha(0.15f));
    g.fillPath(path);

    // Build outline path
    juce::Path outline;
    for (int i = 0; i < length; ++i) {
        const float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(length - 1)) * bounds.getWidth();
        const float y = bounds.getBottom() - (previewCurve[i] * bounds.getHeight());
        if (i == 0) outline.startNewSubPath(x, y);
        else outline.lineTo(x, y);
    }
    g.setColour(Theme::Colors::highlight);
    g.strokePath(outline, juce::PathStrokeType(1.5f));
}

void EnvelopeControl::updateCurve() {
    Envelope::computeEnvelopeCurve(previewCurve, ENVELOPE_PREVIEW_POINTS, envelope.type, envelope.attack, envelope.release);
}

/* PUBLIC */

EnvelopeControl::EnvelopeControl() {
    /*
    typeSelector.addItemList(envelopeTypes, 1);
    typeSelector.setLookAndFeel(&comboBoxLookAndFeel);
    typeSelector.onChange = [&]() {
        auto type = static_cast<EnvelopeType>(typeSelector.getSelectedItemIndex());
        if (type != EnvelopeType::PERC) {
            envelope.attack = juce::jlimit(0.0f, 0.5f, envelope.attack);
            envelope.release = juce::jlimit(0.0f, 0.5f, envelope.release);
        }
        onEnvelopeChanged(type, envelope.attack, envelope.release);
        repaint();
    };
    addAndMakeVisible(typeSelector);
    */
}

EnvelopeControl::~EnvelopeControl() {
    typeSelector.setLookAndFeel(nullptr);
}

void EnvelopeControl::setSlot(const IRSlotLite slot) {
    updatingSlot = true; // Block callback while updating

    envelope = slot.window.envelope;
    typeSelector.setSelectedItemIndex(static_cast<int>(slot.window.envelope.type), juce::dontSendNotification);

    updatingSlot = false;
    repaint();
}

void EnvelopeControl::paint(juce::Graphics& g) {
    updateCurve();
    drawCurve(g, curveBounds);
}

void EnvelopeControl::resized() {
    auto bounds = getLocalBounds();
    curveBounds = bounds.removeFromTop(bounds.getHeight());
    // typeSelector.setBounds(bounds.withSizeKeepingCentre(static_cast<int>(bounds.getWidth() * 0.7f), bounds.getHeight()));
}

void EnvelopeControl::mouseMove(const juce::MouseEvent& e) {
    if (!curveBounds.toFloat().contains(e.position)) {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    } else {
        const float arCutoff = (envelope.type == EnvelopeType::PERC) ? juce::jlimit(0.05f, 1.0f, envelope.attack) : 0.5f;
        setMouseCursor(map(e.position.x) < arCutoff
            ? juce::MouseCursor::LeftEdgeResizeCursor
            : juce::MouseCursor::RightEdgeResizeCursor);
    }
}

void EnvelopeControl::mouseDown(const juce::MouseEvent& e) {
    if (!curveBounds.toFloat().contains(e.position)) return;
    const bool isPerc = (envelope.type == EnvelopeType::PERC);

    dragTarget = hitSection(e.position.x);
    dragStartValue = dragTarget == DragTarget::ATTACK
        ? (isPerc ? envelope.attack : std::sqrt(envelope.attack * 0.5f))
        : (isPerc ? envelope.release : std::sqrt(envelope.release * 0.5f));

    updateValueTooltipText();
    showValueTooltip();
    repaint();
}

void EnvelopeControl::mouseDrag(const juce::MouseEvent& e) {
    if (dragTarget == DragTarget::NONE || updatingSlot) return;

    const bool isPerc = (envelope.type == EnvelopeType::PERC);
    float dx = static_cast<float>(e.getDistanceFromDragStartX()) / static_cast<float>(curveBounds.getWidth());

    if (dragTarget == DragTarget::ATTACK) {
        const float attackLin = juce::jlimit(0.0f, isPerc ? 1.0f : 0.5f - EPSILON, dragStartValue + dx);
        envelope.attack = isPerc ? attackLin : ((attackLin * 2.0f) * (attackLin * 2.0f) * 0.5f);
    } else {
        const float releaseLin = juce::jlimit(0.0f, isPerc ? 1.0f : 0.5f - EPSILON, dragStartValue - dx);
        envelope.release = isPerc ? releaseLin : ((releaseLin * 2.0f) * (releaseLin * 2.0f) * 0.5f);
    }

    updateValueTooltipText();
    repaint();
}

void EnvelopeControl::mouseUp(const juce::MouseEvent& /*e*/) {
    if (dragTarget != DragTarget::NONE && !updatingSlot && onEnvelopeChanged)
        onEnvelopeChanged(static_cast<EnvelopeType>(typeSelector.getSelectedItemIndex()), envelope.attack, envelope.release);

    dragTarget = DragTarget::NONE;
    repaint();
}

void EnvelopeControl::mouseEnter(const juce::MouseEvent& e) {
    updateValueTooltipPosition(e.position);
}

void EnvelopeControl::mouseExit(const juce::MouseEvent& /*e*/) {
    dragTarget = DragTarget::NONE;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    hideValueTooltip();
    repaint();
}

void EnvelopeControl::mouseDoubleClick(const juce::MouseEvent& e) {
    auto target = hitSection(e.position.x);
    if (target == DragTarget::ATTACK) envelope.attack = 0.0f;
    else if (target == DragTarget::RELEASE) envelope.release = 0.0f;

    onEnvelopeChanged(static_cast<EnvelopeType>(typeSelector.getSelectedItemIndex()), envelope.attack, envelope.release);

    updateValueTooltipText();
    repaint();
}

juce::String EnvelopeControl::getValueTooltip() {
    float value = 0.0f;
    switch (dragTarget) {
        case DragTarget::ATTACK: {
            value = envelope.attack;
            break;
        }
        case DragTarget::RELEASE: {
            value = envelope.release;
            break;
        }
        case DragTarget::NONE:
        default: {
            value = 0.0f; // Shouldn't ever happen
            break;
        }
    }

    juce::String tooltipText = (envelope.type == EnvelopeType::PERC) 
        ? Format::dimensionless(juce::jmap(value, 1.26f, 40.0f), 4)
        : textFromValue(static_cast<double>(value));
    return tooltipText;
}