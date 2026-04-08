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

void PolarMapComponent::buildPath() {
    parametricPath.clear();

    Settings settings = MareverbAudioProcessor::getSettings(audioProcessor.apvts);
    PositionPattern& positionPattern = settings.positionPattern;

    if (positionPattern == PositionPattern::RANDOM_DISCRETE || positionPattern == PositionPattern::RANDOM_WALK) return;
    float& positionModA = settings.positionModA, positionModB = settings.positionModB;
    CartesianCoordinate initP = map(polarToCartesian(MotionController::computeParametric({positionPattern, 0.0f, positionModA, positionModB}, 0.0f))); // t = 0
    parametricPath.startNewSubPath(initP.x, initP.y);
    for (float t = 0.01f; t < 10.0f; t += 0.01f) {
        CartesianCoordinate p = map(polarToCartesian(MotionController::computeParametric({positionPattern, 0.0f, positionModA, positionModB}, t)));
        parametricPath.lineTo(p.x, p.y);
    }

    pathChanged = false;
}

void PolarMapComponent::notifyPathChanged() { pathChanged = true; repaint(); }
void PolarMapComponent::notifyPositionChanged(PolarCoordinate nPosition) {
    currentPosition = nPosition;
    repaint(indicatorBounds.toNearestInt().expanded(1)); // Clear old position
    
    // Paint new position
    CartesianCoordinate nPos = map(polarToCartesian(nPosition));
    auto nBounds = juce::Rectangle<float>(nPos.x - indicatorRadius, nPos.y - indicatorRadius, 
        indicatorRadius * 2.0f, indicatorRadius * 2.0f);
    repaint(nBounds.toNearestInt().expanded(1));
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
    if (pathChanged) buildPath();
    if (!parametricPath.isEmpty()) {
        g.setColour(juce::Colours::white);
        g.setOpacity(0.2f);
        g.strokePath(parametricPath, juce::PathStrokeType(2.0f));
    }

    // Position indicator
    CartesianCoordinate currentPos = map(polarToCartesian(currentPosition));
    indicatorBounds = { currentPos.x - indicatorRadius, currentPos.y - indicatorRadius,
        indicatorRadius * 2.0f, indicatorRadius * 2.0f };
    g.setColour(juce::Colours::lightcyan);
    g.setOpacity(1.0f);
    g.fillEllipse(indicatorBounds);
}