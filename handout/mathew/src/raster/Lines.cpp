// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : raster          Owner : Mathew Nguithi (Rasterizer)
//  File   : Lines.cpp
// ============================================================================
#include "Lines.h"
#include <algorithm>
#include <cmath>

namespace ori {

// ------------------------------------------------------------------- DDA --
std::vector<PixelPoint> lineDDA(int x0, int y0, int x1, int y1) {
    std::vector<PixelPoint> out;
    const int dx = x1 - x0, dy = y1 - y0;
    const int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) { out.push_back({x0, y0}); return out; }
    const double xInc = double(dx) / steps, yInc = double(dy) / steps;
    double x = x0, y = y0;
    for (int i = 0; i <= steps; ++i) {
        out.push_back({int(std::lround(x)), int(std::lround(y))});
        x += xInc;
        y += yInc;
    }
    return out;
}

// ------------------------------------------------------------- Bresenham --
// One error term handles all 8 octants: err = dx + dy (dy negative) tracks the
// distance from the ideal line; each step moves in x, in y, or in both.
std::vector<PixelPoint> lineBresenham(int x0, int y0, int x1, int y1) {
    std::vector<PixelPoint> out;
    const int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        out.push_back({x0, y0});
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return out;
}

void drawLineDDA(Image& img, int x0, int y0, int x1, int y1, Color c) {
    for (const PixelPoint& p : lineDDA(x0, y0, x1, y1)) img.setPixel(p.x, p.y, c);
}

void drawLine(Image& img, int x0, int y0, int x1, int y1, Color c) {
    const int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        img.setPixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// ------------------------------------------------------------ Xiaolin Wu --
namespace {
double fpart(double x) { return x - std::floor(x); }
double rfpart(double x) { return 1.0 - fpart(x); }
}

void drawLineWu(Image& img, double x0, double y0, double x1, double y1, Color c) {
    const bool steep = std::fabs(y1 - y0) > std::fabs(x1 - x0);
    if (steep) { std::swap(x0, y0); std::swap(x1, y1); }
    if (x0 > x1) { std::swap(x0, x1); std::swap(y0, y1); }
    auto plot = [&](int x, int y, double a) {
        if (steep) img.blendPixel(y, x, c, a); else img.blendPixel(x, y, c, a);
    };
    const double dx = x1 - x0, dy = y1 - y0;
    const double gradient = dx < 1e-12 ? 1.0 : dy / dx;

    // first endpoint
    double xend = std::round(x0);
    double yend = y0 + gradient * (xend - x0);
    double xgap = rfpart(x0 + 0.5);
    const int xpx1 = int(xend), ypx1 = int(std::floor(yend));
    plot(xpx1, ypx1, rfpart(yend) * xgap);
    plot(xpx1, ypx1 + 1, fpart(yend) * xgap);
    double intery = yend + gradient;

    // second endpoint
    xend = std::round(x1);
    yend = y1 + gradient * (xend - x1);
    xgap = fpart(x1 + 0.5);
    const int xpx2 = int(xend), ypx2 = int(std::floor(yend));
    plot(xpx2, ypx2, rfpart(yend) * xgap);
    plot(xpx2, ypx2 + 1, fpart(yend) * xgap);

    // main loop: coverage split between the two pixels straddling the line
    for (int x = xpx1 + 1; x < xpx2; ++x) {
        plot(x, int(std::floor(intery)), rfpart(intery));
        plot(x, int(std::floor(intery)) + 1, fpart(intery));
        intery += gradient;
    }
}

// ------------------------------------------------------- crease patterns --
void drawValleyCrease(Image& img, int x0, int y0, int x1, int y1, Color c, int dash, int gap) {
    const int period = std::max(1, dash + gap);
    int n = 0;
    for (const PixelPoint& p : lineBresenham(x0, y0, x1, y1))
        if (n++ % period < dash) img.setPixel(p.x, p.y, c);
}

void drawMountainCrease(Image& img, int x0, int y0, int x1, int y1, Color c, int dash, int gap, int dot) {
    const int period = std::max(1, dash + gap + dot + gap);
    int n = 0;
    for (const PixelPoint& p : lineBresenham(x0, y0, x1, y1)) {
        const int m = n++ % period;
        if (m < dash || (m >= dash + gap && m < dash + gap + dot)) img.setPixel(p.x, p.y, c);
    }
}

// --------------------------------------------------------------- strokes --
Stroke Stroke::solid(Color c, int width) { Stroke s; s.color = c; s.width = width; return s; }
Stroke Stroke::dashed(Color c, int width, int dash, int gap) {
    Stroke s = solid(c, width);
    s.pattern = {dash + width - 1, gap + width - 1};   // wide brushes overlap, so lengthen both runs
    return s;
}
Stroke Stroke::dotted(Color c, int width) { Stroke s = solid(c, width); s.pattern = {width, 2 + 2 * width}; return s; }
Stroke Stroke::dashDot(Color c, int width) {
    Stroke s = solid(c, width);
    s.pattern = {9 + width, 3 + width, width, 3 + width};
    return s;
}
Stroke Stroke::smooth(Color c) { Stroke s = solid(c, 1); s.antialias = true; return s; }

void stampBrush(Image& img, int x, int y, int width, Color c) {
    if (width <= 1) { img.setPixel(x, y, c); return; }
    // Round brush: pixel centres inside a circle of diameter `width`.
    const double r = width / 2.0;
    const int lo = -(width / 2), hi = (width - 1) / 2;
    const double off = (width % 2 == 0) ? 0.5 : 0.0;
    for (int j = lo; j <= hi; ++j)
        for (int i = lo; i <= hi; ++i) {
            const double fx = i + off, fy = j + off;
            if (fx * fx + fy * fy <= r * r + 0.25) img.setPixel(x + i, y + j, c);
        }
}

namespace {

// Is pattern step `n` an "on" pixel?
bool patternOn(const std::vector<int>& pattern, long n) {
    if (pattern.empty()) return true;
    long period = 0;
    for (int run : pattern) period += std::max(1, run);
    long m = n % period;
    for (std::size_t i = 0; i < pattern.size(); ++i) {
        const int run = std::max(1, pattern[i]);
        if (m < run) return i % 2 == 0;
        m -= run;
    }
    return true;
}

// Draw one segment; `phase` counts pixel steps so dashes continue across joints.
// `skipFirst` avoids plotting a shared joint pixel twice (matters for alpha).
void strokeSegment(Image& img, const Vec2& a, const Vec2& b, const Stroke& s, long& phase, bool skipFirst) {
    if (s.antialias && s.width <= 1 && s.pattern.empty()) {
        drawLineWu(img, a.x, a.y, b.x, b.y, s.color);
        return;
    }
    const auto px = lineBresenham(int(std::lround(a.x)), int(std::lround(a.y)),
                                  int(std::lround(b.x)), int(std::lround(b.y)));
    for (std::size_t i = skipFirst ? 1 : 0; i < px.size(); ++i) {
        if (patternOn(s.pattern, phase)) stampBrush(img, px[i].x, px[i].y, s.width, s.color);
        ++phase;
    }
}

} // namespace

void drawStroke(Image& img, const Vec2& a, const Vec2& b, const Stroke& s) {
    long phase = 0;
    strokeSegment(img, a, b, s, phase, false);
}

void drawPolyline(Image& img, const std::vector<Vec2>& pts, const Stroke& s, bool closed) {
    if (pts.empty()) return;
    if (pts.size() == 1) { stampBrush(img, int(std::lround(pts[0].x)), int(std::lround(pts[0].y)), s.width, s.color); return; }
    long phase = 0;
    const std::size_t n = closed ? pts.size() : pts.size() - 1;
    for (std::size_t i = 0; i < n; ++i)
        strokeSegment(img, pts[i], pts[(i + 1) % pts.size()], s, phase, i > 0);
}

} // namespace ori
