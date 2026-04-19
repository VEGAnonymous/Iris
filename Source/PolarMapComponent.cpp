#include "GUIUtilities.h"
#include "PolarMapComponent.h"
#include "PluginProcessor.h"

/* PRIVATE */

CartesianCoordinate PolarMapComponent::map(CartesianCoordinate p) const { // [-1, 1] -> pixel coordinates
    auto b = getLocalBounds().toFloat();
    return {
        juce::jmap(p.x, -1.0f, 1.0f, b.getX(), b.getRight()),
        juce::jmap(p.y, -1.0f, 1.0f, b.getY(), b.getBottom())
    };
}

CartesianCoordinate PolarMapComponent::inverseMap(CartesianCoordinate p) const {
    auto b = getLocalBounds().toFloat();
    return {
        juce::jmap(p.x, b.getX(), b.getRight(),  -1.0f, 1.0f),
        juce::jmap(p.y, b.getY(), b.getBottom(), -1.0f, 1.0f)
    };
}

juce::Rectangle<float> PolarMapComponent::toBounds(PolarCoordinate p, float radius) const { // Coordinate -> fillEllipse() bounds
    CartesianCoordinate c = map(polarToCartesian(p));
    return { c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f };
}

void PolarMapComponent::repaintPath() {
    parametricPath.clear();

    ParameterSettings settings = getParameterSettings(audioProcessor.apvts);
    PositionPattern& positionPattern = settings.positionPattern;
    if (positionPattern == PositionPattern::RANDOM_DISCRETE || positionPattern == PositionPattern::RANDOM_WALK) return;

    float positionModA = settings.positionModA;
    float positionModB = settings.positionModB;

    for (float t = 0.0f; t < 10.0f; t += 0.01f) {
        PolarCoordinate p = MotionController::computePositionParametric({ positionPattern, 0.0f, positionModA, positionModB }, t);
        CartesianCoordinate c = map(polarToCartesian(p));
        if (t == 0.0f) parametricPath.startNewSubPath(c.x, c.y);
        else parametricPath.lineTo(c.x, c.y);
    }

    pathChanged = false;
}

bool PolarMapComponent::hitPosition(juce::Point<float> p) const {
    auto center = toBounds(currentPosition, positionRadius).getCentre();
    return p.getDistanceFrom(center) <= positionRadius + hitRadius;
}

int PolarMapComponent::hitField(juce::Point<float> p) const {
    for (int ir = 0; ir < MAX_IR_COUNT; ++ir) {
        auto center = toBounds(fieldCoordinates[ir], coordinateRadius).getCentre();
        if (p.getDistanceFrom(center) <= coordinateRadius + hitRadius) return ir;
    }
    return -1;
}

/* PUBLIC */

PolarMapComponent::PolarMapComponent(MareverbAudioProcessor& p) : audioProcessor(p) { setBufferedToImage(true); }

void PolarMapComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();

    // Background + border
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::floralwhite);
    // g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    g.setOpacity(0.5f);
    g.drawEllipse(bounds, 2.0f);

    // Parametric path
    if (pathChanged) repaintPath();
    if (!parametricPath.isEmpty()) {
        g.setColour(juce::Colours::white);
        g.setOpacity(0.2f);
        g.strokePath(parametricPath, juce::PathStrokeType(2.0f));
    }

    g.setOpacity(1.0f);

    // Position indicator
    positionBounds = toBounds(currentPosition, positionRadius);
    g.setColour(juce::Colours::lightcyan);
    g.fillEllipse(positionBounds);

    // Field indicators
    auto* irManager = audioProcessor.getIRManager();
    for (int i = 0; i < fieldCoordinates.size(); ++i) {
        const auto& coordinate = fieldCoordinates[i];
        Paint::irIndicator(g, map(polarToCartesian(coordinate)), coordinateRadius, i, 
            irManager->getIRSlot(i).occupied, irManager->getIRSlot(i).active);
    }
}

void PolarMapComponent::mouseDown(const juce::MouseEvent& e) {
    auto p = e.position;
    auto positionPattern = static_cast<PositionPattern>(audioProcessor.apvts.getRawParameterValue(ParamID::positionPattern)->load());
    if (positionPattern == PositionPattern::MANUAL && hitPosition(p)) {
        dragTarget = DragTarget::POSITION;
        return;
    }

    fieldIndex = hitField(p);
    if (fieldIndex >= 0) {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (fieldIndex != selectedIR) {
            audioProcessor.apvts.state.setProperty(PropertyID::selectedIR, fieldIndex, nullptr);
            switchedIR.store(true, std::memory_order_release);
            audioProcessor.guiState.syncingField.store(true, std::memory_order_release);
        }

        auto fieldPattern = static_cast<FieldPattern>(audioProcessor.apvts.getRawParameterValue(ParamID::fieldPattern)->load());
        if (fieldPattern == FieldPattern::MANUAL) {
            dragTarget = DragTarget::FIELD;
            return;
        }
    }

    dragTarget = DragTarget::NONE;
}

void PolarMapComponent::mouseDrag(const juce::MouseEvent& e) {
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

void PolarMapComponent::mouseUp(const juce::MouseEvent&) {
    dragTarget = DragTarget::NONE;
    fieldIndex = -1;
}

std::atomic<bool>& PolarMapComponent::getIRSwitched() { return switchedIR; }

void PolarMapComponent::notifyPathChanged() { pathChanged = true; repaint(); }

void PolarMapComponent::notifyPositionChanged(PolarCoordinate nPosition) {
    currentPosition = nPosition;
    repaint(positionBounds.toNearestInt().expanded(1)); // Clear old bounds
    repaint(toBounds(nPosition, positionRadius).toNearestInt().expanded(1)); // Repaint at new bounds
}

void PolarMapComponent::notifyFieldChanged(std::vector<PolarCoordinate> nCoordinates) {
    // Clear old bounds
    for (const auto& coord : fieldCoordinates) {
        auto oBounds = toBounds(coord, coordinateRadius);
        repaint(oBounds.toNearestInt().expanded(1));
    }

    // Repaint at new bounds
    fieldCoordinates = std::move(nCoordinates);
    for (const auto& coord : fieldCoordinates) {
        auto nBounds = toBounds(coord, coordinateRadius);
        repaint(nBounds.toNearestInt().expanded(1));
    }
}