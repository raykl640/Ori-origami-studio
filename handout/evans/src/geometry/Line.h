// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : geometry        Owner : Evans Mwendwa (Crease pattern editor)
//  File   : Line.h
//  Line / segment representation and the line operations a crease editor
//  needs: side-of-line tests, intersections, projection, distance, reflection.
// ============================================================================
#pragma once
#include "../math/Mat3.h"
#include "../math/Vec2.h"
#include <optional>

namespace ori {

// Finite line segment from a to b (a crease, a polygon edge, ...).
struct Segment {
    Vec2 a, b;
    Vec2 direction() const { return b - a; }
    double length() const { return distance(a, b); }
    Vec2 midpoint() const { return (a + b) * 0.5; }
    Vec2 at(double t) const { return a + (b - a) * t; }   // parametric form
};

// Infinite line through point p with direction d (a fold axis).
struct Line {
    Vec2 p, d;
    static Line through(const Vec2& a, const Vec2& b) { return {a, b - a}; }

    // Signed side test: >0 on one side, <0 on the other, 0 on the line.
    double side(const Vec2& q) const { return cross(d, q - p); }
    // Implicit form  A x + B y + C = 0  (normalised so that |(A,B)| = 1)
    void implicit(double& A, double& B, double& C) const;
    double distanceTo(const Vec2& q) const;
    Vec2 project(const Vec2& q) const;            // closest point on the line
    Vec2 reflect(const Vec2& q) const;            // mirror image of q
    Mat3 reflectionMatrix() const { return Mat3::reflectLine(p, p + d); }
};

// Intersection of two infinite lines (nullopt when parallel).
std::optional<Vec2> intersect(const Line& l1, const Line& l2);

// Intersection of two segments (nullopt when they do not meet).
std::optional<Vec2> intersect(const Segment& s1, const Segment& s2);

// Intersection of an infinite line with a segment.
std::optional<Vec2> intersect(const Line& l, const Segment& s);

// Shortest distance from q to the segment.
double distanceToSegment(const Vec2& q, const Segment& s);

// Angle of a segment in degrees, measured from the +x axis.
double angleDegrees(const Segment& s);

} // namespace ori
