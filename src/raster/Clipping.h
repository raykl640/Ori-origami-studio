// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : raster          Owner : Mathew Nguithi (Rasterizer)
//  File   : Clipping.h
//  Line clipping:
//    * Cohen-Sutherland - 4-bit region codes, trivial accept / reject
//    * Liang-Barsky     - parametric form, p_k / q_k inequalities
//    * Cyrus-Beck       - parametric clipping against any convex window
//                         (a diamond sheet, a folded panel, ...)
//  Polygon clipping:
//    * Sutherland-Hodgman - re-entrant clipping against each window edge
// ============================================================================
#pragma once
#include "../math/Vec2.h"
#include <vector>

namespace ori {

struct ClipRect {
    double xmin = 0, ymin = 0, xmax = 0, ymax = 0;
    std::vector<Vec2> polygon() const { return {{xmin, ymin}, {xmax, ymin}, {xmax, ymax}, {xmin, ymax}}; }
};

// Region code bits (y grows downwards: TOP is y < ymin).
enum OutCode : int { kInside = 0, kLeft = 1, kRight = 2, kTop = 4, kBottom = 8 };
int outCode(const Vec2& p, const ClipRect& r);

// Each clipper shortens a -> b in place and returns false when nothing is left.
bool clipCohenSutherland(Vec2& a, Vec2& b, const ClipRect& r);
bool clipLiangBarsky(Vec2& a, Vec2& b, const ClipRect& r);
// `window` must be convex; either vertex order is accepted.
bool clipCyrusBeck(Vec2& a, Vec2& b, const std::vector<Vec2>& window);
// Same, for an infinite line through a and b (the result is the chord of the window).
bool clipLineToConvex(const Vec2& a, const Vec2& b, const std::vector<Vec2>& window, Vec2& outA, Vec2& outB);

// Sutherland-Hodgman: clip `subject` (any simple polygon) by a convex window.
std::vector<Vec2> clipSutherlandHodgman(const std::vector<Vec2>& subject, const std::vector<Vec2>& window);
// The intermediate polygon after each window edge (stages[i] = after edge i).
std::vector<std::vector<Vec2>> sutherlandHodgmanStages(const std::vector<Vec2>& subject, const std::vector<Vec2>& window);
inline std::vector<Vec2> clipPolygonToRect(const std::vector<Vec2>& subject, const ClipRect& r) {
    return clipSutherlandHodgman(subject, r.polygon());
}

} // namespace ori
