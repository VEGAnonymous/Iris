#pragma once

#include "Defines.h"
#include "PolarMap.h"

class MotionController {
private:
	PolarMap* polarMap;
	float* t;

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
	bool updated = false;
public:
	MotionController(PolarMap* initMap, float* initT);
	~MotionController() = default;

	static PolarCoordinate computeParametric(PositionParameters positionParameters, float t);
	static PolarCoordinate computePosition(PositionParameters positionParameters, PositionState& positionState, float t);

	void updatePosition();
	void updateCoordinates();

	void setPolarMap(PolarMap* nPolarMap);
	void setTimer(float* nT);
	void setPositionParameters(PositionParameters nPositionParameters);
	void setPositionPattern(PositionPattern nPositionPattern);
	void setPositionRate(float nPositionRate);
	void setPositionModA(float nPositionModA);
	void setPositionModB(float nPositionModB);

	PolarMap* getPolarMap() const;
	float* getTimer() const;
	PositionPattern getPositionPattern() const;
	float getPositionRate() const;
	float getPositionModA() const;
	float getPositionModB() const;
	bool hasUpdated() const;
};
