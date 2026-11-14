// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : math            Owner : Mohamed Bile (Fold transformation engine)
//  File   : Vec2.h
//  2D point / vector type used by every other module. Points on the paper,
//  crease endpoints, curve control points and fold directions are all Vec2.
//  Coordinates are doubles in image space (x to the right, y downwards).
// ============================================================================
#pragma once
#include <cmath>

namespace ori {

// Tolerance used for geometric comparisons (points on a line, zero lengths).
constexpr double kEps = 1e-9;
constexpr double kPi  = 3.14159265358979323846;

inline double degToRad(double deg) { return deg * kPi / 180.0; }
inline double radToDeg(double rad) { return rad * 180.0 / kPi; }

struct Vec2 {
    double x = 0.0, y = 0.0;

    constexpr Vec2() = default;
    constexpr Vec2(double x_, double y_) : x(x_), y(y_) {}

    constexpr Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator-() const { return {-x, -y}; }
    constexpr Vec2 operator*(double k) const { return {x * k, y * k}; }
    constexpr Vec2 operator/(double k) const { return {x / k, y / k}; }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(double k) { x *= k; y *= k; return *this; }
    Vec2& operator/=(double k) { x /= k; y /= k; return *this; }

    // Exact comparison; use nearlyEqual() for computed values.
    constexpr bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    constexpr bool operator!=(const Vec2& o) const { return !(*this == o); }

    constexpr double lengthSq() const { return x * x + y * y; }
    double length() const { return std::sqrt(lengthSq()); }

    // Unit vector in the same direction (the zero vector stays zero).
    Vec2 normalized() const {
        const double len = length();
        return len < kEps ? Vec2{} : Vec2{x / len, y / len};
    }

    // Perpendicular: this vector rotated by +90 degrees, (x, y) -> (-y, x).
    constexpr Vec2 perp() const { return {-y, x}; }

    // Rotated by `theta` radians about the origin.
    Vec2 rotated(double theta) const {
        const double c = std::cos(theta), s = std::sin(theta);
        return {c * x - s * y, s * x + c * y};
    }

    // Direction angle in radians, in (-pi, pi].
    double angle() const { return std::atan2(y, x); }
};

constexpr Vec2 operator*(double k, const Vec2& v) { return {v.x * k, v.y * k}; }

constexpr double dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }

// z-component of the 3D cross product. Its sign tells which side of the
// direction `a` the vector `b` lies on; its magnitude is the parallelogram area.
constexpr double cross(const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; }

constexpr double distanceSq(const Vec2& a, const Vec2& b) { return (a - b).lengthSq(); }
inline double distance(const Vec2& a, const Vec2& b) { return (a - b).length(); }

// Linear interpolation: t = 0 gives a, t = 1 gives b.
constexpr Vec2 lerp(const Vec2& a, const Vec2& b, double t) { return a + (b - a) * t; }

// Component-wise comparison with a tolerance.
inline bool nearlyEqual(const Vec2& a, const Vec2& b, double eps = 1e-6) {
    return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps;
}

} // namespace ori
