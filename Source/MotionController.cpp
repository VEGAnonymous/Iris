#include "MotionController.h"
#include "Utilities.h"

#include <JuceHeader.h>

/* PRIVATE */

/* 

    PolarMap* polarMap;
    float* t;
    MotionPattern motionPattern = MotionPattern::LISSAJOUS;
    float motionRate = 0.5f, motionModA = 0.5f, motionModB = 0.5f;

*/

/* PUBLIC */

MotionController::MotionController(PolarMap* initMap, float* initT) : polarMap(initMap), t(initT) {}

PolarCoordinate MotionController::computeParametric(MotionParameters motionParameters, float t) {
    float& motionModA = motionParameters.motionModA, motionModB = motionParameters.motionModB;
    switch (motionParameters.motionPattern) {
        case MotionPattern::MANUAL: {
            float& r = motionModA; float theta = motionModB * juce::MathConstants<float>::twoPi;
            return { r, theta };
        }
        case MotionPattern::ORBIT: {
            float& radius = motionModA;
            float r = radius, theta = 3 * t;
            return { r, theta };
        }
        case MotionPattern::SPIRAL: {
            float swirliness = juce::jmap(motionModA, 1.0f, 10.0f);
            float r = sinf(0.1f * t), theta = swirliness * t;
            return { r, theta };
        }
        case MotionPattern::FLORAL: {
            float p = floorf(juce::jmap(motionModA, 1.0f, 10.0f)), q = floorf(juce::jmap(motionModB, 1.0f, 10.0f));
            float r = sinf(p * t), theta = q * t;
            return { r, theta };
        }
        case MotionPattern::LISSAJOUS: {
            float p = floorf(juce::jmap(motionModA, 1.0f, 10.0f)), q = floorf(juce::jmap(motionModB, 1.0f, 10.0f));
            const float phi = juce::MathConstants<float>::halfPi;
            float r = sinf(p * t), theta = phi * sinf((q * t) + phi);
            return { r, theta };
        }

        // Stochastic, N/A
        case MotionPattern::RANDOM_DISCRETE:
        case MotionPattern::RANDOM_WALK: return { 0.0f, 0.0f };
        default: return { 0.0f, 0.0f };
    }
}

PolarCoordinate MotionController::computePosition(MotionParameters motionParameters, MotionState& motionState, float t) {
    float& motionModA = motionParameters.motionModA, motionModB = motionParameters.motionModB;
    switch (motionParameters.motionPattern) {
        case MotionPattern::RANDOM_DISCRETE: {
            float& radius = motionModA; float smoothing = juce::jmap(1.0f - motionModB, 0.05f, 1.0f);
            CartesianCoordinate currentPosition = polarToCartesian(motionState.currentPosition);
            CartesianCoordinate& targetPosition = motionState.targetPosition;
            bool& hasTarget = motionState.hasTarget;

            float probability = motionParameters.motionRate * 0.1f;
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
        case MotionPattern::RANDOM_WALK: {
            const float step = 0.02f * motionParameters.motionRate;
            const float damping = juce::jmap(motionModA * motionModA, 0.85f, 0.98f);
            const float& bounce = motionModB;

            CartesianCoordinate& velocity = motionState.walkVelocity;
            CartesianCoordinate p = polarToCartesian(motionState.currentPosition);

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
        default: return computeParametric(motionParameters, t);
    }
}

void MotionController::updatePosition() {
    PolarCoordinate current = polarMap->getPosition();
    motionState.currentPosition = current;

    PolarCoordinate next = computePosition(motionParameters, motionState, *t);

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
void MotionController::setMotionPattern(MotionPattern nMotionPattern) { motionParameters.motionPattern = nMotionPattern; }
void MotionController::setMotionRate(float nMotionRate) { motionParameters.motionRate = nMotionRate; }
void MotionController::setMotionModA(float nMotionModA) { motionParameters.motionModA = nMotionModA; }
void MotionController::setMotionModB(float nMotionModB) { motionParameters.motionModB = nMotionModB; }

PolarMap* MotionController::getPolarMap() const { return polarMap; }
float* MotionController::getTimer() const { return t; }
MotionPattern MotionController::getMotionPattern() const { return motionParameters.motionPattern; }
float MotionController::getMotionRate() const { return motionParameters.motionRate; }
float MotionController::getMotionModA() const { return motionParameters.motionModA; }
float MotionController::getMotionModB() const { return motionParameters.motionModB; }
bool MotionController::hasUpdated() const { return updated; }