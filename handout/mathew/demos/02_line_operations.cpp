// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demo 02 : Line operations                  Owner : Mathew Nguithi
//  Cohen-Sutherland and Liang-Barsky clipping (checked against each other),
//  Cyrus-Beck clipping to a diamond sheet, segment intersection = crease
//  vertices, reflecting a line across a crease, and vertex-then-grid snapping.
// ============================================================================
#include "Demos.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/geometry/Line.h"
#include "../src/raster/Circle.h"
#include "../src/raster/Clipping.h"
#include "../src/raster/Fill.h"
#include "../src/raster/Lines.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace ori {

namespace {

// Small deterministic generator so every run draws the same picture.
struct Lcg {
    unsigned s;
    explicit Lcg(unsigned seed) : s(seed) {}
    double next() { s = s * 1664525u + 1013904223u; return (s >> 8) / double(1u << 24); }
    double range(double a, double b) { return a + (b - a) * next(); }
};

void drawClipWindow(Image& img, const ClipRect& r) {
    img.fillRect(int(r.xmin), int(r.ymin), int(r.xmax - r.xmin), int(r.ymax - r.ymin), Color(253, 246, 240));
    drawPolyline(img, r.polygon(), Stroke::solid(Color::hex(0x2B2B33), 2), true);
}

std::vector<std::pair<Vec2, Vec2>> demoSegments(const Rect& in, unsigned seed, int n) {
    Lcg g(seed);
    std::vector<std::pair<Vec2, Vec2>> segs;
    for (int i = 0; i < n; ++i)
        segs.push_back({{g.range(in.x + 10, in.right() - 10), g.range(in.y + 8, in.y + 230)},
                        {g.range(in.x + 10, in.right() - 10), g.range(in.y + 8, in.y + 230)}});
    return segs;
}

} // namespace

std::vector<std::string> demoLineOperations(const DemoContext& ctx) {
    int top = 0;
    Image img = makeSheet("LINE OPERATIONS", "CLIPPING (COHEN-SUTHERLAND, LIANG-BARSKY, CYRUS-BECK), INTERSECTION, REFLECTION, SNAPPING", top);
    const Palette pal = Palette::classic();
    const Color ink = Color::hex(0x2B2B33), faded(190, 185, 178);

    // ---- 1: Cohen-Sutherland ----------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(0, 3, 2, img.width(), img.height(), top), "COHEN-SUTHERLAND: REGION CODES");
        const ClipRect win{in.x + 110.0, in.y + 70.0, in.x + 270.0, in.y + 175.0};
        drawClipWindow(img, win);
        // outcodes of the 9 regions
        const char* codes[3][3] = {{"1001", "1000", "1010"}, {"0001", "0000", "0010"}, {"0101", "0100", "0110"}};
        const int xs[3] = {in.x + 45, int(win.xmin + win.xmax) / 2 - 12, in.right() - 70};
        const int ys[3] = {in.y + 30, int(win.ymin + win.ymax) / 2 - 3, in.y + 215};
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c) drawText(img, xs[c], ys[r], codes[r][c], Color(170, 160, 150));
        for (const auto& s : demoSegments(in, 11, 9)) {
            Vec2 a = s.first, b = s.second;
            drawStroke(img, a, b, Stroke::dotted(faded));
            if (clipCohenSutherland(a, b, win)) {
                drawStroke(img, a, b, Stroke::solid(theme::Accent, 2));
                fillCircle(img, int(a.x), int(a.y), 2, ink);
                fillCircle(img, int(b.x), int(b.y), 2, ink);
            }
        }
        drawCaption(img, in.x + 12, in.bottom() - 34, "BOTH CODES 0000: ACCEPT. CODE_A & CODE_B != 0: REJECT.\nOTHERWISE CUT AT ONE BOUNDARY AND TRY AGAIN");
    }

    // ---- 2: Liang-Barsky + agreement check ---------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(1, 3, 2, img.width(), img.height(), top), "LIANG-BARSKY: PARAMETRIC CLIPPING");
        const ClipRect win{in.x + 110.0, in.y + 70.0, in.x + 270.0, in.y + 175.0};
        drawClipWindow(img, win);
        for (const auto& s : demoSegments(in, 11, 9)) {
            Vec2 a = s.first, b = s.second;
            drawStroke(img, a, b, Stroke::dotted(faded));
            if (clipLiangBarsky(a, b, win)) {
                drawStroke(img, a, b, Stroke::solid(pal.valley, 2));
                fillCircle(img, int(a.x), int(a.y), 2, ink);
                fillCircle(img, int(b.x), int(b.y), 2, ink);
            }
        }
        // Cross-check the two algorithms on 500 random segments.
        Lcg g(2024);
        int agree = 0, accepted = 0;
        const ClipRect unit{0, 0, 100, 100};
        for (int i = 0; i < 500; ++i) {
            Vec2 a{g.range(-80, 180), g.range(-80, 180)}, b{g.range(-80, 180), g.range(-80, 180)};
            Vec2 c = a, d = b;
            const bool cs = clipCohenSutherland(a, b, unit), lb = clipLiangBarsky(c, d, unit);
            accepted += cs;
            if (cs == lb && (!cs || (nearlyEqual(a, c, 1e-6) && nearlyEqual(b, d, 1e-6)))) ++agree;
        }
        char buf[200];
        std::snprintf(buf, sizeof buf, "P = (-DX, DX, -DY, DY), Q = DISTANCES TO EDGES\nT0 = MAX ENTERING, T1 = MIN LEAVING. %d/500 RANDOM\nSEGMENTS AGREE WITH COHEN-SUTHERLAND (%d CLIPPED)", agree, accepted);
        drawCaption(img, in.x + 12, in.bottom() - 45, buf);
    }

    // ---- 3: Cyrus-Beck to a diamond sheet -------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(2, 3, 2, img.width(), img.height(), top), "CYRUS-BECK: CLIP CREASES TO A DIAMOND");
        const Vec2 c{double(in.cx()), in.y + 120.0};
        const std::vector<Vec2> diamond = {{c.x, c.y - 100}, {c.x + 100, c.y}, {c.x, c.y + 100}, {c.x - 100, c.y}};
        fillPolygon(img, diamond, pal.paperBack);
        drawPolyline(img, diamond, Stroke::solid(pal.border, 2), true);
        Lcg g(5);
        for (int i = 0; i < 8; ++i) {
            const double ang = g.range(0, kPi);
            const Vec2 d{std::cos(ang), std::sin(ang)};
            const Vec2 p = c + Vec2{g.range(-40, 40), g.range(-40, 40)};
            Vec2 a = p - d * 160, b = p + d * 160;
            // the full (dotted) line, itself clipped to the panel with Liang-Barsky
            Vec2 pa = a, pb = b;
            if (clipLiangBarsky(pa, pb, ClipRect{in.x + 4.0, in.y + 4.0, in.right() - 4.0, in.bottom() - 50.0}))
                drawStroke(img, pa, pb, Stroke::dotted(faded));
            if (clipCyrusBeck(a, b, diamond))
                drawCrease(img, a, b, i % 2 ? CreaseType::Mountain : CreaseType::Valley, pal, 2);
        }
        drawCaption(img, in.x + 12, in.bottom() - 34, "P(T) = A + T(B - A); N_I . (P(T) - E_I) = 0 PER EDGE.\nWORKS FOR ANY CONVEX SHEET, NOT JUST RECTANGLES");
    }

    // ---- 4: intersections = crease vertices ------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(3, 3, 2, img.width(), img.height(), top), "SEGMENT INTERSECTION = CREASE VERTICES");
        CreasePattern cp(Polygon::rectangle(0, 0, 400, 300));
        cp.addCrease({0, 0}, {400, 300}, CreaseType::Valley);
        cp.addCrease({0, 300}, {400, 0}, CreaseType::Valley);
        cp.addCrease({0, 150}, {400, 150}, CreaseType::Mountain);
        cp.addCrease({100, 0}, {100, 300}, CreaseType::Mountain);
        cp.addCrease({300, 0}, {300, 300}, CreaseType::Mountain);
        const Mat3 view = fitView(cp.paper.bounds(), Rect{in.x, in.y, in.w, in.h - 40}, 16);
        CreaseViewOptions o;
        o.showGrid = false;
        o.showVertices = false;
        drawCreasePattern(img, cp, view, pal, o);
        const auto xs = cp.intersections();
        for (const Vec2& p : xs) {
            const Vec2 s = view.apply(p);
            fillCircle(img, int(s.x), int(s.y), 5, theme::Accent);
            drawCircle(img, int(s.x), int(s.y), 5, ink);
        }
        drawCaption(img, in.x + 12, in.bottom() - 34,
                    "T = CROSS(Q - P, S) / CROSS(R, S), SAME FOR U;\nMEET WHEN 0 <= T, U <= 1:  " + std::to_string(xs.size()) + " VERTICES FOUND");
    }

    // ---- 5: reflect a line across a crease --------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(4, 3, 2, img.width(), img.height(), top), "REFLECTING A LINE ACROSS A CREASE");
        const Line crease = Line::through({in.x + 60.0, in.y + 230.0}, {in.x + 300.0, in.y + 20.0});
        drawCrease(img, crease.p, crease.p + crease.d, CreaseType::Valley, pal, 2);
        const Segment s{{in.x + 40.0, in.y + 60.0}, {in.x + 170.0, in.y + 20.0}};
        const Segment m{crease.reflect(s.a), crease.reflect(s.b)};
        drawStroke(img, s.a, s.b, Stroke::solid(theme::Accent, 3));
        drawStroke(img, m.a, m.b, Stroke::solid(Color::hex(0x2A9D8F), 3));
        for (const Vec2& p : {s.a, s.b}) {
            const Vec2 q = crease.reflect(p);
            drawStroke(img, p, q, Stroke::dotted(Color(150, 150, 150)));
            const Vec2 f = crease.project(p);
            img.fillRect(int(f.x) - 2, int(f.y) - 2, 5, 5, ink);
        }
        drawText(img, int(s.a.x), int(s.a.y) - 14, "SEGMENT", theme::Accent);
        drawText(img, int(m.b.x) - 10, int(m.b.y) + 8, "MIRROR IMAGE", Color::hex(0x2A9D8F));
        drawCaption(img, in.x + 12, in.bottom() - 34, "Q' = 2 * FOOT - Q WHERE FOOT = PROJECTION ON THE\nCREASE. A FOLD MOVES EVERY POINT THIS WAY");
    }

    // ---- 6: project: snapping -----------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(5, 3, 2, img.width(), img.height(), top), "PROJECT: VERTEX-THEN-GRID SNAPPING");
        CreasePattern cp(Polygon::rectangle(0, 0, 400, 300));
        cp.gridStep = 50;
        cp.addCrease({0, 0}, {400, 300}, CreaseType::Valley);
        cp.addCrease({0, 300}, {400, 0}, CreaseType::Mountain);
        const Mat3 view = fitView(cp.paper.bounds(), Rect{in.x, in.y, in.w, in.h - 40}, 16);
        drawCreasePattern(img, cp, view, pal);
        const Vec2 cursors[] = {{193, 140}, {62, 112}, {388, 12}, {270, 236}, {130, 40}};
        for (const Vec2& c : cursors) {
            const Vec2 snapped = cp.snap(c, 18.0);
            const bool vertex = !nearlyEqual(snapped, cp.snap(c, 0.0), 1e-9);
            const Vec2 a = view.apply(c), b = view.apply(snapped);
            drawStroke(img, a, b, Stroke::solid(ink));
            drawCircle(img, int(a.x), int(a.y), 3, ink);
            fillCircle(img, int(b.x), int(b.y), 5, vertex ? theme::Accent : Color::hex(0x2A9D8F));
        }
        drawText(img, in.x + 14, in.bottom() - 46, "O CURSOR", ink);
        fillCircle(img, in.x + 100, in.bottom() - 43, 4, theme::Accent);
        drawText(img, in.x + 108, in.bottom() - 46, "SNAPPED TO VERTEX", ink);
        fillCircle(img, in.x + 240, in.bottom() - 43, 4, Color::hex(0x2A9D8F));
        drawText(img, in.x + 248, in.bottom() - 46, "TO GRID", ink);
        drawCaption(img, in.x + 12, in.bottom() - 26, "VERTICES (CORNERS, ENDS, INTERSECTIONS) WIN WITHIN\n18 UNITS; OTHERWISE ROUND TO THE 50-UNIT GRID");
    }

    const std::string out = ctx.path("02_line_operations");
    img.save(out);
    return {out};
}

} // namespace ori
