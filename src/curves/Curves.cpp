// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : curves          Owner : Matthew Wachira (Colour, shading & curves)
//  File   : Curves.cpp
// ============================================================================
#include "Curves.h"
#include "../geometry/Line.h"
#include "../raster/Fill.h"
#include <algorithm>
#include <cmath>

namespace ori {

// ---------------------------------------------------------------- Bezier --
Vec2 deCasteljau(const std::vector<Vec2>& ctrl, double t) {
    if (ctrl.empty()) return {};
    std::vector<Vec2> pts = ctrl;
    // Repeated linear interpolation: n levels for a degree-n curve.
    for (std::size_t level = 1; level < ctrl.size(); ++level)
        for (std::size_t i = 0; i < ctrl.size() - level; ++i)
            pts[i] = lerp(pts[i], pts[i + 1], t);
    return pts[0];
}

std::vector<std::vector<Vec2>> deCasteljauLevels(const std::vector<Vec2>& ctrl, double t) {
    std::vector<std::vector<Vec2>> levels{ctrl};
    while (levels.back().size() > 1) {
        const auto& prev = levels.back();
        std::vector<Vec2> next;
        for (std::size_t i = 0; i + 1 < prev.size(); ++i) next.push_back(lerp(prev[i], prev[i + 1], t));
        levels.push_back(next);
    }
    return levels;
}

Vec2 bezierCubicBernstein(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, double t) {
    const double u = 1.0 - t;
    const double b0 = u * u * u, b1 = 3 * u * u * t, b2 = 3 * u * t * t, b3 = t * t * t;
    return p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3;
}

std::vector<Vec2> bezierCurve(const std::vector<Vec2>& ctrl, int segments) {
    std::vector<Vec2> out;
    for (int i = 0; i <= segments; ++i) out.push_back(deCasteljau(ctrl, double(i) / segments));
    return out;
}

// Flatness = max distance of inner control points from the chord.
static double flatness(const std::vector<Vec2>& c) {
    const Line chord = Line::through(c.front(), c.back());
    double d = 0;
    if (distance(c.front(), c.back()) < kEps) {
        for (const Vec2& p : c) d = std::max(d, distance(p, c.front()));
        return d;
    }
    for (std::size_t i = 1; i + 1 < c.size(); ++i) d = std::max(d, chord.distanceTo(c[i]));
    return d;
}

static void subdivide(const std::vector<Vec2>& c, double tol, int depth, std::vector<Vec2>& out) {
    if (depth > 16 || flatness(c) <= tol) { out.push_back(c.back()); return; }
    // Split at t = 0.5: the left half's control points are the first point of
    // every de Casteljau level, the right half's are the last points.
    const auto levels = deCasteljauLevels(c, 0.5);
    std::vector<Vec2> left, right;
    for (const auto& lv : levels) { left.push_back(lv.front()); right.push_back(lv.back()); }
    std::vector<Vec2> rightRev(right.rbegin(), right.rend());
    subdivide(left, tol, depth + 1, out);
    subdivide(rightRev, tol, depth + 1, out);
}

std::vector<Vec2> bezierAdaptive(const std::vector<Vec2>& ctrl, double tolerance) {
    std::vector<Vec2> out;
    if (ctrl.empty()) return out;
    out.push_back(ctrl.front());
    subdivide(ctrl, tolerance, 0, out);
    return out;
}

// --------------------------------------------------------------- Splines --
Vec2 hermite(const Vec2& p0, const Vec2& m0, const Vec2& p1, const Vec2& m1, double t) {
    const double t2 = t * t, t3 = t2 * t;
    const double h00 = 2 * t3 - 3 * t2 + 1, h10 = t3 - 2 * t2 + t;
    const double h01 = -2 * t3 + 3 * t2, h11 = t3 - t2;
    return p0 * h00 + m0 * h10 + p1 * h01 + m1 * h11;
}

std::vector<Vec2> catmullRom(const std::vector<Vec2>& p, int samples, bool closed) {
    std::vector<Vec2> out;
    const int n = int(p.size());
    if (n < 2) return p;
    auto at = [&](int i) {
        if (closed) return p[std::size_t(((i % n) + n) % n)];
        return p[std::size_t(std::clamp(i, 0, n - 1))];   // clamp ends (duplicate endpoints)
    };
    const int segs = closed ? n : n - 1;
    for (int i = 0; i < segs; ++i) {
        const Vec2 p0 = at(i - 1), p1 = at(i), p2 = at(i + 1), p3 = at(i + 2);
        const Vec2 m1 = (p2 - p0) * 0.5, m2 = (p3 - p1) * 0.5;   // Catmull-Rom tangents
        for (int s = 0; s < samples; ++s) out.push_back(hermite(p1, m1, p2, m2, double(s) / samples));
    }
    out.push_back(closed ? at(0) : p.back());
    return out;
}

std::vector<Vec2> bSplineUniform(const std::vector<Vec2>& c, int samples, bool closed) {
    std::vector<Vec2> out;
    const int n = int(c.size());
    if (n < 4 && !closed) return c;
    auto at = [&](int i) { return c[std::size_t(((i % n) + n) % n)]; };
    const int segs = closed ? n : n - 3;
    for (int i = 0; i < segs; ++i) {
        const Vec2 p0 = at(i), p1 = at(i + 1), p2 = at(i + 2), p3 = at(i + 3);
        for (int s = 0; s <= samples; ++s) {
            if (s == samples && i + 1 < segs) break;          // avoid duplicate joints
            const double t = double(s) / samples, t2 = t * t, t3 = t2 * t;
            // Uniform cubic B-spline basis (each weight / 6)
            const double b0 = (1 - t) * (1 - t) * (1 - t) / 6.0;
            const double b1 = (3 * t3 - 6 * t2 + 4) / 6.0;
            const double b2 = (-3 * t3 + 3 * t2 + 3 * t + 1) / 6.0;
            const double b3 = t3 / 6.0;
            out.push_back(p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3);
        }
    }
    return out;
}

// ---------------------------------------------------------------- easing --
double cubicBezierEase(double t, double x1, double y1, double x2, double y2) {
    t = std::clamp(t, 0.0, 1.0);
    auto bez = [](double a, double b, double u) {          // 1-D cubic with P0 = 0, P3 = 1
        const double v = 1 - u;
        return 3 * v * v * u * a + 3 * v * u * u * b + u * u * u;
    };
    double lo = 0, hi = 1, u = t;
    for (int i = 0; i < 40; ++i) {                          // x(u) is monotonic for x1,x2 in [0,1]
        u = 0.5 * (lo + hi);
        if (bez(x1, x2, u) < t) lo = u; else hi = u;
    }
    return bez(y1, y2, u);
}

// --------------------------------------------------------------- drawing --
void drawCurve(Image& img, const std::vector<Vec2>& poly, const Stroke& s) {
    drawPolyline(img, poly, s, false);
}

void drawControlPolygon(Image& img, const std::vector<Vec2>& ctrl, Color c) {
    drawPolyline(img, ctrl, Stroke::dotted(c), false);
    for (const Vec2& p : ctrl) {
        const int x = int(std::lround(p.x)), y = int(std::lround(p.y));
        img.fillRect(x - 3, y - 3, 7, 7, colors::White);
        img.strokeRect(x - 3, y - 3, 7, 7, c);
    }
}

void drawArrowHead(Image& img, const Vec2& tip, const Vec2& dir, double size, Color c, ArrowHead kind) {
    const Vec2 d = dir.normalized(), n = d.perp();
    const Vec2 base = tip - d * size;
    const Contour tri = {tip, base + n * (size * 0.55), base - n * (size * 0.55)};
    if (kind == ArrowHead::Filled) {
        fillPolygon(img, tri, c);
    } else {
        fillPolygon(img, tri, colors::White);
    }
    drawPolyline(img, tri, Stroke::solid(c, 2), true);
}

void drawFoldArrow(Image& img, const Vec2& from, const Vec2& to, double bulge, Color c,
                   ArrowHead kind, int width) {
    const Vec2 chord = to - from;
    const Vec2 n = chord.perp() * bulge;
    // Cubic Bezier: control points pushed sideways for a symmetric arc.
    const std::vector<Vec2> ctrl = {from, from + chord * 0.25 + n, from + chord * 0.75 + n, to};
    const auto pts = bezierAdaptive(ctrl, 0.3);
    // Stop the shaft short of the tip so the head sits cleanly on the curve.
    const double headSize = 9.0 + 2.0 * width;
    std::vector<Vec2> shaft;
    for (const Vec2& p : pts) if (distance(p, to) > headSize * 0.6) shaft.push_back(p);
    drawPolyline(img, shaft, Stroke::solid(c, width), false);
    const Vec2 tangent = to - deCasteljau(ctrl, 0.92);
    drawArrowHead(img, to, tangent, headSize, c, kind);
}

} // namespace ori
