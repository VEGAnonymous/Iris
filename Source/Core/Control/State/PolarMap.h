#pragma once

#include "Core/Defines.h"

#include <vector>

class PolarMap {
private:
	PolarCoordinate position = { 0.0f, 0.0f };
	std::vector<PolarCoordinate> coordinates {}; // Locations
	std::vector<PolarCoordinate> relatives {}; // Distance-directions from position to each location

public:
	PolarMap(PolarCoordinate initPos = {0.0f, 0.0f});
	~PolarMap() = default;

	void setPosition(PolarCoordinate pos);
	void setCoordinate(int index, PolarCoordinate coordinate, bool setRelative = true);
	void setCoordinates(std::vector<PolarCoordinate> coords, bool setRelatives = true);

	PolarCoordinate getPosition() const;
	PolarCoordinate getCoordinate(int index) const;
	std::vector<PolarCoordinate> getCoordinates() const;
	PolarCoordinate getRelative(int index) const;
	std::vector<PolarCoordinate> getRelatives() const;
	int getCoordinateCount() const;

	PolarCoordinate computeRelative(PolarCoordinate p1, PolarCoordinate p2, Axis reference = Axis::Y_AXIS, bool computeAngle = true);
	void computeRelatives(Axis reference = Axis::Y_AXIS, bool computeAngles = true);
};