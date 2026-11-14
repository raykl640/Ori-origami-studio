// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : geometry        Owner : Evans Mwendwa (Crease pattern editor)
//  File   : Polygon.cpp
// ============================================================================
#include "Polygon.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace ori {

Polygon Polygon::rectangle(double x, double y, double w, double h) {
    return Polygon({{x, y}, {x + w, y}, {x + w, y + h}, {x, y + h}});
}

Polygon Polygon::regular(int sides, const Vec2& c, double r, double start) {
    Polygon p;
    for (int i = 0; i < sides; ++i) {
        const double a = start + 2.0 * kPi * i / sides;
        p.v.push_back({c.x + r * std::cos(a), c.y + r * std::sin(a)});
    }
    return p;
}

std::vector<Segment> Polygon::edges() const {
    std::vector<Segment> e;
    for (std::size_t i = 0; i < v.size(); ++i) e.push_back(edge(i));
    return e;
}

double Polygon::signedArea() const {
    // Shoelace:  A = 1/2 * sum (x_i * y_{i+1} - x_{i+1} * y_i)
    double s = 0;
    for (std::size_t i = 0; i < v.size(); ++i) s += cross(v[i], v[(i + 1) % v.size()]);
    return 0.5 * s;
}

double Polygon::area() const { return std::fabs(signedArea()); }

double Polygon::perimeter() const {
    double p = 0;
    for (std::size_t i = 0; i < v.size(); ++i) p += distance(v[i], v[(i + 1) % v.size()]);
    return p;
}

Vec2 Polygon::centroid() const {
    const double A = signedArea();
    if (std::fabs(A) < kEps) {                  // degenerate: vertex average
        Vec2 c; for (auto& p : v) c += p;
        return v.empty() ? c : c / double(v.size());
    }
    Vec2 c;
    for (std::size_t i = 0; i < v.size(); ++i) {
        const Vec2& a = v[i]; const Vec2& b = v[(i + 1) % v.size()];
        c += (a + b) * cross(a, b);
    }
    return c / (6.0 * A);
}

BBox Polygon::bounds() const {
    const double inf = std::numeric_limits<double>::infinity();
    BBox b{{inf, inf}, {-inf, -inf}};
    for (auto& p : v) {
        b.min.x = std::min(b.min.x, p.x); b.min.y = std::min(b.min.y, p.y);
        b.max.x = std::max(b.max.x, p.x); b.max.y = std::max(b.max.y, p.y);
    }
    return b;
}

bool Polygon::isConvex() const {
    if (v.size() < 3) return false;
    int sign = 0;
    for (std::size_t i = 0; i < v.size(); ++i) {
        const Vec2& a = v[i];
        const Vec2& b = v[(i + 1) % v.size()];
        const Vec2& c = v[(i + 2) % v.size()];
        const double z = cross(b - a, c - b);
        if (std::fabs(z) < 1e-9) continue;       // collinear vertices are fine
        const int s = z > 0 ? 1 : -1;
        if (sign == 0) sign = s;
        else if (s != sign) return false;        // turn direction changed
    }
    return true;
}

bool Polygon::isCounterClockwise() const { return signedArea() > 0; }

bool Polygon::contains(const Vec2& q) const {
    // Cast a ray to +x and count edge crossings (even-odd rule).
    bool inside = false;
    for (std::size_t i = 0, j = v.size() - 1; i < v.size(); j = i++) {
        const Vec2& a = v[i]; const Vec2& b = v[j];
        if ((a.y > q.y) != (b.y > q.y)) {
            const double xCross = a.x + (q.y - a.y) * (b.x - a.x) / (b.y - a.y);
            if (q.x < xCross) inside = !inside;
        }
    }
    return inside;
}

Polygon Polygon::reversed() const { return Polygon(std::vector<Vec2>(v.rbegin(), v.rend())); }

Polygon Polygon::transformed(const Mat3& M) const {
    Polygon p; p.v.reserve(v.size());
    for (auto& q : v) p.v.push_back(M.apply(q));
    return p;
}

// ---------------------------------------------------------------- ear clip --
static bool pointInTriangle(const Vec2& p, const Vec2& a, const Vec2& b, const Vec2& c) {
    const double d1 = cross(b - a, p - a), d2 = cross(c - b, p - b), d3 = cross(a - c, p - c);
    const bool neg = d1 < 0 || d2 < 0 || d3 < 0, pos = d1 > 0 || d2 > 0 || d3 > 0;
    return !(neg && pos);
}

std::vector<std::array<Vec2, 3>> Polygon::triangulate() const {
    std::vector<std::array<Vec2, 3>> tris;
    if (v.size() < 3) return tris;
    std::vector<Vec2> pts = isCounterClockwise() ? v : reversed().v;   // work in CCW order
    std::size_t guard = 0;
    while (pts.size() > 3 && guard++ < 10000) {
        bool clipped = false;
        for (std::size_t i = 0; i < pts.size(); ++i) {
            const Vec2& prev = pts[(i + pts.size() - 1) % pts.size()];
            const Vec2& cur = pts[i];
            const Vec2& next = pts[(i + 1) % pts.size()];
            if (cross(cur - prev, next - cur) <= 0) continue;          // reflex vertex
            bool ear = true;
            for (std::size_t k = 0; k < pts.size() && ear; ++k) {
                if (pts[k] == prev || pts[k] == cur || pts[k] == next) continue;
                if (pointInTriangle(pts[k], prev, cur, next)) ear = false;
            }
            if (!ear) continue;
            tris.push_back({prev, cur, next});
            pts.erase(pts.begin() + std::ptrdiff_t(i));
            clipped = true;
            break;
        }
        if (!clipped) break;                                            // not simple
    }
    if (pts.size() == 3) tris.push_back({pts[0], pts[1], pts[2]});
    return tris;
}

// ------------------------------------------------------------- convex hull --
Polygon convexHull(std::vector<Vec2> pts) {
    std::sort(pts.begin(), pts.end(), [](const Vec2& a, const Vec2& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    });
    if (pts.size() < 3) return Polygon(pts);
    std::vector<Vec2> h(2 * pts.size());
    std::size_t k = 0;
    for (std::size_t i = 0; i < pts.size(); ++i) {                        // lower hull
        while (k >= 2 && cross(h[k - 1] - h[k - 2], pts[i] - h[k - 2]) <= 0) --k;
        h[k++] = pts[i];
    }
    for (std::size_t i = pts.size() - 1, t = k + 1; i-- > 0;) {           // upper hull
        while (k >= t && cross(h[k - 1] - h[k - 2], pts[i] - h[k - 2]) <= 0) --k;
        h[k++] = pts[i];
    }
    h.resize(k - 1);
    return Polygon(h);
}

} // namespace ori
