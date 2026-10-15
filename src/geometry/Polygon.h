// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : geometry        Owner : Evans Mwendwa (Crease pattern editor)
//  File   : Polygon.h
//  Polygon representation: an ordered vertex list (implicitly closed - the
//  last vertex connects back to the first). Paper sheets and folded panels are
//  all Polygons.
// ============================================================================
#pragma once
#include "../math/Mat3.h"
#include "../math/Vec2.h"
#include "Line.h"
#include <array>
#include <vector>

namespace ori {

struct BBox { Vec2 min, max; double width() const { return max.x - min.x; } double height() const { return max.y - min.y; } };

class Polygon {
public:
    std::vector<Vec2> v;   // vertices in order

    Polygon() = default;
    explicit Polygon(std::vector<Vec2> verts) : v(std::move(verts)) {}

    // ---- construction helpers -------------------------------------------
    static Polygon rectangle(double x, double y, double w, double h);
    static Polygon regular(int sides, const Vec2& centre, double radius, double startAngle = 0.0);

    // ---- representation queries -----------------------------------------
    std::size_t size() const { return v.size(); }
    bool empty() const { return v.size() < 3; }
    Segment edge(std::size_t i) const { return {v[i], v[(i + 1) % v.size()]}; }
    std::vector<Segment> edges() const;

    // ---- operations ------------------------------------------------------
    double signedArea() const;          // shoelace formula
    double area() const;
    double perimeter() const;
    Vec2 centroid() const;              // area-weighted centroid
    BBox bounds() const;
    bool isConvex() const;
    bool isCounterClockwise() const;    // in a y-up frame (signed area > 0)
    bool contains(const Vec2& q) const; // even-odd ray casting
    Polygon reversed() const;
    Polygon transformed(const Mat3& M) const;
    Polygon translated(const Vec2& t) const { return transformed(Mat3::translate(t)); }

    // Ear-clipping triangulation of a simple polygon.
    std::vector<std::array<Vec2, 3>> triangulate() const;
};

// Convex hull of a point set (Andrew's monotone chain).
Polygon convexHull(std::vector<Vec2> pts);

} // namespace ori
