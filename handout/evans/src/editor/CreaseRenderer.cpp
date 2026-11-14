// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : editor          Owner : Evans Mwendwa (Crease pattern editor)
//  File   : CreaseRenderer.cpp
// ============================================================================
#include "CreaseRenderer.h"
#include "../core/Font.h"
#include "../raster/Circle.h"
#include "../raster/Clipping.h"
#include "../raster/Fill.h"
#include "../raster/Lines.h"
#include <algorithm>
#include <cmath>

namespace ori {

Mat3 fitView(const BBox& world, const Rect& screen, int margin) {
    const double w = std::max(1e-9, world.width()), h = std::max(1e-9, world.height());
    const double k = std::min((screen.w - 2.0 * margin) / w, (screen.h - 2.0 * margin) / h);
    const Vec2 worldCentre = (world.min + world.max) * 0.5;
    const Vec2 screenCentre{screen.x + screen.w / 2.0, screen.y + screen.h / 2.0};
    return Mat3::translate(screenCentre) * Mat3::scale(k, k) * Mat3::translate(-worldCentre);
}

void drawCrease(Image& img, const Vec2& a, const Vec2& b, CreaseType t, const Palette& pal, int width) {
    switch (t) {
        case CreaseType::Mountain: drawStroke(img, a, b, Stroke::dashDot(pal.mountain, width)); break;
        case CreaseType::Valley:   drawStroke(img, a, b, Stroke::dashed(pal.valley, width)); break;
        default:                   drawStroke(img, a, b, Stroke::dotted(Color(150, 150, 150), 1)); break;
    }
}

void drawCreasePattern(Image& img, const CreasePattern& cp, const Mat3& view, const Palette& pal,
                       const CreaseViewOptions& opt) {
    const Polygon screenPaper = cp.paper.transformed(view);

    if (opt.fillPaper) fillPolygon(img, screenPaper.v, pal.paperBack);

    // Grid: lines of the axis-aligned lattice, trimmed to the paper with Cyrus-Beck.
    if (opt.showGrid && cp.paper.size() >= 3) {
        const BBox b = cp.paper.bounds();
        double step = opt.gridStep > 0 ? opt.gridStep : cp.gridStep > 0 ? cp.gridStep : std::max(b.width(), b.height()) / 8.0;
        if (step > 0) {
            const Color gc = pal.grid;
            for (double x = b.min.x + step; x < b.max.x - 1e-9; x += step) {
                Vec2 p, q;
                if (clipLineToConvex({x, 0}, {x, 1}, cp.paper.v, p, q))
                    drawStroke(img, view.apply(p), view.apply(q), Stroke::solid(gc));
            }
            for (double y = b.min.y + step; y < b.max.y - 1e-9; y += step) {
                Vec2 p, q;
                if (clipLineToConvex({0, y}, {1, y}, cp.paper.v, p, q))
                    drawStroke(img, view.apply(p), view.apply(q), Stroke::solid(gc));
            }
        }
    }

    for (std::size_t i = 0; i < cp.creases.size(); ++i) {
        const Crease& c = cp.creases[i];
        if (c.type == CreaseType::Aux && !opt.showAux) continue;
        const int w = int(i) == opt.highlight ? opt.creaseWidth + 2 : opt.creaseWidth;
        drawCrease(img, view.apply(c.a), view.apply(c.b), c.type, pal, w);
    }

    if (opt.showBorder) drawPolyline(img, screenPaper.v, Stroke::solid(pal.border, 2), true);

    if (opt.showVertices) {
        for (const Vec2& v : cp.vertices()) {
            const Vec2 s = view.apply(v);
            const int x = int(std::lround(s.x)), y = int(std::lround(s.y));
            fillCircle(img, x, y, 3, colors::White);
            drawCircle(img, x, y, 3, pal.border);
        }
    }
}

void drawCreaseLegend(Image& img, int x, int y, const Palette& pal) {
    drawStroke(img, {double(x), y + 3.0}, {x + 24.0, y + 3.0}, Stroke::dashDot(pal.mountain, 2));
    drawText(img, x + 30, y, "MOUNTAIN", pal.mountain);
    drawStroke(img, {x + 90.0, y + 3.0}, {x + 114.0, y + 3.0}, Stroke::dashed(pal.valley, 2));
    drawText(img, x + 120, y, "VALLEY", pal.valley);
    drawStroke(img, {x + 166.0, y + 3.0}, {x + 190.0, y + 3.0}, Stroke::solid(pal.border, 2));
    drawText(img, x + 196, y, "PAPER EDGE", pal.border);
}

// ------------------------------------------------------------ face fill --
namespace {

// Number of crease segments the straight path ref -> p crosses.
int crossings(const CreasePattern& cp, const Vec2& ref, const Vec2& p) {
    int n = 0;
    const Segment path{ref, p};
    for (const Crease& c : cp.creases)
        if (c.type != CreaseType::Aux && intersect(path, c.segment())) ++n;
    return n;
}

} // namespace

int fillCreaseFaces(Image& img, const CreasePattern& cp, const Mat3& view, Color ca, Color cb) {
    // Off-screen mask: paper = marker colour, crease lines = black boundary.
    // Each face is then found by a seed (flood) fill of the marker colour.
    const Color paperMark(0, 0, 1), line(0, 0, 0);
    Image mask(img.width(), img.height(), colors::White);
    fillPolygon(mask, cp.paper.transformed(view).v, paperMark);
    for (const Crease& c : cp.creases) {
        if (c.type == CreaseType::Aux) continue;
        // Extend each crease 1.5 px past its ends: rounding can otherwise stop a
        // line one pixel short of the paper edge (or of another crease), and the
        // flood fill would leak through that gap and merge two faces.
        Vec2 a = view.apply(c.a), b = view.apply(c.b);
        const Vec2 d = (b - a).normalized() * 1.5;
        a -= d;
        b += d;
        drawLine(mask, int(std::lround(a.x)), int(std::lround(a.y)), int(std::lround(b.x)), int(std::lround(b.y)), line);
    }

    const Mat3 inv = view.inverse();
    // Reference point slightly off any vertex so the parity paths avoid them.
    const Vec2 ref = cp.paper.centroid() + Vec2{0.01731, 0.02913} * std::max(1.0, cp.paper.bounds().width());
    std::vector<bool> parity;   // per face id
    int faces = 0;
    for (int y = 0; y < mask.height(); ++y)
        for (int x = 0; x < mask.width(); ++x) {
            if (!mask.getPixel(x, y).sameRGB(paperMark)) continue;
            // New face: give it its own id colour (b = 2 marks "face"), then fill the region.
            // 4-connected, so it cannot leak through the diagonal steps of a Bresenham line.
            const int id = int(parity.size());
            const Color idColour(std::uint8_t(id & 0xFF), std::uint8_t((id >> 8) & 0xFF), 2);
            const long size = floodFill(mask, x, y, idColour, false);
            const Vec2 p = inv.apply(Vec2{x + 0.5, y + 0.5});
            parity.push_back(crossings(cp, ref, p) % 2 == 1);
            if (size >= 8) ++faces;   // ignore pixel crumbs where lines meet
        }

    for (int y = 0; y < mask.height(); ++y)
        for (int x = 0; x < mask.width(); ++x) {
            const Color m = mask.getPixel(x, y);
            if (m.b != 2) continue;
            const int id = m.r | (m.g << 8);
            if (id < int(parity.size())) img.setPixel(x, y, parity[std::size_t(id)] ? cb : ca);
        }
    return faces;
}

} // namespace ori
