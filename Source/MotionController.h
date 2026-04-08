#pragma once

#include "Defines.h"
#include "PolarMap.h"

class MotionController {
private:
	PolarMap* polarMap;
	float* t;

	struct MotionParameters {
		MotionPattern motionPattern = MotionPattern::LISSAJOUS;
		float motionRate = 0.5f;
		float motionModA = 0.5f, motionModB = 0.5f;
	};

	struct MotionState {
		PolarCoordinate currentPosition {0.0f, 0.0f};
		// Discrete
		CartesianCoordinate targetPosition {0.0f, 0.0f};
		bool hasTarget = false;
		// Walk
		CartesianCoordinate walkVelocity {0.0f, 0.0f};
	};

	MotionParameters motionParameters;
	MotionState motionState;
	bool updated = false;
public:
	MotionController(PolarMap* initMap, float* initT);
	~MotionController() = default;

	static PolarCoordinate computeParametric(MotionParameters motionParameters, float t);
	static PolarCoordinate computePosition(MotionParameters motionParameters, MotionState& motionState, float t);

	void updatePosition();
	void updateCoordinates();

	void setPolarMap(PolarMap* nPolarMap);
	void setTimer(float* nT);
	void setMotionPattern(MotionPattern nMotionPattern);
	void setMotionRate(float nMotionRate);
	void setMotionModA(float nMotionModA);
	void setMotionModB(float nMotionModB);

	PolarMap* getPolarMap() const;
	float* getTimer() const;
	MotionPattern getMotionPattern() const;
	float getMotionRate() const;
	float getMotionModA() const;
	float getMotionModB() const;
	bool hasUpdated() const;
};
