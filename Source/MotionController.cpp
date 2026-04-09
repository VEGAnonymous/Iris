#include "MotionController.h"
#include "Utilities.h"

#include <JuceHeader.h>

/* PRIVATE */

/* 

    PolarMap* polarMap;
    float* positionTime; float* fieldTime;
    bool positionUpdated = false, fieldUpdated = false;

    struct PositionParameters {
        PositionPattern positionPattern = PositionPattern::LISSAJOUS;
        float positionRate = 0.5f;
        float positionModA = 0.5f, positionModB = 0.5f;
    };
    struct PositionState {
        PolarCoordinate currentPosition {0.0f, 0.0f};
        // Discrete
        CartesianCoordinate targetPosition {0.0f, 0.0f};
        bool hasTarget = false;
        // Walk
        CartesianCoordinate walkVelocity {0.0f, 0.0f};
    };
    PositionParameters positionParameters;
    PositionState positionState;

    struct FieldParameters {
        FieldPattern fieldPattern = FieldPattern::RING;
        float fieldRate = 0.5f;
        float fieldModA = 0.5f, fieldModB = 0.5f;
    };
    struct FieldState {
        std::vector<PolarCoordinate> currentCoordinates {};
    };
    FieldParameters fieldParameters;
    FieldState fieldState;

*/

/* PUBLIC */

MotionController::MotionController(PolarMap* map, float* positionTimer, float* fieldTimer) 
    : polarMap(map), positionTime(positionTimer), fieldTime(fieldTimer) {}

PolarCoordinate MotionController::randomDiscrete(PositionParameters positionParameters, PositionState& positionState, float t) {
    float& radius = positionParameters.positionModA; 
    float smoothing = juce::jmap(1.0f - positionParameters.positionModB, 0.05f, 1.0f);

    CartesianCoordinate currentPosition = polarToCartesian(positionState.currentPosition);
    CartesianCoordinate& targetPosition = positionState.targetPosition;
    bool& hasTarget = positionState.hasTarget;

    float probability = std::fabs(positionParameters.positionRate) * 0.1f;
    if (randFloat() < probability && !hasTarget) {
        // Set new target
        float r = std::sqrt(randFloat()) * radius;
        float theta = randFloat() * juce::MathConstants<float>::twoPi;
        targetPosition = polarToCartesian({ r, theta });
        hasTarget = true;
    }

    // Check if arrived
    float dx = currentPosition.x - targetPosition.x, dy = currentPosition.y - targetPosition.y;
    if (((dx * dx) + (dy * dy)) < 1e-3f) {
        hasTarget = false; return cartesianToPolar(currentPosition);
    }

    // Smooth toward target position
    currentPosition = { juce::jmap(smoothing, currentPosition.x, targetPosition.x),
                        juce::jmap(smoothing, currentPosition.y, targetPosition.y) };

    return cartesianToPolar(currentPosition);
}

PolarCoordinate MotionController::randomWalk(PositionParameters positionParameters, PositionState& positionState, float t) {
    const float step = 0.02f * positionParameters.positionRate;
    const float damping = juce::jmap(positionParameters.positionModA * positionParameters.positionModA, 0.85f, 0.98f);
    const float& bounce = positionParameters.positionModB;

    CartesianCoordinate& velocity = positionState.walkVelocity;
    CartesianCoordinate p = polarToCartesian(positionState.currentPosition);

    velocity.x += randSigned() * step; velocity.y += randSigned() * step; // Apply random force
    velocity.x *= damping; velocity.y *= damping;

    p.x += velocity.x; p.y += velocity.y; // Apply movement

    // Constrain to unit circle
    float mag = std::sqrt(p.x * p.x + p.y * p.y);
    if (mag > 1.0f) {
        p.x /= mag; p.y /= mag;
        velocity.x *= -0.1f - bounce; velocity.y *= -0.1f - bounce; // Bounce
    }

    return cartesianToPolar(p);
}

PolarCoordinate MotionController::computePositionParametric(PositionParameters positionParameters, float t) {
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
    switch (positionParameters.positionPattern) {
        case PositionPattern::RANDOM_DISCRETE: return randomDiscrete(positionParameters, positionState, t);
        case PositionPattern::RANDOM_WALK: return randomWalk(positionParameters, positionState, t);
        default: return computePositionParametric(positionParameters, t);
    }
}

// TODO: Refactor to pass output vector by reference
// TODO: Implement remaining patterns
std::vector<PolarCoordinate> MotionController::computeFieldParametric(FieldParameters fieldParameters, float t) { 
    float& fieldRate = fieldParameters.fieldRate, fieldModA = fieldParameters.fieldModA, fieldModB = fieldParameters.fieldModB;
    int& fieldCount = fieldParameters.fieldCount;
    std::vector<PolarCoordinate> out; out.reserve(fieldCount);
    switch (fieldParameters.fieldPattern) {
        default:
        case FieldPattern::RING: {
            float& radius = fieldModA; 
            float offset = juce::jmap(fieldModB, 0.0f, juce::MathConstants<float>::twoPi);
            for (int coord = 0; coord < fieldCount; ++coord)
                out.push_back({ radius, ((coord * juce::MathConstants<float>::twoPi) / fieldCount) + offset + t});
            return out;
        }
    }
}

std::vector<PolarCoordinate> MotionController::computeField(FieldParameters fieldParameters, FieldState& fieldState, float t) {
    auto algorithm = [&](auto&& function) {
        std::vector<PolarCoordinate> coordinates; coordinates.reserve(fieldParameters.fieldCount);
        PositionParameters coordinateParameters{ PositionPattern::MANUAL,
            fieldParameters.fieldRate, fieldParameters.fieldModA, fieldParameters.fieldModB };

        for (int coord = 0; coord < fieldParameters.fieldCount; ++coord)
            coordinates.emplace_back(function(coordinateParameters, fieldState.coordinateStates[coord], t));
        return coordinates;
    };

    switch (fieldParameters.fieldPattern) {
        case FieldPattern::RANDOM_DISCRETE: return algorithm(randomDiscrete);
        case FieldPattern::RANDOM_WALK: return algorithm(randomWalk);
        default: return computeFieldParametric(fieldParameters, t);
    }
}

void MotionController::updatePosition() {
    PolarCoordinate current = polarMap->getPosition();
    positionState.currentPosition = current;

    PolarCoordinate next = computePosition(positionParameters, positionState, *positionTime);

    if (current == next) positionUpdated = false;
    else { polarMap->setPosition(next); positionUpdated = true; }
}

void MotionController::updateField() {
    const auto& current = polarMap->getCoordinates();
    int coordinateCount = polarMap->getCoordinateCount();
    fieldState.coordinateStates.resize(coordinateCount);
    for (int coord = 0; coord < coordinateCount; coord++) 
        fieldState.coordinateStates[coord].currentPosition = current[coord];

    std::vector<PolarCoordinate> next = computeField(fieldParameters, fieldState, *fieldTime);

    if (current == next) fieldUpdated = false;
    else { polarMap->setCoordinates(next); fieldUpdated = true; }
}

void MotionController::setPolarMap(PolarMap* nPolarMap) { polarMap = nPolarMap; }
void MotionController::setPositionTimer(float* nPT) { positionTime = nPT; }
void MotionController::setFieldTimer(float* nFT) { fieldTime = nFT; }
void MotionController::setPositionParameters(PositionParameters nPositionParameters) { positionParameters = nPositionParameters; }
void MotionController::setFieldParameters(FieldParameters nFieldParameters) { fieldParameters = nFieldParameters; };

bool MotionController::hasPositionUpdated() const { return positionUpdated; }
bool MotionController::hasFieldUpdated() const { return fieldUpdated; }