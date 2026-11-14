// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : raster          Owner : Mathew Nguithi (Rasterizer)
//  File   : Fill.cpp
// ============================================================================
#include "Fill.h"
#include "Lines.h"
#include <algorithm>
#include <cmath>
#include <map>

namespace ori {

namespace {

struct Edge {
    int yStart = 0, yEnd = 0;   // first and one-past-last scanline the edge crosses
    double x = 0;               // x where the edge meets the current scanline centre
    double invSlope = 0;        // dx / dy, added to x on every scanline
};

} // namespace

ScanlineStats scanlineSpans(const std::vector<Contour>& contours,
                            const std::function<void(int, int, int)>& span, int maxY) {
    ScanlineStats st;

    // ---- 1. Edge Table: bucket every non-horizontal edge by its first scanline.
    std::map<int, std::vector<Edge>> table;
    for (const Contour& c : contours) {
        const std::size_t n = c.size();
        if (n < 3) continue;
        for (std::size_t i = 0; i < n; ++i) {
            Vec2 a = c[i], b = c[(i + 1) % n];
            if (a.y == b.y) continue;                       // horizontal: never crossed
            if (a.y > b.y) std::swap(a, b);
            // Scanline y samples the pixel centre y + 0.5. The edge covers the
            // half-open interval [a.y, b.y), i.e. scanlines ceil(a.y - .5) ..
            const int y0 = int(std::ceil(a.y - 0.5));
            const int y1 = int(std::ceil(b.y - 0.5));
            if (y1 <= y0) continue;                         // misses every centre
            Edge e;
            e.yStart = y0;
            e.yEnd = y1;
            e.invSlope = (b.x - a.x) / (b.y - a.y);
            e.x = a.x + (y0 + 0.5 - a.y) * e.invSlope;
            table[y0].push_back(e);
            ++st.edges;
        }
    }
    if (table.empty()) return st;

    // ---- 2. Walk the scanlines with the Active Edge Table.
    std::vector<Edge> active;
    auto it = table.begin();
    int y = it->first;
    if (maxY > 0 && y < 0) y = 0;                           // rows above the image cost nothing
    std::vector<double> xs;
    while (it != table.end() || !active.empty()) {
        if (active.empty() && it != table.end() && y < it->first) y = it->first;
        // add edges that start on this scanline
        while (it != table.end() && it->first <= y) {
            for (const Edge& e : it->second) {
                Edge moved = e;
                moved.x += (y - e.yStart) * e.invSlope;    // only if we skipped rows
                moved.yStart = y;
                if (moved.yEnd > y) active.push_back(moved);
            }
            ++it;
        }
        if (maxY > 0 && y >= maxY) break;

        if (y >= 0 && !active.empty()) {
            // sort the crossings by x and fill between pairs (even-odd)
            xs.clear();
            for (const Edge& e : active) xs.push_back(e.x);
            std::sort(xs.begin(), xs.end());
            bool any = false;
            for (std::size_t k = 0; k + 1 < xs.size(); k += 2) {
                const int xa = int(std::ceil(xs[k] - 0.5));
                const int xb = int(std::ceil(xs[k + 1] - 0.5));   // exclusive
                if (xb > xa) {
                    span(y, xa, xb);
                    ++st.spans;
                    st.pixels += xb - xa;
                    any = true;
                }
            }
            if (any) ++st.scanlines;
        }

        // advance: drop finished edges, step x by 1/m
        ++y;
        std::size_t w = 0;
        for (std::size_t k = 0; k < active.size(); ++k) {
            if (active[k].yEnd <= y) continue;
            active[k].x += active[k].invSlope;
            active[w++] = active[k];
        }
        active.resize(w);
    }
    return st;
}

ScanlineStats fillPolygon(Image& img, const Contour& poly, Color c) {
    return fillPolygon(img, std::vector<Contour>{poly}, c);
}

ScanlineStats fillPolygon(Image& img, const std::vector<Contour>& contours, Color c) {
    return scanlineSpans(contours, [&](int y, int x0, int x1) {
        if (y < 0 || y >= img.height()) return;
        for (int x = std::max(0, x0); x < std::min(img.width(), x1); ++x) img.setPixel(x, y, c);
    }, img.height());
}

ScanlineStats fillPolygonAlpha(Image& img, const std::vector<Contour>& contours, Color c, double alpha) {
    return scanlineSpans(contours, [&](int y, int x0, int x1) {
        if (y < 0 || y >= img.height()) return;
        for (int x = std::max(0, x0); x < std::min(img.width(), x1); ++x) img.blendPixel(x, y, c, alpha);
    }, img.height());
}

ScanlineStats fillPolygonShaded(Image& img, const std::vector<Contour>& contours, const PixelShader& shader) {
    return scanlineSpans(contours, [&](int y, int x0, int x1) {
        if (y < 0 || y >= img.height()) return;
        for (int x = std::max(0, x0); x < std::min(img.width(), x1); ++x) img.setPixel(x, y, shader(x, y));
    }, img.height());
}

// ------------------------------------------------------------- seed fills --
namespace {

template <class Inside>
long seedFill(Image& img, int x, int y, Color fill, bool eight, Inside inside) {
    if (!img.inBounds(x, y) || !inside(img.getPixel(x, y))) return 0;
    static const int d4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    static const int d8[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    std::vector<char> seen(std::size_t(img.width()) * img.height(), 0);
    std::vector<PixelPoint> stack;   // explicit stack instead of recursion
    stack.push_back({x, y});
    seen[std::size_t(y) * img.width() + x] = 1;
    long count = 0;
    while (!stack.empty()) {
        const PixelPoint p = stack.back();
        stack.pop_back();
        img.setPixel(p.x, p.y, fill);
        ++count;
        const int n = eight ? 8 : 4;
        for (int k = 0; k < n; ++k) {
            const int nx = p.x + (eight ? d8[k][0] : d4[k][0]);
            const int ny = p.y + (eight ? d8[k][1] : d4[k][1]);
            if (!img.inBounds(nx, ny)) continue;
            char& s = seen[std::size_t(ny) * img.width() + nx];
            if (s || !inside(img.getPixel(nx, ny))) continue;
            s = 1;
            stack.push_back({nx, ny});
        }
    }
    return count;
}

} // namespace

long floodFill(Image& img, int x, int y, Color fill, bool eightConnected) {
    if (!img.inBounds(x, y)) return 0;
    const Color target = img.getPixel(x, y);
    return seedFill(img, x, y, fill, eightConnected, [&](Color c) { return c.sameRGB(target); });
}

long boundaryFill(Image& img, int x, int y, Color fill, Color boundary, bool eightConnected) {
    return seedFill(img, x, y, fill, eightConnected, [&](Color c) { return !c.sameRGB(boundary); });
}

} // namespace ori
