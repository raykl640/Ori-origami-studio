// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : Bezier, splines, easing
//  Owner : Matthew Wachira (Colour, shading & curves)
// ============================================================================
#include "TestFramework.h"
#include "../src/curves/Curves.h"
#include <cmath>
#include <set>
#include <vector>

using namespace ori;

TEST(bezier_endpoints_and_forms_agree) {
    const std::vector<Vec2> c = {{0, 0}, {10, 30}, {40, -20}, {50, 10}};
    CHECK(nearlyEqual(deCasteljau(c, 0), c.front()));
    CHECK(nearlyEqual(deCasteljau(c, 1), c.back()));
    for (double t = 0; t <= 1.0; t += 0.05)
        CHECK(nearlyEqual(deCasteljau(c, t), bezierCubicBernstein(c[0], c[1], c[2], c[3], t), 1e-9));
    const auto ad = bezierAdaptive(c, 0.25);
    CHECK(nearlyEqual(ad.front(), c.front()) && nearlyEqual(ad.back(), c.back()));
}

TEST(catmull_rom_interpolates_control_points) {
    const std::vector<Vec2> pts = {{0, 0}, {10, 5}, {20, -5}, {30, 0}};
    const auto curve = catmullRom(pts, 10);
    for (std::size_t i = 0; i < pts.size(); ++i) CHECK(nearlyEqual(curve[i * 10], pts[i], 1e-9));
}

TEST(easing_is_monotonic_with_fixed_ends) {
    CHECK_NEAR(easeInOut(0), 0.0, 1e-6);
    CHECK_NEAR(easeInOut(1), 1.0, 1e-6);
    CHECK_NEAR(easeInOut(0.5), 0.5, 1e-6);
    double prev = -1;
    for (int i = 0; i <= 100; ++i) { const double v = easeInOut(i / 100.0); CHECK(v >= prev); prev = v; }
}

