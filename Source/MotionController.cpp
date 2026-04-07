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

PolarCoordinate MotionController::computeParametric(MotionPattern motionPattern, float motionModA, float motionModB, float t) {
    switch (motionPattern) {
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

PolarCoordinate MotionController::computePosition(MotionPattern motionPattern, 
    float motionRate, float motionModA, float motionModB, float t,
    PolarCoordinate currentPosition, PolarCoordinate& randomTarget) {
    switch (motionPattern) {
        case MotionPattern::RANDOM_DISCRETE: {
            float& radius = motionModA; float& smoothing = motionModB;
            float probability = motionRate * 0.1f;
            if (randFloat() < probability) {
                // Set new target
                float r = std::sqrt(randFloat()) * radius;
                float theta = randFloat() * juce::MathConstants<float>::twoPi;
                randomTarget = { r, theta };
            }

            if (abs(currentPosition.r - randomTarget.r) < 1e-3 && abs(currentPosition.theta - randomTarget.theta) < 1e-3)
                return currentPosition;

            // Smooth toward target position
            float nextR = juce::jmap(smoothing, currentPosition.r, randomTarget.r);
            float dTheta = std::fmod(randomTarget.theta - currentPosition.theta + (juce::MathConstants<float>::pi * 3),
                juce::MathConstants<float>::twoPi) - juce::MathConstants<float>::pi;
            float nextTheta = currentPosition.theta + (dTheta * (1.0f - smoothing));
            return { nextR, nextTheta };
        }
        case MotionPattern::RANDOM_WALK: {
            float& radius = motionModA;
            float step = 0.02f * motionRate;
            CartesianCoordinate p = polarToCartesian(currentPosition);
            p.x += randSigned() * step; p.y += randSigned() * step;
            float mag = std::sqrt((p.x * p.x) + (p.y * p.y));
            if (mag > radius) { p.x /= mag; p.y /= mag; }
            return cartesianToPolar(p);
        }
        default: return computeParametric(motionPattern, motionModA, motionModB, t);
    }
}

void MotionController::updatePosition() {
    PolarCoordinate current = polarMap->getPosition();
    PolarCoordinate next = computePosition(motionPattern, motionRate, motionModA, motionModB, *t, current, randomTarget);

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
void MotionController::setMotionPattern(MotionPattern nMotionPattern) { motionPattern = nMotionPattern; }
void MotionController::setMotionRate(float nMotionRate) { if (nMotionRate != motionRate) DBG("Set motion rate " << nMotionRate); motionRate = nMotionRate; }
void MotionController::setMotionModA(float nMotionModA) { if (nMotionModA != motionModA) DBG("Set motion mod A " << nMotionModA);  motionModA = nMotionModA; }
void MotionController::setMotionModB(float nMotionModB) { if (nMotionModB != motionModB) DBG("Set motion mod B " << nMotionModB); motionModB = nMotionModB; }

PolarMap* MotionController::getPolarMap() const { return polarMap; }
float* MotionController::getTimer() const { return t; }
MotionPattern MotionController::getMotionPattern() const { return motionPattern; }
float MotionController::getMotionRate() const { return motionRate; }
float MotionController::getMotionModA() const { return motionModA; }
float MotionController::getMotionModB() const { return motionModB; }
bool MotionController::hasUpdated() const { return updated; }