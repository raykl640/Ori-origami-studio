// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : curves          Owner : Matthew Wachira (Colour, shading & curves)
//  File   : Curves.h
//  Parametric curves:
//    * Bezier curves - de Casteljau (any degree), Bernstein form (cubic),
//                      adaptive subdivision (improvement)
//    * Splines       - Catmull-Rom (interpolating), uniform cubic B-spline
//                      (approximating), cubic Hermite segments
//  In ORI curves draw the curved fold arrows of origami diagrams and the
//  smooth paths that fold animations follow.
// ============================================================================
#pragma once
#include "../core/Image.h"
#include "../math/Vec2.h"
#include "../raster/Lines.h"
#include <vector>

namespace ori {

// ---- Bezier -------------------------------------------------------------
Vec2 deCasteljau(const std::vector<Vec2>& ctrl, double t);
// Every intermediate level of the de Casteljau construction at t
// (level 0 = control polygon, last level = the single curve point).
std::vector<std::vector<Vec2>> deCasteljauLevels(const std::vector<Vec2>& ctrl, double t);
// Cubic Bezier via Bernstein polynomials: B(t) = sum C(3,i) t^i (1-t)^(3-i) P_i
Vec2 bezierCubicBernstein(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, double t);
// Uniformly sampled polyline (segments + 1 points).
std::vector<Vec2> bezierCurve(const std::vector<Vec2>& ctrl, int segments = 64);
// Adaptive subdivision: split until the control polygon is flat within `tolerance` px.
std::vector<Vec2> bezierAdaptive(const std::vector<Vec2>& ctrl, double tolerance = 0.25);

// ---- Splines ------------------------------------------------------------
// Catmull-Rom: passes through every point; tangent at P_i = (P_{i+1} - P_{i-1}) / 2.
std::vector<Vec2> catmullRom(const std::vector<Vec2>& pts, int samplesPerSegment = 24, bool closed = false);
// Uniform cubic B-spline: C2-continuous, approximates (does not touch) the control points.
std::vector<Vec2> bSplineUniform(const std::vector<Vec2>& ctrl, int samplesPerSegment = 24, bool closed = false);
// Cubic Hermite segment from p0 to p1 with tangents m0, m1.
Vec2 hermite(const Vec2& p0, const Vec2& m0, const Vec2& p1, const Vec2& m1, double t);

// ---- easing ---------------------------------------------------------------
// CSS-style timing curve: cubic Bezier through (0,0), (x1,y1), (x2,y2), (1,1).
// Solves x(u) = t for u by bisection and returns y(u). Drives fold animation.
double cubicBezierEase(double t, double x1, double y1, double x2, double y2);
inline double easeInOut(double t) { return cubicBezierEase(t, 0.42, 0.0, 0.58, 1.0); }

// ---- drawing ------------------------------------------------------------
void drawCurve(Image& img, const std::vector<Vec2>& polyline, const Stroke& s);
void drawControlPolygon(Image& img, const std::vector<Vec2>& ctrl, Color c);

enum class ArrowHead { Filled, Hollow };
// Straight-shafted arrow head at `tip` pointing along `dir`.
void drawArrowHead(Image& img, const Vec2& tip, const Vec2& dir, double size, Color c, ArrowHead kind);
// Curved origami-diagram arrow from `from` to `to`, bowed sideways by `bulge`
// (fraction of the chord length; negative bows the other way).
// Valley-fold arrows use a filled head, mountain-fold arrows a hollow head.
void drawFoldArrow(Image& img, const Vec2& from, const Vec2& to, double bulge, Color c,
                   ArrowHead kind = ArrowHead::Filled, int width = 2);

} // namespace ori
