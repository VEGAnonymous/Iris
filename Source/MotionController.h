#pragma once

#include "Defines.h"
#include "PolarMap.h"

class MotionController {
private:
	PolarMap* polarMap;
	float* t;

	MotionPattern motionPattern = MotionPattern::LISSAJOUS;
	float motionRate = 0.5f, motionModA = 0.5f, motionModB = 0.5f;
	PolarCoordinate randomTarget { 0.0f, 0.0f };
	bool updated = false;

public:
	MotionController(PolarMap* initMap, float* initT);
	~MotionController() = default;

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

