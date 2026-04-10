#pragma once

#include "Defines.h"
#include "PolarMap.h"

class MotionController {
private:
	PolarMap* polarMap;

	float* positionTime; 
	float* fieldTime;
	bool positionUpdated = false;
	bool fieldUpdated = false;

	// Position data
	struct PositionParameters {
		PositionPattern positionPattern = PositionPattern::LISSAJOUS;
		float positionRate = 0.5f;
		float positionModA = 0.5f;
		float positionModB = 0.5f;
	};

	struct PositionState {
		PolarCoordinate currentPosition {0.0f, 0.0f};
		// Random discrete
		CartesianCoordinate targetPosition {0.0f, 0.0f};
		bool hasTarget = false;
		// Random walk
		CartesianCoordinate walkVelocity {0.0f, 0.0f};
	};

	PositionParameters positionParameters;
	PositionState positionState;

	// Field data
	struct FieldParameters {
		int fieldCount = 0;
		int fieldSelect = 0;
		FieldPattern fieldPattern = FieldPattern::RING;
		float fieldRate = 0.5f;
		float fieldModA = 0.5f;
		float fieldModB = 0.5f;
	};

	struct FieldState {
		std::vector<PositionState> coordinateStates {};
		std::vector<PolarCoordinate> nextCoordinates {};
	};

	FieldParameters fieldParameters;
	FieldState fieldState;

	// Stochastic patterns
	static PolarCoordinate randomDiscrete(PositionParameters positionParameters, PositionState& positionState);
	static PolarCoordinate randomWalk(PositionParameters positionParameters, PositionState& positionState);
	
public:
	MotionController(PolarMap* map, float* positionTimer, float* fieldTimer);
	~MotionController() = default;

	static PolarCoordinate computePositionParametric(PositionParameters positionParameters, float t);
	static PolarCoordinate computePosition(PositionParameters positionParameters, PositionState& positionState, float t);

	static void computeFieldParametric(FieldParameters fieldParameters, std::vector<PolarCoordinate>& outputCoordinates, float t);
	static void computeField(FieldParameters fieldParameters, FieldState& fieldState, float t);

	void updatePosition();
	void updateField();

	void setPolarMap(PolarMap* nPolarMap);
	void setPositionTimer(float* nPT); 
	void setFieldTimer(float* nFT);

	void setPositionParameters(PositionParameters nPositionParameters);
	void setFieldParameters(FieldParameters nFieldParameters);

	bool hasPositionUpdated() const;
	bool hasFieldUpdated() const;
};
