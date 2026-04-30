#include "Core/Control/State/PolarMap.h"

/* PUBLIC */

PolarMap::PolarMap(PolarCoordinate initPos) : position(initPos) { }

void PolarMap::setPosition(PolarCoordinate pos) { position = pos; }

void PolarMap::setCoordinate(int index, PolarCoordinate coordinate, bool setRelative) {
    if (index < 0) return;

    PolarCoordinate rel = setRelative ? computeRelative(position, coordinate) : PolarCoordinate{0.0f, 0.0f};
    if (index >= coordinates.size()) {
        coordinates.emplace_back(coordinate); 
        relatives.emplace_back(rel); 
    } else { 
        coordinates[index] = coordinate; 
        relatives[index] = rel; 
    }
}

void PolarMap::setCoordinates(std::vector<PolarCoordinate> coords, bool setRelatives) { 
    coordinates = coords;
    if (setRelatives) computeRelatives();
}

PolarCoordinate PolarMap::getPosition() const { return position; }

PolarCoordinate PolarMap::getCoordinate(int index) const {
    if (index < 0 || index >= coordinates.size()) return { 0.0f, 0.0f };
    else return coordinates[index];
}

std::vector<PolarCoordinate> PolarMap::getCoordinates() const { return coordinates; }

PolarCoordinate PolarMap::getRelative(int index) const { 
    if (index < 0 || index >= relatives.size()) return { 0.0f, 0.0f };
    else return relatives[index];
}

std::vector<PolarCoordinate> PolarMap::getRelatives() const { return relatives; }

int PolarMap::getCoordinateCount() const { return static_cast<int>(coordinates.size()); }

PolarCoordinate PolarMap::computeRelative(PolarCoordinate p1, PolarCoordinate p2, Axis reference, bool computeAngle) {
    // TODO: Optimize with approximations
    float d = 0.0f, phi = 0.0f;
    float dTheta = p2.theta - p1.theta;
    d = sqrtf((p1.r * p1.r) + (p2.r * p2.r) - (2 * p1.r * p2.r * cosf(dTheta)));

    if (computeAngle) {
        if (reference == Axis::X_AXIS)
            phi = atan2f((p2.r * sinf(p2.theta)) - (p1.r * sinf(p1.theta)), (p2.r * cosf(p2.theta)) - (p1.r * cosf(p1.theta)));
        else // Y_AXIS
            phi = atan2f((p2.r * cosf(p2.theta)) - (p1.r * cosf(p1.theta)), (p2.r * sinf(p2.theta)) - (p1.r * sinf(p1.theta)));
    }

    return { d, phi };
}

void PolarMap::computeRelatives(Axis reference, bool computeAngles) {
    relatives.resize(coordinates.size());
    for (int coord = 0; coord < coordinates.size(); ++coord)
        relatives[coord] = computeRelative(position, coordinates[coord], reference, computeAngles);
}