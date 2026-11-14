// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : segments, lines, polygons
//  Owner : Evans Mwendwa (Crease pattern editor)
// ============================================================================
#include "TestFramework.h"
#include "../src/geometry/Line.h"
#include "../src/geometry/Polygon.h"
#include <cmath>

using namespace ori;

TEST(geometry_line_intersections) {
    const auto p = intersect(Line::through({0, 0}, {10, 10}), Line::through({0, 10}, {10, 0}));
    CHECK(p && nearlyEqual(*p, {5, 5}));
    CHECK(!intersect(Line::through({0, 0}, {1, 0}), Line::through({0, 1}, {1, 1})));   // parallel
    const auto s = intersect(Segment{{0, 0}, {4, 4}}, Segment{{0, 4}, {4, 0}});
    CHECK(s && nearlyEqual(*s, {2, 2}));
    CHECK(!intersect(Segment{{0, 0}, {1, 1}}, Segment{{3, 0}, {2, 1}}));           // lines meet, segments do not
    const auto t = intersect(Segment{{0, 0}, {2, 0}}, Segment{{2, -1}, {2, 1}});   // touching at an end point
    CHECK(t && nearlyEqual(*t, {2, 0}));
    const auto u = intersect(Line::through({5, -100}, {5, 100}), Segment{{0, 3}, {10, 3}});
    CHECK(u && nearlyEqual(*u, {5, 3}));
    CHECK(!intersect(Line::through({50, 0}, {50, 1}), Segment{{0, 3}, {10, 3}}));
}

TEST(geometry_line_measures_and_reflection) {
    const Line l = Line::through({0, 0}, {4, 3});
    double A, B, C;
    l.implicit(A, B, C);
    CHECK_NEAR(A * A + B * B, 1.0, 1e-12);
    CHECK_NEAR(l.distanceTo({0, 5}), 4.0, 1e-12);
    CHECK(nearlyEqual(l.project({0, 5}), {2.4, 1.8}, 1e-12));
    const Vec2 r = l.reflect({0, 5});
    CHECK(nearlyEqual(r, {4.8, -1.4}, 1e-12));
    CHECK(nearlyEqual(l.reflect(r), {0, 5}, 1e-12));                      // involution
    CHECK(nearlyEqual(l.reflectionMatrix().apply({0, 5}), r, 1e-9));      // agrees with Mat3
    CHECK(l.side({0, 5}) * l.side(r) < 0);
    CHECK_NEAR(distanceToSegment({5, 5}, Segment{{0, 0}, {2, 0}}), std::hypot(3.0, 5.0), 1e-12);
    CHECK_NEAR(distanceToSegment({1, 5}, Segment{{0, 0}, {2, 0}}), 5.0, 1e-12);
    CHECK_NEAR(angleDegrees(Segment{{0, 0}, {0, 3}}), 90.0, 1e-12);
}

TEST(geometry_polygon_measures) {
    const Polygon sq = Polygon::rectangle(0, 0, 10, 20);
    CHECK_NEAR(sq.area(), 200.0, 1e-12);
    CHECK_NEAR(sq.perimeter(), 60.0, 1e-12);
    CHECK(nearlyEqual(sq.centroid(), {5, 10}, 1e-12));
    CHECK(sq.isConvex());
    CHECK_NEAR(sq.signedArea(), -sq.reversed().signedArea(), 1e-12);
    const BBox b = Polygon::regular(6, {0, 0}, 10).bounds();
    CHECK_NEAR(b.width(), 20.0, 1e-9);
    // L-shape: centroid of a non-convex polygon.
    const Polygon L({{0, 0}, {20, 0}, {20, 10}, {10, 10}, {10, 30}, {0, 30}});
    CHECK(!L.isConvex());
    CHECK_NEAR(L.area(), 400.0, 1e-12);
    CHECK(nearlyEqual(L.centroid(), {7.5, 12.5}, 1e-9));
    CHECK(L.contains({5, 25}) && L.contains({15, 5}) && !L.contains({15, 25}));
    const Polygon m = L.transformed(Mat3::translate(100, 0) * Mat3::rotate(0.7));
    CHECK_NEAR(m.area(), L.area(), 1e-9);
}

TEST(geometry_triangulation_and_hull) {
    const Polygon star = [] {
        Polygon p;
        for (int i = 0; i < 10; ++i) {
            const double r = i % 2 ? 40 : 100, a = i * kPi / 5;
            p.v.push_back({r * std::cos(a), r * std::sin(a)});
        }
        return p;
    }();
    const auto tris = star.triangulate();
    CHECK(tris.size() == star.size() - 2);
    double sum = 0;
    for (const auto& t : tris) sum += Polygon({t[0], t[1], t[2]}).area();
    CHECK_NEAR(sum, star.area(), 1e-6);
    // Clockwise input triangulates the same.
    CHECK(star.reversed().triangulate().size() == star.size() - 2);
    const Polygon hull = convexHull(star.v);
    CHECK(hull.size() == 5);
    CHECK(hull.isConvex());
    for (const Vec2& p : star.v) CHECK(hull.contains(p * 0.999));   // every point inside (centre is the origin)
    CHECK(convexHull({{0, 0}, {1, 1}}).size() == 2);
}
