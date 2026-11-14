// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : raster          Owner : Mathew Nguithi (Rasterizer)
//  File   : Clipping.cpp
// ============================================================================
#include "Clipping.h"
#include <algorithm>
#include <cmath>

namespace ori {

// ------------------------------------------------------ Cohen-Sutherland --
int outCode(const Vec2& p, const ClipRect& r) {
    int code = kInside;
    if (p.x < r.xmin) code |= kLeft;
    else if (p.x > r.xmax) code |= kRight;
    if (p.y < r.ymin) code |= kTop;
    else if (p.y > r.ymax) code |= kBottom;
    return code;
}

bool clipCohenSutherland(Vec2& a, Vec2& b, const ClipRect& r) {
    int ca = outCode(a, r), cb = outCode(b, r);
    for (int guard = 0; guard < 16; ++guard) {
        if (!(ca | cb)) return true;          // both inside: trivial accept
        if (ca & cb) return false;            // both share an outside region: trivial reject
        const int out = ca ? ca : cb;         // an endpoint that is outside
        Vec2 p;
        const Vec2 d = b - a;
        if (out & kTop)         { p.x = a.x + d.x * (r.ymin - a.y) / d.y; p.y = r.ymin; }
        else if (out & kBottom) { p.x = a.x + d.x * (r.ymax - a.y) / d.y; p.y = r.ymax; }
        else if (out & kRight)  { p.y = a.y + d.y * (r.xmax - a.x) / d.x; p.x = r.xmax; }
        else                    { p.y = a.y + d.y * (r.xmin - a.x) / d.x; p.x = r.xmin; }
        if (out == ca) { a = p; ca = outCode(a, r); }
        else           { b = p; cb = outCode(b, r); }
    }
    return false;
}

// ---------------------------------------------------------- Liang-Barsky --
bool clipLiangBarsky(Vec2& a, Vec2& b, const ClipRect& r) {
    const double dx = b.x - a.x, dy = b.y - a.y;
    // Point P(t) = a + t d is inside when p_k t <= q_k for k = left, right, top, bottom.
    const double p[4] = {-dx, dx, -dy, dy};
    const double q[4] = {a.x - r.xmin, r.xmax - a.x, a.y - r.ymin, r.ymax - a.y};
    double t0 = 0.0, t1 = 1.0;
    for (int k = 0; k < 4; ++k) {
        if (p[k] == 0.0) {
            if (q[k] < 0) return false;        // parallel and outside this boundary
            continue;
        }
        const double t = q[k] / p[k];
        if (p[k] < 0) { if (t > t1) return false; t0 = std::max(t0, t); }   // entering
        else          { if (t < t0) return false; t1 = std::min(t1, t); }   // leaving
    }
    const Vec2 a0 = a;
    a = a0 + Vec2{dx, dy} * t0;
    b = a0 + Vec2{dx, dy} * t1;
    return true;
}

// ------------------------------------------------------------ Cyrus-Beck --
namespace {

double signedArea(const std::vector<Vec2>& poly) {
    double s = 0;
    for (std::size_t i = 0; i < poly.size(); ++i) s += cross(poly[i], poly[(i + 1) % poly.size()]);
    return 0.5 * s;
}

// Parametric range [t0, t1] of a + t (b - a) inside the convex window.
bool cyrusBeckRange(const Vec2& a, const Vec2& b, const std::vector<Vec2>& window, double& t0, double& t1) {
    if (window.size() < 3) return false;
    const double orient = signedArea(window) >= 0 ? 1.0 : -1.0;
    const Vec2 d = b - a;
    for (std::size_t i = 0; i < window.size(); ++i) {
        const Vec2& e0 = window[i];
        const Vec2& e1 = window[(i + 1) % window.size()];
        // Inward normal of edge e0 -> e1.
        const Vec2 n = (e1 - e0).perp() * orient;
        const double num = dot(n, a - e0);    // > 0 when a is inside this edge
        const double den = dot(n, d);
        if (std::fabs(den) < 1e-12) {
            if (num < 0) return false;         // parallel and outside
            continue;
        }
        const double t = -num / den;
        if (den > 0) t0 = std::max(t0, t);     // entering
        else         t1 = std::min(t1, t);     // leaving
        if (t0 > t1) return false;
    }
    return true;
}

} // namespace

bool clipCyrusBeck(Vec2& a, Vec2& b, const std::vector<Vec2>& window) {
    double t0 = 0.0, t1 = 1.0;
    if (!cyrusBeckRange(a, b, window, t0, t1)) return false;
    const Vec2 a0 = a, d = b - a;
    a = a0 + d * t0;
    b = a0 + d * t1;
    return true;
}

bool clipLineToConvex(const Vec2& a, const Vec2& b, const std::vector<Vec2>& window, Vec2& outA, Vec2& outB) {
    double t0 = -1e9, t1 = 1e9;
    if ((b - a).lengthSq() < 1e-18) return false;
    if (!cyrusBeckRange(a, b, window, t0, t1) || t1 - t0 < 1e-12) return false;
    outA = a + (b - a) * t0;
    outB = a + (b - a) * t1;
    return true;
}

// --------------------------------------------------- Sutherland-Hodgman --
namespace {

std::vector<Vec2> clipAgainstEdge(const std::vector<Vec2>& poly, const Vec2& e0, const Vec2& e1, double orient) {
    std::vector<Vec2> out;
    if (poly.empty()) return out;
    auto side = [&](const Vec2& p) { return cross(e1 - e0, p - e0) * orient; };   // >= 0 inside
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const Vec2& s = poly[i];
        const Vec2& p = poly[(i + 1) % poly.size()];
        const double ss = side(s), sp = side(p);
        const bool sIn = ss >= 0, pIn = sp >= 0;
        if (sIn && pIn) {
            out.push_back(p);                                   // in -> in : keep p
        } else if (sIn && !pIn) {
            out.push_back(lerp(s, p, ss / (ss - sp)));          // in -> out: intersection
        } else if (!sIn && pIn) {
            out.push_back(lerp(s, p, ss / (ss - sp)));          // out -> in: intersection + p
            out.push_back(p);
        }                                                       // out -> out: nothing
    }
    return out;
}

} // namespace

std::vector<std::vector<Vec2>> sutherlandHodgmanStages(const std::vector<Vec2>& subject, const std::vector<Vec2>& window) {
    std::vector<std::vector<Vec2>> stages;
    if (window.size() < 3) return stages;
    const double orient = signedArea(window) >= 0 ? 1.0 : -1.0;
    std::vector<Vec2> cur = subject;
    for (std::size_t i = 0; i < window.size(); ++i) {
        cur = clipAgainstEdge(cur, window[i], window[(i + 1) % window.size()], orient);
        stages.push_back(cur);
    }
    return stages;
}

std::vector<Vec2> clipSutherlandHodgman(const std::vector<Vec2>& subject, const std::vector<Vec2>& window) {
    const auto stages = sutherlandHodgmanStages(subject, window);
    return stages.empty() ? std::vector<Vec2>{} : stages.back();
}

} // namespace ori
