#include "GUI/GUIUtilities.h"
#include "GUI/Panels/PolarMapPanel.h"
#include "PluginProcessor.h"

/* PRIVATE */

// Utilities

CartesianCoordinate PolarMapPanel::map(CartesianCoordinate p) const { // [-1, 1] -> pixel coordinates
    auto b = getLocalBounds().reduced(MAP_INSET).toFloat();
    return {
        juce::jmap(p.x, -1.0f, 1.0f, b.getX(), b.getRight()),
        juce::jmap(p.y, -1.0f, 1.0f, b.getY(), b.getBottom())
    };
}

CartesianCoordinate PolarMapPanel::inverseMap(CartesianCoordinate p) const {
    auto b = getLocalBounds().reduced(MAP_INSET).toFloat();
    return {
        juce::jmap(p.x, b.getX(), b.getRight(),  -1.0f, 1.0f),
        juce::jmap(p.y, b.getY(), b.getBottom(), -1.0f, 1.0f)
    };
}

BoundsF PolarMapPanel::toBounds(PolarCoordinate p, float radius) const { // Coordinate -> fillEllipse() bounds
    CartesianCoordinate c = map(polarToCartesian(p));
    return { c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f };
}

// Hovering

bool PolarMapPanel::hitPosition(juce::Point<float> p) const {
    auto center = toBounds(currentPosition, positionRadius).getCentre();
    return p.getDistanceFrom(center) <= (positionRadius + HIT_RADIUS);
}

int PolarMapPanel::hitField(juce::Point<float> p) const {
    for (int ir = 0; ir < fieldCoordinates.size(); ++ir) {
        auto center = toBounds(fieldCoordinates[ir], coordinateRadius).getCentre();
        if (p.getDistanceFrom(center) <= (coordinateRadius + HIT_RADIUS)) return ir;
    }
    return -1;
}

PolarMapPanel::HoverState PolarMapPanel::getHoverState(juce::Point<float> p) const {
    HoverState hoverState;

    const auto positionPattern = static_cast<PositionPattern>(
        audioProcessor.apvts.getRawParameterValue(ParamID::positionPattern)->load());

    if (positionPattern == PositionPattern::MANUAL && hitPosition(p)) {
        hoverState.hoveringPosition = true;
        return hoverState; // Prioritize position
    }

    hoverState.fieldIndex = hitField(p);
    return hoverState;
}

void PolarMapPanel::applyHoverState(const HoverState& hoverState) {
    positionInteractionState.setValue(0.0f);
    for (auto& indicatorInteractionState : fieldInteractionStates) indicatorInteractionState.setValue(0.0f);

    if (hoverState.hoveringPosition) {
        positionInteractionState.setValue(1.0f);
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
        return;
    }

    if (hoverState.fieldIndex >= 0) {
        fieldInteractionStates[hoverState.fieldIndex].setValue(1.0f);

        const auto fieldPattern = static_cast<FieldPattern>(
            audioProcessor.apvts.getRawParameterValue(ParamID::fieldPattern)->load());

        setMouseCursor((fieldPattern == FieldPattern::MANUAL) 
            ? juce::MouseCursor::CrosshairCursor 
            : juce::MouseCursor::NormalCursor);

        return;
    }

    setMouseCursor(juce::MouseCursor::NormalCursor);
}

// Parametric path

void PolarMapPanel::repaintPath() {
    parametricPath.clear();

    ParameterSettings settings = getParameterSettings(audioProcessor.apvts);
    PositionPattern& positionPattern = settings.positionPattern;
    if (positionPattern == PositionPattern::RANDOM_DISCRETE || positionPattern == PositionPattern::RANDOM_WALK) return;

    float positionModA = settings.positionModA;
    float positionModB = settings.positionModB;

    CartesianCoordinate last = { 0.0f, 0.0f };
    const float discontinuityThreshold = 0.2f;

    float tEnd = 10.0f;
    // Extend path time for certain patterns
    if (positionPattern == PositionPattern::EYES) tEnd = 20.0f;
    if (positionPattern == PositionPattern::SPIRAL) tEnd = 20.0f;

    for (float t = 0.0f; t < tEnd; t += 0.01f) {
        PolarCoordinate p = MotionController::computePositionParametric({ positionPattern, 0.0f, positionModA, positionModB }, t);
        CartesianCoordinate c = polarToCartesian(p);
        if (t == 0.0f) {
            last = c;
            c = map(c);
            parametricPath.startNewSubPath(c.x, c.y);
        } else {
            if (cartesianDistance(c, last) > discontinuityThreshold) { // Handle discontinuities
                last = c;
                c = map(c);
                parametricPath.startNewSubPath(c.x, c.y);
            } else {
                last = c;
                c = map(c);
                parametricPath.lineTo(c.x, c.y);
            }
        }
    }

    pathChanged = false;
}

/* PUBLIC */

PolarMapPanel::PolarMapPanel(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater) 
    : audioProcessor(processor), animatorUpdater(updater), 
    positionInteractionState(*this, updater) {

    fieldActiveStates.reserve(MAX_IR_COUNT);
    fieldInteractionStates.reserve(MAX_IR_COUNT);
    fieldSelectionStates.reserve(MAX_IR_COUNT);

    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        fieldActiveStates.emplace_back(*this, updater, ACTIVE_ANIMATION_TIME_MS);
        fieldInteractionStates.emplace_back(*this, updater, INDICATOR_HOVER_ANIMATION_TIME_MS);
        fieldSelectionStates.emplace_back(*this, updater, SELECTION_ANIMATION_TIME_MS);

        const auto& slot = audioProcessor.getIRManager()->getIRSlot(i);
        fieldActiveStates[i].setValue(getIRIndicatorAlpha(slot.occupied, slot.active), true);
    }

    setMouseCursor(juce::MouseCursor::NormalCursor);
    setBufferedToImage(true); 
}

void PolarMapPanel::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().reduced(MAP_INSET).toFloat();

    // Background + border
    g.fillAll(juce::Colours::black);
    g.setColour(Theme::Colors::textLight);
    g.setOpacity(0.621f);
    g.drawEllipse(bounds, 2.61f);

    // Parametric path
    if (pathChanged) repaintPath();
    if (!parametricPath.isEmpty()) {
        g.setColour(juce::Colours::white);
        g.setOpacity(0.2f);
        g.strokePath(parametricPath, juce::PathStrokeType(2.0f));
    }

    g.setOpacity(1.0f);

    // Field indicators


    for (int i = 0; i < fieldCoordinates.size(); ++i) {
        coordinateRadius = baseCoordinateRadius + juce::jmap(fieldInteractionStates[i].getValue(), 0.0f, 2.0f);
        const int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        const auto& coordinate = fieldCoordinates[i];
        const float indicatorAlpha = fieldActiveStates[i].getValue();
        const float selectionAlpha = fieldSelectionStates[i].getValue();
        Paint::irIndicator(g, map(polarToCartesian(coordinate)), coordinateRadius, i, false, false,
            (i == selectedIR), 
            indicatorAlpha, 
            selectionAlpha, 
            juce::Colours::transparentBlack,
            &fieldIndicatorIcons[i]
        );
    }

    // Position indicator
    g.setColour(Theme::Colors::textLight);
    positionRadius = basePositionRadius + juce::jmap(positionInteractionState.getValue(), 0.0f, 2.0f);
    if (!positionIndicatorIcon.isNull()) {
        positionBounds = toBounds(currentPosition, positionRadius + 8.0f);
        g.drawImage(positionIndicatorIcon, positionBounds, juce::RectanglePlacement::centred);
    } else {
        positionBounds = toBounds(currentPosition, positionRadius);;
        g.fillEllipse(positionBounds);
    }
}

void PolarMapPanel::mouseMove(const juce::MouseEvent& e) {
    applyHoverState(getHoverState(e.position));
}

void PolarMapPanel::mouseDown(const juce::MouseEvent& e) {
    auto hoverState = getHoverState(e.position);

    if (hoverState.hoveringPosition) {
        positionInteractionState.setValue(0.621f);
        dragTarget = DragTarget::POSITION;
        return;
    }

    if (hoverState.fieldIndex >= 0) {
        fieldInteractionStates[hoverState.fieldIndex].setValue(0.621f);

        const int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (hoverState.fieldIndex != selectedIR) {
            audioProcessor.apvts.state.setProperty(PropertyID::selectedIR, hoverState.fieldIndex, nullptr);
            switchedIR.store(true, std::memory_order_release);
            audioProcessor.guiState.syncingField.store(true, std::memory_order_release);
        }

        const auto fieldPattern = static_cast<FieldPattern>(
            audioProcessor.apvts.getRawParameterValue(ParamID::fieldPattern)->load());

        if (fieldPattern == FieldPattern::MANUAL) {
            dragTarget = DragTarget::FIELD;
            return;
        }
    }

    dragTarget = DragTarget::NONE;
}

void PolarMapPanel::mouseDrag(const juce::MouseEvent& e) {
    if (dragTarget == DragTarget::NONE) return;

    PolarCoordinate p = cartesianToPolar(inverseMap({ e.position.x, e.position.y }));
    p.r = juce::jlimit(0.0f, 1.0f, p.r);
    p.theta = wrapAngle(p.theta);

    if (dragTarget == DragTarget::POSITION) {
        auto* modA = audioProcessor.apvts.getParameter(ParamID::positionModA);
        auto* modB = audioProcessor.apvts.getParameter(ParamID::positionModB);
        if (modA) modA->setValueNotifyingHost(modA->convertTo0to1(p.r));
        if (modB) modB->setValueNotifyingHost(modB->convertTo0to1(p.theta / juce::MathConstants<float>::twoPi));
    }
    else if (dragTarget == DragTarget::FIELD) {
        auto* modA = audioProcessor.apvts.getParameter(ParamID::fieldModA);
        auto* modB = audioProcessor.apvts.getParameter(ParamID::fieldModB);
        if (modA) modA->setValueNotifyingHost(modA->convertTo0to1(p.r));
        if (modB) modB->setValueNotifyingHost(modB->convertTo0to1(p.theta / juce::MathConstants<float>::twoPi));
    }
}

void PolarMapPanel::mouseUp(const juce::MouseEvent& e) {
    dragTarget = DragTarget::NONE;
    applyHoverState(getHoverState(e.position));
}

void PolarMapPanel::mouseDoubleClick(const juce::MouseEvent& e) {
    const auto& p = e.position;

    const auto positionPattern = static_cast<PositionPattern>(
        audioProcessor.apvts.getRawParameterValue(ParamID::positionPattern)->load());

    if (positionPattern == PositionPattern::MANUAL && hitPosition(p)) {
        auto* modA = audioProcessor.apvts.getParameter(ParamID::positionModA);
        auto* modB = audioProcessor.apvts.getParameter(ParamID::positionModB);
        if (modA) modA->setValueNotifyingHost(0.0f);
        if (modB) modB->setValueNotifyingHost(0.0f);

        positionInteractionState.setValue(0.0f);
    }

    const auto fieldPattern = static_cast<FieldPattern>(
        audioProcessor.apvts.getRawParameterValue(ParamID::fieldPattern)->load());

    const int hitFieldIndex = hitField(p);
    if (fieldPattern == FieldPattern::MANUAL && hitFieldIndex >= 0) {
        auto* modA = audioProcessor.apvts.getParameter(ParamID::fieldModA);
        auto* modB = audioProcessor.apvts.getParameter(ParamID::fieldModB);
        if (modA) modA->setValueNotifyingHost(modA->convertTo0to1(0.0f));
        if (modB) modB->setValueNotifyingHost(modB->convertTo0to1(0.0f));

        fieldInteractionStates[hitFieldIndex].setValue(0.0f);
    }
}

// Updates

void PolarMapPanel::notifyPathChanged() { 
    pathChanged = true; 
    repaint(); 
}

void PolarMapPanel::notifyPositionChanged(PolarCoordinate nPosition) {
    currentPosition = nPosition;
    repaint(positionBounds.toNearestInt().expanded(10)); // Clear old bounds
    repaint(toBounds(nPosition, positionRadius).toNearestInt().expanded(10)); // Repaint at new bounds
}

void PolarMapPanel::notifyFieldChanged(std::vector<PolarCoordinate> nCoordinates, bool animate) {
    const float repaintRadius = baseCoordinateRadius + 10.0f;

    // Clear old bounds
    for (const auto& coord : fieldCoordinates) {
        auto oBounds = toBounds(coord, coordinateRadius);
        repaint(oBounds.expanded(repaintRadius).toNearestInt());
    }

    // Update animator targets
    const int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        const auto& slot = audioProcessor.getIRManager()->getIRSlot(i);
        fieldActiveStates[i].setValue(getIRIndicatorAlpha(slot.occupied, slot.active), animate);
        fieldSelectionStates[i].setValue((i == selectedIR) ? 1.0f : 0.0f, animate);
    }

    const auto& p = getMouseXYRelative().toFloat();
    if (dragTarget == DragTarget::NONE && !hitPosition(p)) { applyHoverState(getHoverState(p)); }

    // Repaint at new bounds
    fieldCoordinates = std::move(nCoordinates);
    for (const auto& coord : fieldCoordinates) {
        auto nBounds = toBounds(coord, coordinateRadius);
        repaint(nBounds.expanded(repaintRadius).toNearestInt());
    }
}

void PolarMapPanel::notifyIndicatorStyleChanged() {
    // Update position indicator style
    const juce::String positionIndicatorStyle =
        audioProcessor.apvts.state.getProperty(PropertyID::positionIndicatorStyle, PositionIndicatorStyle::Anon);

    juce::Image positionIcon {};
    if (positionIndicatorStyle != PositionIndicatorStyle::Mareless) {
        if (positionIndicatorStyle == PositionIndicatorStyle::Anon) positionIcon = Theme::Icons::getAnon();
        else positionIcon = Theme::Mares::getMare<Theme::Mares::AltMares>(positionIndicatorStyle);
    }
    positionIndicatorIcon = positionIcon;

    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        juce::SpinLock::ScopedLockType lock(audioProcessor.guiState.mareLock);
        updateFieldIndicatorStyle(
            fieldIndicatorIcons[i],
            audioProcessor.guiState.mareImages[i],
            audioProcessor.apvts.state.getProperty(PropertyID::fieldIndicatorStyle, FieldIndicatorStyle::Mareful)
        );
    }
}

std::atomic<bool>& PolarMapPanel::getIRSwitched() { return switchedIR; }