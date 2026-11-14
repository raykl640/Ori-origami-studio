// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : raster          Owner : Matthew Wachira (Colour, shading & curves)
//  File   : Circle.cpp
// ============================================================================
#include "Circle.h"
#include "../math/Vec2.h"
#include <algorithm>
#include <cmath>

namespace ori {

// ---------------------------------------------------------- midpoint circle --
std::vector<PixelPoint> circleOctant(int r) {
    std::vector<PixelPoint> pts;
    if (r < 0) return pts;
    int x = 0, y = r;
    int d = 1 - r;                       // decision parameter at the first midpoint
    while (x <= y) {
        pts.push_back({x, y});
        if (d < 0) {
            d += 2 * x + 3;              // midpoint inside: go east
        } else {
            d += 2 * (x - y) + 5;        // midpoint outside: go south-east
            --y;
        }
        ++x;
    }
    return pts;
}

void drawCircle(Image& img, int cx, int cy, int r, Color c) {
    for (const PixelPoint& p : circleOctant(r)) {
        const int x = p.x, y = p.y;
        img.setPixel(cx + x, cy + y, c); img.setPixel(cx - x, cy + y, c);
        img.setPixel(cx + x, cy - y, c); img.setPixel(cx - x, cy - y, c);
        img.setPixel(cx + y, cy + x, c); img.setPixel(cx - y, cy + x, c);
        img.setPixel(cx + y, cy - x, c); img.setPixel(cx - y, cy - x, c);
    }
}

namespace {

// Half-width of the filled shape on every row dy in [-ry, ry], built from the
// midpoint points so the fill matches the outline exactly.
std::vector<int> circleHalfWidths(int r) {
    std::vector<int> half(std::size_t(2 * r + 1), -1);
    auto put = [&](int dy, int dx) {
        int& h = half[std::size_t(dy + r)];
        h = std::max(h, dx);
    };
    for (const PixelPoint& p : circleOctant(r)) {
        put(p.y, p.x); put(-p.y, p.x); put(p.x, p.y); put(-p.x, p.y);
    }
    return half;
}

std::vector<int> ellipseHalfWidths(int rx, int ry) {
    std::vector<int> half(std::size_t(2 * ry + 1), -1);
    for (const PixelPoint& p : ellipseQuadrant(rx, ry)) {
        int& a = half[std::size_t(ry + p.y)];
        int& b = half[std::size_t(ry - p.y)];
        a = std::max(a, p.x);
        b = std::max(b, p.x);
    }
    return half;
}

template <class Plot>
void spans(int cx, int cy, const std::vector<int>& half, Plot plot) {
    const int r = int(half.size() / 2);
    for (int dy = -r; dy <= r; ++dy) {
        const int h = half[std::size_t(dy + r)];
        for (int x = cx - h; x <= cx + h; ++x) plot(x, cy + dy);
    }
}

} // namespace

void fillCircle(Image& img, int cx, int cy, int r, Color c) {
    if (r < 0) return;
    spans(cx, cy, circleHalfWidths(r), [&](int x, int y) { img.setPixel(x, y, c); });
}

void fillCircleAlpha(Image& img, int cx, int cy, int r, Color c, double alpha) {
    if (r < 0) return;
    spans(cx, cy, circleHalfWidths(r), [&](int x, int y) { img.blendPixel(x, y, c, alpha); });
}

void drawRing(Image& img, int cx, int cy, int r, int thickness, Color c) {
    if (thickness <= 1) { drawCircle(img, cx, cy, r, c); return; }
    const auto outer = circleHalfWidths(r);
    const int ri = std::max(-1, r - thickness);
    const auto inner = ri >= 0 ? circleHalfWidths(ri) : std::vector<int>{};
    for (int dy = -r; dy <= r; ++dy) {
        const int ho = outer[std::size_t(dy + r)];
        int hi = -1;
        if (ri >= 0 && std::abs(dy) <= ri) hi = inner[std::size_t(dy + ri)];
        for (int dx = -ho; dx <= ho; ++dx)
            if (std::abs(dx) > hi) img.setPixel(cx + dx, cy + dy, c);
    }
}

// --------------------------------------------------------- midpoint ellipse --
std::vector<PixelPoint> ellipseQuadrant(int rx, int ry, int* region1Count) {
    std::vector<PixelPoint> pts;
    if (rx < 0 || ry < 0) return pts;
    const double rx2 = double(rx) * rx, ry2 = double(ry) * ry;
    int x = 0, y = ry;
    // Region 1: slope magnitude < 1, step in x each time.
    double d1 = ry2 - rx2 * ry + 0.25 * rx2;
    double dx = 2 * ry2 * x, dy = 2 * rx2 * y;
    while (dx < dy) {
        pts.push_back({x, y});
        if (d1 < 0) {
            ++x; dx += 2 * ry2;
            d1 += dx + ry2;
        } else {
            ++x; --y; dx += 2 * ry2; dy -= 2 * rx2;
            d1 += dx - dy + ry2;
        }
    }
    if (region1Count) *region1Count = int(pts.size());
    // Region 2: slope magnitude > 1, step in y each time.
    double d2 = ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1.0) * (y - 1.0) - rx2 * ry2;
    while (y >= 0) {
        pts.push_back({x, y});
        if (d2 > 0) {
            --y; dy -= 2 * rx2;
            d2 += rx2 - dy;
        } else {
            --y; ++x; dx += 2 * ry2; dy -= 2 * rx2;
            d2 += dx - dy + rx2;
        }
    }
    // Very flat ellipses (ry much smaller than rx) can reach y = 0 before
    // x = rx; finish the run along the major axis so the outline is closed.
    if (!pts.empty() && pts.back().y == 0)
        for (int xx = pts.back().x + 1; xx <= rx; ++xx) pts.push_back({xx, 0});
    return pts;
}

void drawEllipse(Image& img, int cx, int cy, int rx, int ry, Color c) {
    for (const PixelPoint& p : ellipseQuadrant(rx, ry)) {
        img.setPixel(cx + p.x, cy + p.y, c); img.setPixel(cx - p.x, cy + p.y, c);
        img.setPixel(cx + p.x, cy - p.y, c); img.setPixel(cx - p.x, cy - p.y, c);
    }
}

void fillEllipse(Image& img, int cx, int cy, int rx, int ry, Color c) {
    if (rx < 0 || ry < 0) return;
    spans(cx, cy, ellipseHalfWidths(rx, ry), [&](int x, int y) { img.setPixel(x, y, c); });
}

void fillEllipseAlpha(Image& img, int cx, int cy, int rx, int ry, Color c, double alpha) {
    if (rx < 0 || ry < 0) return;
    spans(cx, cy, ellipseHalfWidths(rx, ry), [&](int x, int y) { img.blendPixel(x, y, c, alpha); });
}

// -------------------------------------------------------------------- arcs --
void drawArc(Image& img, int cx, int cy, int r, double a0, double a1, Color c, int width) {
    if (a1 < a0) std::swap(a0, a1);
    const double span = a1 - a0;
    auto inArc = [&](int x, int y) {
        if (span >= 2 * kPi) return true;
        double a = std::atan2(double(y), double(x)) - a0;
        a = std::fmod(a, 2 * kPi);
        if (a < 0) a += 2 * kPi;
        return a <= span + 1e-9;
    };
    for (const PixelPoint& p : circleOctant(r)) {
        const int pts[8][2] = {{p.x, p.y}, {-p.x, p.y}, {p.x, -p.y}, {-p.x, -p.y},
                               {p.y, p.x}, {-p.y, p.x}, {p.y, -p.x}, {-p.y, -p.x}};
        for (const auto& q : pts)
            if (inArc(q[0], q[1])) stampBrush(img, cx + q[0], cy + q[1], width, c);
    }
}

} // namespace ori
