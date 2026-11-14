// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : lines, scanline and seed fills, clipping
//  Owner : Mathew Nguithi (Rasterizer)
// ============================================================================
#include "TestFramework.h"
#include "../src/raster/Clipping.h"
#include "../src/raster/Fill.h"
#include "../src/raster/Lines.h"
#include <cmath>
#include <cstdlib>
#include <set>

using namespace ori;

namespace {

unsigned gSeed = 1;
int rnd(int lo, int hi) {
    gSeed = gSeed * 1664525u + 1013904223u;
    return lo + int((gSeed >> 8) % unsigned(hi - lo + 1));
}

long countColour(const Image& img, Color c) {
    long n = 0;
    for (const Color& p : img.pixels()) n += p.sameRGB(c);
    return n;
}

double polyArea(const std::vector<Vec2>& p) {
    double a = 0;
    for (std::size_t i = 0; i < p.size(); ++i) a += cross(p[i], p[(i + 1) % p.size()]);
    return std::fabs(a) / 2;
}

} // namespace

TEST(lines_dda_and_bresenham_agree_on_shape) {
    gSeed = 3;
    for (int i = 0; i < 400; ++i) {
        const int x0 = rnd(-50, 50), y0 = rnd(-50, 50), x1 = rnd(-50, 50), y1 = rnd(-50, 50);
        const auto b = lineBresenham(x0, y0, x1, y1);
        const auto d = lineDDA(x0, y0, x1, y1);
        // Both plot one pixel per step along the major axis, end to end.
        CHECK(b.size() == std::size_t(std::max(std::abs(x1 - x0), std::abs(y1 - y0)) + 1));
        CHECK(b.size() == d.size());
        CHECK(b.front() == PixelPoint({x0, y0}) && b.back() == PixelPoint({x1, y1}));
        CHECK(d.front() == PixelPoint({x0, y0}) && d.back() == PixelPoint({x1, y1}));
        // 8-connected, and never more than half a pixel from the ideal line.
        bool connected = true, close = true;
        const double len = std::hypot(double(x1 - x0), double(y1 - y0));
        for (std::size_t k = 0; k < b.size(); ++k) {
            if (k && (std::abs(b[k].x - b[k - 1].x) > 1 || std::abs(b[k].y - b[k - 1].y) > 1)) connected = false;
            if (len > 0) {
                const double dist = std::fabs(double(x1 - x0) * (b[k].y - y0) - double(y1 - y0) * (b[k].x - x0)) / len;
                if (dist > 0.5 * std::max(std::fabs(double(x1 - x0)), std::fabs(double(y1 - y0))) / len + 1e-9) close = false;
            }
        }
        CHECK(connected);
        CHECK(close);
        // DDA and Bresenham differ by at most one pixel in the minor axis (rounding ties).
        bool near = true;
        for (std::size_t k = 0; k < b.size(); ++k)
            if (std::abs(b[k].x - d[k].x) + std::abs(b[k].y - d[k].y) > 1) near = false;
        CHECK(near);
    }
}

TEST(lines_bresenham_is_reversible_without_ties) {
    // Lines whose slope is 1/odd never hit a rounding tie, so both directions match.
    for (int k = 1; k < 20; k += 2) {
        const auto f = lineBresenham(0, 0, 3 * k, 3);
        auto r = lineBresenham(3 * k, 3, 0, 0);
        std::reverse(r.begin(), r.end());
        CHECK(f == r);
        CHECK(lineDDA(0, 0, 3 * k, 3) == f);
    }
}

TEST(lines_dash_pattern_and_brush) {
    Image img(100, 20, colors::White);
    drawStroke(img, {0, 5}, {99, 5}, Stroke::dashed(colors::Black, 1, 6, 4));   // 6 on, 4 off
    CHECK(countColour(img, colors::Black) == 60);
    Image thick(40, 40, colors::White);
    drawStroke(thick, {5, 20}, {34, 20}, Stroke::solid(colors::Black, 5));
    CHECK(countColour(thick, colors::Black) >= 30 * 5);   // a 5-px wide band
    CHECK(thick.getPixel(20, 22) == colors::Black && thick.getPixel(20, 23) == colors::White);
    // Wu blends: every touched pixel is a grey between white and black.
    Image aa(30, 30, colors::White);
    drawLineWu(aa, 2.3, 4.1, 27.7, 19.6, colors::Black);
    long greys = 0;
    for (const Color& p : aa.pixels()) greys += (p.r > 0 && p.r < 255);
    CHECK(greys > 20);
    drawLineWu(aa, 10, 2, 10, 28, colors::Black);           // vertical: no division by zero
    CHECK(aa.getPixel(10, 15).r < 128);
}

TEST(scanline_fill_exact_rectangles_and_shared_edges) {
    Image img(50, 50, colors::White);
    const auto st = fillPolygon(img, Contour{{10, 10}, {30, 10}, {30, 25}, {10, 25}}, colors::Black);
    CHECK(countColour(img, colors::Black) == 20 * 15);
    CHECK(st.edges == 2 && st.scanlines == 15 && st.spans == 15);
    // Two triangles sharing a diagonal: top-left rule -> no gaps, no overlap.
    Image a(64, 64, colors::White);
    const Contour t1 = {{3.3, 4.1}, {57.8, 9.6}, {11.2, 55.5}}, t2 = {{57.8, 9.6}, {60.1, 58.2}, {11.2, 55.5}};
    fillPolygonAlpha(a, t1, colors::Black, 0.5);
    fillPolygonAlpha(a, t2, colors::Black, 0.5);
    long once = 0, twice = 0;
    for (const Color& p : a.pixels()) { once += (p.r == 128 || p.r == 127); twice += p.r < 100; }
    CHECK(twice == 0);
    Image b(64, 64, colors::White);
    fillPolygon(b, Contour{{3.3, 4.1}, {57.8, 9.6}, {60.1, 58.2}, {11.2, 55.5}}, colors::Black);
    CHECK(once == countColour(b, colors::Black));
    // Pixel count close to the true area.
    CHECK(std::fabs(once - polyArea({{3.3, 4.1}, {57.8, 9.6}, {60.1, 58.2}, {11.2, 55.5}})) < 40);
}

TEST(scanline_even_odd_holes) {
    Image img(100, 100, colors::White);
    const Contour outer = {{10, 10}, {90, 10}, {90, 90}, {10, 90}}, hole = {{30, 30}, {70, 30}, {70, 70}, {30, 70}};
    fillPolygon(img, std::vector<Contour>{outer, hole}, colors::Black);
    CHECK(countColour(img, colors::Black) == 80 * 80 - 40 * 40);
    CHECK(img.getPixel(50, 50) == colors::White);
    CHECK(img.getPixel(20, 50) == colors::Black);
    // Off-screen geometry is clipped, not crashed on.
    fillPolygon(img, Contour{{-1000, -1000}, {2000, -1000}, {2000, 2000}}, colors::Red);
    CHECK(countColour(img, colors::Red) > 0);
}

TEST(seed_fills_4_vs_8_connected) {
    Image img(21, 21, colors::White);
    drawLine(img, 10, 1, 19, 10, colors::Black); drawLine(img, 19, 10, 10, 19, colors::Black);
    drawLine(img, 10, 19, 1, 10, colors::Black); drawLine(img, 1, 10, 10, 1, colors::Black);
    Image f4 = img, f8 = img, b4 = img;
    const long n4 = floodFill(f4, 10, 10, colors::Red, false);
    const long n8 = floodFill(f8, 10, 10, colors::Red, true);
    const long nb = boundaryFill(b4, 10, 10, colors::Red, colors::Black, false);
    CHECK(n4 > 100 && n4 == nb);                 // stays inside the diamond
    CHECK(f4.getPixel(0, 0) == colors::White);
    CHECK(n8 > n4 && f8.getPixel(0, 0) == colors::Red);   // leaks through diagonal gaps
    CHECK(floodFill(f4, 10, 10, colors::Red, false) == n4);   // same colour again: same region, no hang
    // Large region: explicit stack, no recursion depth problems.
    Image big(1000, 1000, colors::White);
    CHECK(floodFill(big, 500, 500, colors::Blue) == 1000L * 1000L);
}

TEST(clipping_cohen_sutherland_matches_liang_barsky) {
    gSeed = 99;
    const ClipRect r{10, 20, 90, 70};
    int both = 0;
    for (int i = 0; i < 500; ++i) {
        Vec2 a{double(rnd(-40, 140)), double(rnd(-40, 140))}, b{double(rnd(-40, 140)), double(rnd(-40, 140))};
        Vec2 c = a, d = b, e = a, f = b;
        const bool cs = clipCohenSutherland(a, b, r), lb = clipLiangBarsky(c, d, r), cb = clipCyrusBeck(e, f, r.polygon());
        CHECK(cs == lb);
        CHECK(lb == cb);
        if (cs && lb) {
            ++both;
            CHECK(nearlyEqual(a, c, 1e-6) && nearlyEqual(b, d, 1e-6));
            CHECK(nearlyEqual(c, e, 1e-6) && nearlyEqual(d, f, 1e-6));
            CHECK(outCode(a, ClipRect{9.999, 19.999, 90.001, 70.001}) == kInside);
        }
    }
    CHECK(both > 50);
}

TEST(clipping_cyrus_beck_any_convex_window_and_orientation) {
    const std::vector<Vec2> diamond = {{50, 0}, {100, 50}, {50, 100}, {0, 50}};
    const std::vector<Vec2> reversed(diamond.rbegin(), diamond.rend());
    Vec2 a{-20, 50}, b{120, 50}, c = a, d = b;
    CHECK(clipCyrusBeck(a, b, diamond) && clipCyrusBeck(c, d, reversed));
    CHECK(nearlyEqual(a, {0, 50}, 1e-9) && nearlyEqual(b, {100, 50}, 1e-9));
    CHECK(nearlyEqual(a, c, 1e-9) && nearlyEqual(b, d, 1e-9));
    Vec2 e{0, 0}, f{10, 0};
    CHECK(!clipCyrusBeck(e, f, diamond));                    // misses the corner region
    Vec2 p, q;
    CHECK(clipLineToConvex({50, 50}, {51, 51}, diamond, p, q));   // infinite line -> chord
    CHECK(nearlyEqual(p, {25, 25}, 1e-9) && nearlyEqual(q, {75, 75}, 1e-9));
}

TEST(clipping_sutherland_hodgman_areas) {
    const std::vector<Vec2> a = {{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    const std::vector<Vec2> b = {{50, 50}, {150, 50}, {150, 150}, {50, 150}};
    CHECK_NEAR(polyArea(clipSutherlandHodgman(a, b)), 2500.0, 1e-9);
    const auto stages = sutherlandHodgmanStages(b, a);
    CHECK(stages.size() == 4);
    CHECK_NEAR(polyArea(stages.back()), 2500.0, 1e-9);
    // Concave subject (a "C") against a square window.
    const std::vector<Vec2> c = {{-10, -10}, {110, -10}, {110, 20}, {20, 20}, {20, 80}, {110, 80}, {110, 110}, {-10, 110}};
    CHECK_NEAR(polyArea(clipPolygonToRect(c, ClipRect{0, 0, 100, 100})), 100.0 * 100 - 80 * 60, 1e-9);
    CHECK(clipSutherlandHodgman(a, {{200, 200}, {300, 200}, {300, 300}}).empty());
}
