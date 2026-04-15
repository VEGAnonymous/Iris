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

juce::Rectangle<float> PolarMapComponent::toBounds(PolarCoordinate p, float radius) const { // Coordinate -> fillEllipse() bounds
    CartesianCoordinate c = map(polarToCartesian(p));
    return { c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f };
}

void PolarMapComponent::repaintPath() {
    parametricPath.clear();

    Settings settings = MareverbAudioProcessor::getSettings(audioProcessor.apvts);
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

/* PUBLIC */

PolarMapComponent::PolarMapComponent(MareverbAudioProcessor& p) : audioProcessor(p) { setBufferedToImage(true); }

void PolarMapComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();

    // Background + border
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::floralwhite);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
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