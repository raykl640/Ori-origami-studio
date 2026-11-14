// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : geometry        Owner : Evans Mwendwa (Crease pattern editor)
//  File   : Line.cpp
// ============================================================================
#include "Line.h"
#include <algorithm>
#include <cmath>

namespace ori {

void Line::implicit(double& A, double& B, double& C) const {
    const Vec2 n = d.perp().normalized();      // unit normal
    A = n.x; B = n.y; C = -(A * p.x + B * p.y);
}

double Line::distanceTo(const Vec2& q) const {
    double A, B, C; implicit(A, B, C);
    return std::fabs(A * q.x + B * q.y + C);
}

Vec2 Line::project(const Vec2& q) const {
    const double t = dot(q - p, d) / d.lengthSq();
    return p + d * t;
}

Vec2 Line::reflect(const Vec2& q) const {
    const Vec2 foot = project(q);
    return foot * 2.0 - q;                     // q' = 2 * foot - q
}

std::optional<Vec2> intersect(const Line& l1, const Line& l2) {
    const double denom = cross(l1.d, l2.d);
    if (std::fabs(denom) < kEps) return std::nullopt;   // parallel
    const double t = cross(l2.p - l1.p, l2.d) / denom;
    return l1.p + l1.d * t;
}

std::optional<Vec2> intersect(const Segment& s1, const Segment& s2) {
    const Vec2 r = s1.direction(), s = s2.direction();
    const double denom = cross(r, s);
    if (std::fabs(denom) < kEps) return std::nullopt;   // parallel / collinear
    const Vec2 qp = s2.a - s1.a;
    const double t = cross(qp, s) / denom;              // parameter on s1
    const double u = cross(qp, r) / denom;              // parameter on s2
    const double e = 1e-9;
    if (t < -e || t > 1 + e || u < -e || u > 1 + e) return std::nullopt;
    return s1.at(t);
}

std::optional<Vec2> intersect(const Line& l, const Segment& s) {
    const Vec2 r = s.direction();
    const double denom = cross(l.d, r);
    if (std::fabs(denom) < kEps) return std::nullopt;
    const double u = cross(s.a - l.p, l.d) / denom;     // parameter on the segment
    // Note sign: solve l.p + t l.d = s.a + u r  ->  u = cross(l.p - s.a, l.d) / cross(r, l.d)
    if (u < -1e-9 || u > 1 + 1e-9) return std::nullopt;
    return s.at(u);
}

double distanceToSegment(const Vec2& q, const Segment& s) {
    const Vec2 d = s.direction();
    const double len2 = d.lengthSq();
    if (len2 < kEps) return distance(q, s.a);
    const double t = std::clamp(dot(q - s.a, d) / len2, 0.0, 1.0);
    return distance(q, s.at(t));
}

double angleDegrees(const Segment& s) {
    const Vec2 d = s.direction();
    return std::atan2(d.y, d.x) * 180.0 / kPi;
}

} // namespace ori
