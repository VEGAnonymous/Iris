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

void MotionController::updatePosition() {
    switch (motionPattern) {
        case MotionPattern::MANUAL: {
            float& r = motionModA; float theta = motionModB * juce::MathConstants<float>::twoPi;
            PolarCoordinate currentPosition = polarMap->getPosition();

            if (abs(currentPosition.r - r) < 1e-3 && abs(currentPosition.theta - theta) < 1e-3) {
                updated = false; return; }
            polarMap->setPosition({ r, theta });
            updated = true;
            break;
        }
        case MotionPattern::ORBIT: {
            float& radius = motionModA;
            float r = radius, theta = *t;
            polarMap->setPosition({ r, theta });
            updated = true;
            break;
        }
        case MotionPattern::SPIRAL: {
            float swirliness = juce::jmap(motionModA, 1.0f, 10.0f);
            float r = sinf(0.1f * *t), theta = swirliness * *t;
            polarMap->setPosition({ r, theta });
            updated = true;
            break;
        }
        case MotionPattern::FLORAL: {
            float p = floorf(juce::jmap(motionModA, 1.0f, 10.0f)), q = floorf(juce::jmap(motionModB, 1.0f, 10.0f));
            float r = sinf(p * *t), theta = q * *t;
            polarMap->setPosition({ r, theta });
            updated = true;
            break;
        }
        case MotionPattern::LISSAJOUS: {
            float p = floorf(juce::jmap(motionModA, 1.0f, 10.0f)), q = floorf(juce::jmap(motionModB, 1.0f, 10.0f));
            const float phi = juce::MathConstants<float>::halfPi;
            float r = sinf(p * *t), theta = phi * sinf((q * *t) + phi);
            polarMap->setPosition({ r, theta });
            updated = true;
            break;
        }
        case MotionPattern::RANDOM_DISCRETE: {
            float& radius = motionModA; float& smoothing = motionModB;
            PolarCoordinate currentPosition = polarMap->getPosition();
            float probability = motionRate * 0.1f;
            if (randFloat() < probability) {
                // Set new target
                float r = std::sqrt(randFloat()) * radius;
                float theta = randFloat() * juce::MathConstants<float>::twoPi;
                randomTarget = { r, theta };
            }

            if (abs(currentPosition.r - randomTarget.r) < 1e-3 && abs(currentPosition.theta - randomTarget.theta) < 1e-3) { 
                updated = false; return; }

            // Smooth toward target position
            float nextR = juce::jmap(smoothing, currentPosition.r, randomTarget.r);
            float dTheta = std::fmod(randomTarget.theta - currentPosition.theta + (juce::MathConstants<float>::pi * 3),
                juce::MathConstants<float>::twoPi) - juce::MathConstants<float>::pi;
            float nextTheta = currentPosition.theta + (dTheta * (1.0f - smoothing));
            polarMap->setPosition({ nextR, nextTheta });
            updated = true;
            break;
        }
        case MotionPattern::RANDOM_WALK: {
            float& radius = motionModA;
            PolarCoordinate currentPosition = polarMap->getPosition();
            float step = 0.02f * motionRate;
            // Convert to Cartesian
            float x = currentPosition.r * std::cos(currentPosition.theta), y = currentPosition.r * std::sin(currentPosition.theta);
            // Step and constrain radius
            x += randSigned() * step; y += randSigned() * step;
            float mag = std::sqrt((x * x) + (y * y));
            if (mag > radius) { x /= mag; y /= mag; }
            // Convert to polar
            float r = std::sqrt((x * x) + (y * y)), theta = std::atan2(y, x);
            polarMap->setPosition({ r, theta });
            updated = true;
            break;
        }
    } // DBG("Position: " << r << ", " << theta);
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