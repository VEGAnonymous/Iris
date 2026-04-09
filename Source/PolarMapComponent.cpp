#include "PolarMapComponent.h"
#include "PluginProcessor.h"

PolarMapComponent::PolarMapComponent(MareverbAudioProcessor& p) : audioProcessor(p) { setBufferedToImage(true); }

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

void PolarMapComponent::notifyPathChanged() { pathChanged = true; repaint(); }
void PolarMapComponent::notifyPositionChanged(PolarCoordinate nPosition) {
    currentPosition = nPosition;
    repaint(positionBounds.toNearestInt().expanded(1)); // Clear old bounds
    repaint(toBounds(nPosition, positionRadius).toNearestInt().expanded(1)); // Repaint at new bounds
}
void PolarMapComponent::notifyFieldChanged(std::vector<PolarCoordinate> nCoordinates) {
    for (const auto& coord : fieldCoordinates) repaint(toBounds(coord, coordinateRadius).toNearestInt().expanded(1)); // Clear old bounds
    fieldCoordinates = std::move(nCoordinates);
    for (const auto& coord : fieldCoordinates) repaint(toBounds(coord, coordinateRadius).toNearestInt().expanded(1)); // Repaint at new bounds
}

void PolarMapComponent::repaintPath() {
    parametricPath.clear();

    Settings settings = MareverbAudioProcessor::getSettings(audioProcessor.apvts);
    PositionPattern& positionPattern = settings.positionPattern;

    if (positionPattern == PositionPattern::RANDOM_DISCRETE || positionPattern == PositionPattern::RANDOM_WALK) return;
    float& positionModA = settings.positionModA, positionModB = settings.positionModB;
    CartesianCoordinate initP = map(polarToCartesian(MotionController::computePositionParametric({ positionPattern, 0.0f, positionModA, positionModB }, 0.0f))); // t = 0
    parametricPath.startNewSubPath(initP.x, initP.y);
    for (float t = 0.01f; t < 10.0f; t += 0.01f) {
        CartesianCoordinate p = map(polarToCartesian(MotionController::computePositionParametric({ positionPattern, 0.0f, positionModA, positionModB }, t)));
        parametricPath.lineTo(p.x, p.y);
    }

    pathChanged = false;
}

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
    for (const auto& coordinate : fieldCoordinates) {
        auto coordinateBounds = toBounds(coordinate, coordinateRadius);
        g.setColour(juce::Colours::aqua);
        g.fillEllipse(coordinateBounds);
    }
}