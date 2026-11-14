// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demo 05 : Circles and ellipses              Owner : Matthew Wachira
//  Midpoint circle (one octant mirrored 8 ways), midpoint ellipse (regions
//  1 and 2), filled / translucent shapes and arcs - and the project uses:
//  vertex markers, selection and rotate handles, the dog's eyes and nose.
// ============================================================================
#include "Demos.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/layering/LayerRenderer.h"
#include "../src/raster/Circle.h"
#include "../src/raster/Fill.h"
#include "../src/raster/Lines.h"
#include <cmath>
#include <cstdio>

namespace ori {

namespace {

void gridCells(Image& img, int x, int y, int n, int cell) {
    const Color g(228, 223, 214);
    for (int i = 0; i <= n; ++i) {
        img.fillRect(x + i * cell, y, 1, n * cell + 1, g);
        img.fillRect(x, y + i * cell, n * cell + 1, 1, g);
    }
}

} // namespace

std::vector<std::string> demoCirclesEllipses(const DemoContext& ctx) {
    int top = 0;
    Image img = makeSheet("CIRCLES AND ELLIPSES", "MIDPOINT CIRCLE (8-WAY SYMMETRY), MIDPOINT ELLIPSE (TWO REGIONS), FILLS, ALPHA AND ARCS", top);
    const Palette pal = Palette::classic();
    const Color ink = Color::hex(0x2B2B33);
    const Color oct[8] = {theme::Accent, Color::hex(0xF2A65A), Color::hex(0xE9C46A), Color::hex(0x8CC084),
                          Color::hex(0x2A9D8F), pal.valley, Color::hex(0x3D4E9E), Color::hex(0x8E5BB5)};

    // ---- 1: one octant mirrored 8 ways ---------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(0, 3, 2, img.width(), img.height(), top), "MIDPOINT CIRCLE: 1 OCTANT, 8-WAY SYMMETRY");
        const int r = 11, cell = 10, n = 2 * r + 3;
        const int gx = in.x + (in.w - n * cell) / 2, gy = in.y + 10;
        img.fillRect(gx, gy, n * cell, n * cell, Color(252, 251, 248));
        gridCells(img, gx, gy, n, cell);
        const int c = r + 1;
        for (const PixelPoint& p : circleOctant(r)) {
            const int pts[8][2] = {{p.x, p.y}, {p.y, p.x}, {p.y, -p.x}, {p.x, -p.y}, {-p.x, -p.y}, {-p.y, -p.x}, {-p.y, p.x}, {-p.x, p.y}};
            for (int k = 0; k < 8; ++k)
                img.fillRect(gx + (c + pts[k][0]) * cell + 1, gy + (c + pts[k][1]) * cell + 1, cell - 1, cell - 1, oct[k]);
        }
        img.strokeRect(gx + c * cell, gy + c * cell, cell + 1, cell + 1, ink);
        char buf[200];
        std::snprintf(buf, sizeof buf, "R = %d: %d POINTS COMPUTED (ORANGE), 7 MIRRORS.\nD = 1 - R; D < 0 ? D += 2X + 3 : D += 2(X - Y) + 5", r, int(circleOctant(r).size()));
        drawCaption(img, in.x + 12, in.bottom() - 34, buf);
    }

    // ---- 2: midpoint ellipse regions ----------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(1, 3, 2, img.width(), img.height(), top), "MIDPOINT ELLIPSE: REGION 1 AND REGION 2");
        const int rx = 15, ry = 9, cell = 9;
        const int nx = 2 * rx + 3, ny = 2 * ry + 3;
        const int gx = in.x + (in.w - nx * cell) / 2, gy = in.y + 20;
        img.fillRect(gx, gy, nx * cell, ny * cell, Color(252, 251, 248));
        const Color g(228, 223, 214);
        for (int i = 0; i <= nx; ++i) img.fillRect(gx + i * cell, gy, 1, ny * cell + 1, g);
        for (int j = 0; j <= ny; ++j) img.fillRect(gx, gy + j * cell, nx * cell + 1, 1, g);
        int r1 = 0;
        const auto q = ellipseQuadrant(rx, ry, &r1);
        for (std::size_t i = 0; i < q.size(); ++i) {
            const Color c = int(i) < r1 ? theme::Accent : pal.valley;
            const int sx[4] = {1, -1, 1, -1}, sy[4] = {1, 1, -1, -1};
            for (int k = 0; k < 4; ++k)
                img.fillRect(gx + (rx + 1 + sx[k] * q[i].x) * cell + 1, gy + (ry + 1 + sy[k] * q[i].y) * cell + 1, cell - 1, cell - 1, c);
        }
        drawText(img, in.x + 20, in.y + 220, "REGION 1: SLOPE > -1, STEP X", theme::Accent);
        drawText(img, in.x + 20, in.y + 232, "REGION 2: SLOPE < -1, STEP Y", pal.valley);
        char buf[160];
        std::snprintf(buf, sizeof buf, "RX = %d, RY = %d: %d + %d POINTS PER QUADRANT.\nSWITCH WHEN 2 RY^2 X >= 2 RX^2 Y", rx, ry, r1, int(q.size()) - r1);
        drawCaption(img, in.x + 12, in.bottom() - 34, buf);
    }

    // ---- 3: filled + alpha ----------------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(2, 3, 2, img.width(), img.height(), top), "FILLED AND ALPHA-BLENDED SHAPES");
        const int cx = in.cx(), cy = in.y + 115;
        fillCircleAlpha(img, cx - 45, cy - 25, 62, theme::Accent, 0.55);
        fillCircleAlpha(img, cx + 45, cy - 25, 62, pal.valley, 0.55);
        fillCircleAlpha(img, cx, cy + 45, 62, Color::hex(0xE9C46A), 0.55);
        fillEllipse(img, in.x + 50, in.y + 210, 30, 12, ink);
        fillEllipseAlpha(img, in.right() - 60, in.y + 210, 40, 16, Color::hex(0x2A9D8F), 0.5);
        drawEllipse(img, in.right() - 60, in.y + 210, 40, 16, ink);
        drawCaption(img, in.x + 12, in.bottom() - 34, "FILL = HORIZONTAL SPANS BETWEEN MIRRORED POINTS,\nEACH PIXEL BLENDED ONCE: C = A*SRC + (1-A)*DST");
    }

    // ---- 4: arcs and rings ------------------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(3, 3, 2, img.width(), img.height(), top), "ARCS AND RINGS");
        const int cx = in.x + 100, cy = in.y + 115;
        for (int i = 0; i < 5; ++i) drawArc(img, cx, cy, 30 + i * 15, -kPi / 2, -kPi / 2 + (i + 1) * kPi / 3, oct[i], 2);
        drawRing(img, in.right() - 95, cy, 70, 10, pal.paperFront);
        drawRing(img, in.right() - 95, cy, 50, 4, pal.valley);
        drawCircle(img, in.right() - 95, cy, 30, ink);
        drawCaption(img, in.x + 12, in.bottom() - 34, "ARC = MIDPOINT POINTS FILTERED BY ANGLE.\nRING = SPANS OF THE OUTER CIRCLE MINUS THE INNER ONE");
    }

    // ---- 5: project: editor handles ---------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(4, 3, 2, img.width(), img.height(), top), "PROJECT: VERTEX MARKERS AND HANDLES");
        CreasePattern cp(Polygon({{200, 0}, {400, 200}, {200, 400}, {0, 200}}));
        cp.addCrease({0, 200}, {400, 200}, CreaseType::Valley);
        cp.addCrease({100, 100}, {300, 300}, CreaseType::Mountain);
        const Mat3 view = fitView(cp.paper.bounds(), Rect{in.x, in.y, in.w, in.h - 40}, 14);
        CreaseViewOptions o;
        o.highlight = 1;
        drawCreasePattern(img, cp, view, pal, o);
        // selection: handles at the selected crease ends + a rotate handle
        const Vec2 a = view.apply(cp.creases[1].a), b = view.apply(cp.creases[1].b);
        for (const Vec2& p : {a, b}) { fillCircle(img, int(p.x), int(p.y), 7, colors::White); drawRing(img, int(p.x), int(p.y), 7, 2, theme::Accent); }
        const Vec2 mid = (a + b) * 0.5;
        drawArc(img, int(mid.x), int(mid.y), 26, -kPi * 0.95, -kPi * 0.55, Color::hex(0x8E5BB5), 2);
        fillCircleAlpha(img, int(mid.x), int(mid.y), 26, Color::hex(0x8E5BB5), 0.12);
        drawCaption(img, in.x + 12, in.bottom() - 34, "WHITE DOTS: VERTICES. CORAL RINGS: ENDS OF THE\nSELECTED CREASE. PURPLE ARC: ROTATE HANDLE");
    }

    // ---- 6: project: dog face eyes and nose ------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(5, 3, 2, img.width(), img.height(), top), "PROJECT: THE DOG'S EYES AND NOSE");
        const OrigamiModel dog = dogFaceModel();
        RenderOptions opt;
        opt.whiteSideUp = dog.whiteSideUp;
        const auto states = dog.run();
        const Mat3 view = drawFoldStateFitted(img, states.back(), Rect{in.x, in.y, in.w, in.h - 40}, pal, opt, 12);
        drawDogFeatures(img, view);
        // call-outs
        const Vec2 eye = view.apply(Vec2{236, 252}), nose = view.apply(Vec2{200, 308});
        drawStroke(img, eye, {in.right() - 70.0, in.y + 30.0}, Stroke::solid(ink));
        drawText(img, in.right() - 105, in.y + 18, "CIRCLE R=10", ink);
        drawStroke(img, nose, {in.right() - 60.0, in.y + 225.0}, Stroke::solid(ink));
        drawText(img, in.right() - 120, in.y + 230, "ELLIPSE 13 X 9", ink);
        drawCaption(img, in.x + 12, in.bottom() - 34, "EYES: FILLED MIDPOINT CIRCLES + HIGHLIGHT.\nNOSE: FILLED MIDPOINT ELLIPSE (SCALED WITH THE VIEW)");
    }

    const std::string out = ctx.path("05_circles_ellipses");
    img.save(out);
    return {out};
}

} // namespace ori
