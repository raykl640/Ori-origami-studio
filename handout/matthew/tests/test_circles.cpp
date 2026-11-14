// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : midpoint circle, midpoint ellipse, alpha fills, arcs
//  Owner : Matthew Wachira (Colour, shading & curves)
// ============================================================================
#include "TestFramework.h"
#include "../src/raster/Circle.h"
#include <cmath>
#include <set>

using namespace ori;

namespace {

long countColour(const Image& img, Color c) {
    long n = 0;
    for (const Color& p : img.pixels()) n += p.sameRGB(c);
    return n;
}

} // namespace

TEST(circle_midpoint_points_lie_on_the_circle) {
    for (int r = 1; r < 60; ++r) {
        const auto pts = circleOctant(r);
        CHECK(pts.front() == PixelPoint({0, r}));
        for (const PixelPoint& p : pts) {
            CHECK(p.x <= p.y + 1);                                    // stays in the octant
            CHECK(std::fabs(std::hypot(p.x, p.y) - r) <= 0.5 + 1e-9); // within half a pixel
        }
    }
    // Filled area approaches pi r^2.
    Image img(220, 220, colors::White);
    fillCircle(img, 110, 110, 100, colors::Black);
    CHECK(std::fabs(countColour(img, colors::Black) - kPi * 100 * 100) / (kPi * 100 * 100) < 0.02);
}

TEST(ellipse_midpoint_regions_cover_the_quadrant) {
    for (int rx = 2; rx < 40; rx += 3)
        for (int ry = 2; ry < 40; ry += 5) {
            int r1 = -1;
            const auto q = ellipseQuadrant(rx, ry, &r1);
            CHECK(q.front() == PixelPoint({0, ry}));
            CHECK(q.back().y == 0 && q.back().x == rx);
            CHECK(r1 >= 1 && r1 <= int(q.size()));
            for (const PixelPoint& p : q) {
                const double v = double(p.x) * p.x / (double(rx) * rx) + double(p.y) * p.y / (double(ry) * ry);
                CHECK(std::fabs(v - 1.0) < 0.5);
            }
        }
    Image img(200, 120, colors::White);
    fillEllipse(img, 100, 60, 80, 40, colors::Black);
    CHECK(std::fabs(countColour(img, colors::Black) - kPi * 80 * 40) / (kPi * 80 * 40) < 0.03);
}

TEST(alpha_fills_blend_each_pixel_once) {
    Image img(60, 60, colors::White);
    fillCircleAlpha(img, 30, 30, 20, colors::Black, 0.5);
    std::set<int> values;
    for (const Color& p : img.pixels()) values.insert(p.r);
    CHECK(values.size() == 2);                  // only white and one grey: no double blending
    CHECK(img.getPixel(30, 30).r == 128 || img.getPixel(30, 30).r == 127);
}

TEST(arcs_and_rings) {
    Image img(100, 100, colors::White);
    drawArc(img, 50, 50, 30, 0, kPi / 2, colors::Black);    // lower-right quarter (y down)
    CHECK(img.getPixel(80, 50) == colors::Black && img.getPixel(50, 80) == colors::Black);
    CHECK(img.getPixel(20, 50) == colors::White && img.getPixel(50, 20) == colors::White);
    Image ring(100, 100, colors::White);
    drawRing(ring, 50, 50, 30, 6, colors::Black);
    CHECK(ring.getPixel(50, 50) == colors::White);           // hollow centre
    CHECK(ring.getPixel(50, 22) == colors::Black);           // inside the band
    const long n = countColour(ring, colors::Black);
    CHECK(std::fabs(n - kPi * (30.0 * 30 - 24.0 * 24)) < 0.1 * n);
}
