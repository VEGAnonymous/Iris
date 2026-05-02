#include "Core/Utilities.h"
#include "Core/Control/MotionController.h"

#include <JuceHeader.h>

/* PRIVATE */

PolarCoordinate MotionController::randomDiscrete(PositionParameters positionParameters, PositionState& positionState) {
    CartesianCoordinate currentPosition = polarToCartesian(positionState.currentPosition);
    CartesianCoordinate& targetPosition = positionState.targetPosition;
    bool& hasTarget = positionState.hasTarget;

    float radius = positionParameters.positionModA;
    float smoothing = juce::jmap(1.0f - positionParameters.positionModB, 0.05f, 1.0f);

    const float probabilityScale = 0.0126f;
    float probability = std::fabs(positionParameters.positionRate) * probabilityScale;

    if (randFloat() < probability && !hasTarget) {
        // Set new target
        float r = std::sqrt(randFloat()) * radius,
              theta = randFloat() * juce::MathConstants<float>::twoPi;
        targetPosition = polarToCartesian({ r, theta });
        hasTarget = true;
    }

    // Check if arrived
    if (cartesianDistance(currentPosition, targetPosition) < EPSILON) {
        hasTarget = false;
        return cartesianToPolar(currentPosition);
    }

    // Smooth toward target position
    currentPosition = { juce::jmap(smoothing, currentPosition.x, targetPosition.x),
                        juce::jmap(smoothing, currentPosition.y, targetPosition.y) };

    return cartesianToPolar(currentPosition);
}

PolarCoordinate MotionController::randomWalk(PositionParameters positionParameters, PositionState& positionState) {
    CartesianCoordinate& velocity = positionState.walkVelocity;
    CartesianCoordinate p = polarToCartesian(positionState.currentPosition);

    const float rateScale = 0.00621f;
    const float step = positionParameters.positionRate * rateScale;

    const float damping = juce::jmap(positionParameters.positionModA * positionParameters.positionModA, 0.85f, 0.98f);
    const float bounce = positionParameters.positionModB;

    // Apply random force
    velocity.x += randSigned() * step; 
    velocity.y += randSigned() * step; 
    velocity.x *= damping; 
    velocity.y *= damping;

    // Apply movement
    p.x += velocity.x; 
    p.y += velocity.y;

    // Constrain to unit circle
    const float bounceBase = -0.1f;
    float mag = std::sqrt(p.x * p.x + p.y * p.y);
    if (mag > 1.0f) {
        p.x /= mag;
        p.y /= mag;
        velocity.x *= bounceBase - bounce; // boing
        velocity.y *= bounceBase - bounce; // boing
    }

    return cartesianToPolar(p);
}

/* PUBLIC */

MotionController::MotionController(PolarMap* map, float* positionTimer, float* fieldTimer) 
    : polarMap(map), positionTime(positionTimer), fieldTime(fieldTimer) {}

PolarCoordinate MotionController::computePositionParametric(PositionParameters positionParameters, float t) {
    float positionModA = positionParameters.positionModA;
    float positionModB = positionParameters.positionModB;

    switch (positionParameters.positionPattern) {
        case PositionPattern::MANUAL: {
            float r = positionModA;
            float theta = positionModB * juce::MathConstants<float>::twoPi;
            return { r, theta };
        }
        case PositionPattern::EYES: {
            // https://www.desmos.com/calculator/4imekbuger
            const float d = 0.52f;  // Horizontal distance from x = 0
            const float a = 0.35f;  // Minor-axis width
            const float b = 0.56f;   // Major-axis width
            float phi = juce::jmap(positionModA, -0.3f, 0.3f); // Tilt in radians

            const float timeScale = 2.0f;
            t *= timeScale;

            float x = (sgn(sinf(t / 2.0f)) * d) + (a * cosf(t) * cosf(phi)) - (b * sinf(t) * sinf(phi * sgn(sinf(t / 2.0f))));
            float y = (a * cosf(t) * sinf(phi * sgn(sin(t / 2.0f)))) + (b * sinf(t) * cosf(phi));

            PolarCoordinate p = cartesianToPolar({ x, y });
            return { p.r, p.theta };
        }
        case PositionPattern::ORBIT: {
            float radius = positionModA;

            const float timeScale = 3.0f;

            float r = radius;
            float theta = t * timeScale;
            return { r, theta };
        }
        case PositionPattern::SPIRAL: {
            float swirliness = juce::jmap(positionModA, 1.0f, 10.0f);

            float r = sinf(0.1f * t);
            float theta = swirliness * t;
            return { r, theta };
        }
        case PositionPattern::FLORAL: {
            float p = floorf(juce::jmap(positionModA, 1.0f, 10.0f));
            float q = floorf(juce::jmap(positionModB, 1.0f, 10.0f));

            float r = sinf(p * t);
            float theta = q * t;
            return { r, theta };
        }
        case PositionPattern::LISSAJOUS: {
            float p = floorf(juce::jmap(positionModA, 1.0f, 10.0f));
            float q = floorf(juce::jmap(positionModB, 1.0f, 10.0f));

            const float phi = juce::MathConstants<float>::halfPi;

            float r = sinf(p * t);
            float theta = phi * sinf((q * t) + phi);
            return { r, theta };
        }

        // Stochastic, N/A
        case PositionPattern::RANDOM_DISCRETE:
        case PositionPattern::RANDOM_WALK: return { 0.0f, 0.0f };
        default: return { 0.0f, 0.0f };
    }
}

PolarCoordinate MotionController::computePosition(PositionParameters positionParameters, PositionState& positionState, float t) {
    PolarCoordinate position;
    switch (positionParameters.positionPattern) {
        case PositionPattern::RANDOM_DISCRETE: position = randomDiscrete(positionParameters, positionState); break;
        case PositionPattern::RANDOM_WALK: position = randomWalk(positionParameters, positionState); break;
        default: position = computePositionParametric(positionParameters, t);
    }

    position.theta = wrapAngle(position.theta);

    return position;
}

void MotionController::computeFieldParametric(FieldParameters fieldParameters, std::vector<PolarCoordinate>& outputCoordinates, float t) {
    int fieldCount = fieldParameters.fieldCount;
    float fieldModA = fieldParameters.fieldModA,
          fieldModB = fieldParameters.fieldModB;

    switch (fieldParameters.fieldPattern) {
        case FieldPattern::MANUAL: {
            float r = fieldModA;
            float theta = fieldModB * juce::MathConstants<float>::twoPi;
            outputCoordinates[fieldParameters.fieldSelect] = { r, theta };
            break;
        }
        case FieldPattern::RING: {
            float radius = fieldModA;
            float offset = juce::jmap(fieldModB, 0.0f, juce::MathConstants<float>::twoPi);

            const float angleStep = juce::MathConstants<float>::twoPi / fieldCount;

            for (int coord = 0; coord < fieldCount; ++coord) {
                float r = radius;
                float theta = (static_cast<float>(coord) * angleStep) + offset + t;
                outputCoordinates[coord] = { r, theta };
            } 
            break;
        }
        case FieldPattern::ORBITS: {
            float radius = fieldModA;
            float bias = juce::jmap(fieldModB, 2.0f, 0.125f);

            const float angleStep = juce::MathConstants<float>::twoPi / fieldCount;
            const float timeScale = 2.0f;

            for (int coord = 0; coord < fieldCount; ++coord) {
                float direction = (coord % 2 == 0) ? 1.0f : -1.0f;
                float normIndex = (coord + 1.0f) / fieldCount;
                float velocity = direction * std::powf(normIndex, bias);

                float r = normIndex * radius;
                float theta = (static_cast<float>(coord) * angleStep) + (t * velocity * timeScale);
                outputCoordinates[coord] = { r, theta };
            } 
            break;
        }
        // Stochastic, N/A
        case FieldPattern::RANDOM_DISCRETE:
        case FieldPattern::RANDOM_WALK: 
        default: return;
    }
}

void MotionController::computeField(FieldParameters fieldParameters, FieldState& fieldState, float t) {
    auto applyStochastic = [&](auto&& function) {
        PositionParameters coordinateParameters { 
            PositionPattern::MANUAL, // Irrelevant
            fieldParameters.fieldRate, 
            fieldParameters.fieldModA, 
            fieldParameters.fieldModB 
        };
        for (int coord = 0; coord < fieldParameters.fieldCount; ++coord)
            fieldState.nextCoordinates[coord] = (function(coordinateParameters, fieldState.coordinateStates[coord]));
    };

    switch (fieldParameters.fieldPattern) {
        case FieldPattern::RANDOM_DISCRETE: { applyStochastic(randomDiscrete); break; }
        case FieldPattern::RANDOM_WALK: { applyStochastic(randomWalk); break; }
        default: computeFieldParametric(fieldParameters, fieldState.nextCoordinates, t);
    }

    // Wrap angle
    auto& nextCoordinates = fieldState.nextCoordinates;
    for (int coord = 0; coord < fieldParameters.fieldCount; ++coord)
        nextCoordinates[coord].theta = wrapAngle(nextCoordinates[coord].theta);
}

void MotionController::updatePosition() {
    PolarCoordinate currentPosition = polarMap->getPosition();
    positionState.currentPosition = currentPosition;

    PolarCoordinate nextPosition = computePosition(positionParameters, positionState, *positionTime);

    if (currentPosition == nextPosition) positionUpdated = false;
    else { polarMap->setPosition(nextPosition); positionUpdated = true; }
}

void MotionController::updateField() {
    const int fieldCount = fieldParameters.fieldCount;
    fieldState.coordinateStates.resize(fieldCount);
    fieldState.nextCoordinates.resize(fieldCount);

    // Bootstrap coordinates vector
    if (polarMap->getCoordinateCount() != fieldCount) {
        std::vector<PolarCoordinate> init(fieldCount, { 0.0f, 0.0f });
        polarMap->setCoordinates(init, true);
    }

    // Get current field coordinates
    const auto& currentCoordinates = polarMap->getCoordinates();
    for (int coord = 0; coord < fieldCount; coord++) 
        fieldState.coordinateStates[coord].currentPosition = currentCoordinates[coord];

    // Compute next field coordinates in-place
    computeField(fieldParameters, fieldState, *fieldTime);

    auto& nextCoordinates = fieldState.nextCoordinates;
    if (currentCoordinates == nextCoordinates) fieldUpdated = false;
    else { polarMap->setCoordinates(nextCoordinates); fieldUpdated = true; }
}

void MotionController::setPolarMap(PolarMap* nPolarMap) { polarMap = nPolarMap; }
void MotionController::setPositionTimer(float* nPT) { positionTime = nPT; }
void MotionController::setFieldTimer(float* nFT) { fieldTime = nFT; }
void MotionController::setPositionParameters(PositionParameters nPositionParameters) { positionParameters = nPositionParameters; }
void MotionController::setFieldParameters(FieldParameters nFieldParameters) { fieldParameters = nFieldParameters; }

bool MotionController::hasPositionUpdated() const { return positionUpdated; }
bool MotionController::hasFieldUpdated() const { return fieldUpdated; }