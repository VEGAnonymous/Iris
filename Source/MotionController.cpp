#include "MotionController.h"
#include "Utilities.h"

#include <JuceHeader.h>

/* PRIVATE */

/* 

    PolarMap* polarMap;
    float* t;
    PositionPattern positionPattern = PositionPattern::LISSAJOUS;
    float positionRate = 0.5f, positionModA = 0.5f, positionModB = 0.5f;

*/

/* PUBLIC */

MotionController::MotionController(PolarMap* initMap, float* initT) : polarMap(initMap), t(initT) {}

PolarCoordinate MotionController::computeParametric(PositionParameters positionParameters, float t) {
    float& positionModA = positionParameters.positionModA, positionModB = positionParameters.positionModB;
    switch (positionParameters.positionPattern) {
        case PositionPattern::MANUAL: {
            float& r = positionModA; float theta = positionModB * juce::MathConstants<float>::twoPi;
            return { r, theta };
        }
        case PositionPattern::ORBIT: {
            float& radius = positionModA;
            float r = radius, theta = 3 * t;
            return { r, theta };
        }
        case PositionPattern::SPIRAL: {
            float swirliness = juce::jmap(positionModA, 1.0f, 10.0f);
            float r = sinf(0.1f * t), theta = swirliness * t;
            return { r, theta };
        }
        case PositionPattern::FLORAL: {
            float p = floorf(juce::jmap(positionModA, 1.0f, 10.0f)), q = floorf(juce::jmap(positionModB, 1.0f, 10.0f));
            float r = sinf(p * t), theta = q * t;
            return { r, theta };
        }
        case PositionPattern::LISSAJOUS: {
            float p = floorf(juce::jmap(positionModA, 1.0f, 10.0f)), q = floorf(juce::jmap(positionModB, 1.0f, 10.0f));
            const float phi = juce::MathConstants<float>::halfPi;
            float r = sinf(p * t), theta = phi * sinf((q * t) + phi);
            return { r, theta };
        }

        // Stochastic, N/A
        case PositionPattern::RANDOM_DISCRETE:
        case PositionPattern::RANDOM_WALK: return { 0.0f, 0.0f };
        default: return { 0.0f, 0.0f };
    }
}

PolarCoordinate MotionController::computePosition(PositionParameters positionParameters, PositionState& positionState, float t) {
    float& positionModA = positionParameters.positionModA, positionModB = positionParameters.positionModB;
    switch (positionParameters.positionPattern) {
        case PositionPattern::RANDOM_DISCRETE: {
            float& radius = positionModA; float smoothing = juce::jmap(1.0f - positionModB, 0.05f, 1.0f);
            CartesianCoordinate currentPosition = polarToCartesian(positionState.currentPosition);
            CartesianCoordinate& targetPosition = positionState.targetPosition;
            bool& hasTarget = positionState.hasTarget;

            float probability = positionParameters.positionRate * 0.1f;
            if (randFloat() < probability && !hasTarget) {
                // Set new target
                float r = std::sqrt(randFloat()) * radius;
                float theta = randFloat() * juce::MathConstants<float>::twoPi;
                targetPosition = polarToCartesian({r, theta});
                hasTarget = true;
            }

            // Check if arrived
            float dx = currentPosition.x - targetPosition.x, dy = currentPosition.y - targetPosition.y;
            if (((dx * dx) + (dy * dy)) < 1e-3f) {
                hasTarget = false; return cartesianToPolar(currentPosition); }

            // Smooth toward target position
            currentPosition = { juce::jmap(smoothing, currentPosition.x, targetPosition.x),
                                juce::jmap(smoothing, currentPosition.y, targetPosition.y) };

            return cartesianToPolar(currentPosition);
        }
        case PositionPattern::RANDOM_WALK: {
            const float step = 0.02f * positionParameters.positionRate;
            const float damping = juce::jmap(positionModA * positionModA, 0.85f, 0.98f);
            const float& bounce = positionModB;

            CartesianCoordinate& velocity = positionState.walkVelocity;
            CartesianCoordinate p = polarToCartesian(positionState.currentPosition);

            velocity.x += randSigned() * step; velocity.y += randSigned() * step; // Apply random force
            velocity.x *= damping; velocity.y *= damping;

            p.x += velocity.x; p.y += velocity.y; // Apply movement

            // Constrain to unit circle
            float mag = std::sqrt(p.x * p.x + p.y * p.y);
            if (mag > 1.0f) {
                p.x /= mag; p.y /= mag;
                velocity.x *= -0.1f -bounce; velocity.y *= -0.1f -bounce; // Bounce
            }

            return cartesianToPolar(p);
        }
        default: return computeParametric(positionParameters, t);
    }
}

void MotionController::updatePosition() {
    PolarCoordinate current = polarMap->getPosition();
    positionState.currentPosition = current;

    PolarCoordinate next = computePosition(positionParameters, positionState, *t);

    if (abs(current.r - next.r) < 1e-3 && abs(current.theta - next.theta) < 1e-3) { updated = false; }
    else { polarMap->setPosition(next); updated = true; }
    // DBG("Position: " << r << ", " << theta);
}

void MotionController::updateCoordinates() {
    // TODO
    /*
    std::vector<PolarCoordinate> coords;
    // Update them with some algorithm or pattern
    polarMap->setCoordinates(coords, false); // setRelatives = false - top level will call computeRelatives() later
    updated = true;
    */
}

void MotionController::setPolarMap(PolarMap* nPolarMap) { polarMap = nPolarMap; }
void MotionController::setTimer(float* nT) { t = nT; }
void MotionController::setPositionParameters(PositionParameters nPositionParameters) { positionParameters = nPositionParameters; }
void MotionController::setPositionPattern(PositionPattern nPositionPattern) { positionParameters.positionPattern = nPositionPattern; }
void MotionController::setPositionRate(float nPositionRate) { positionParameters.positionRate = nPositionRate; }
void MotionController::setPositionModA(float nPositionModA) { positionParameters.positionModA = nPositionModA; }
void MotionController::setPositionModB(float nPositionModB) { positionParameters.positionModB = nPositionModB; }

PolarMap* MotionController::getPolarMap() const { return polarMap; }
float* MotionController::getTimer() const { return t; }
PositionPattern MotionController::getPositionPattern() const { return positionParameters.positionPattern; }
float MotionController::getPositionRate() const { return positionParameters.positionRate; }
float MotionController::getPositionModA() const { return positionParameters.positionModA; }
float MotionController::getPositionModB() const { return positionParameters.positionModB; }
bool MotionController::hasUpdated() const { return updated; }