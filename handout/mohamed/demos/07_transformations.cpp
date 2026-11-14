// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demo 07 : 2D transformations (fold engine maths)    Owner : Mohamed Bile
//  Reflection across any line built as T(p) R(theta) S(1,-1) R(-theta) T(-p)
//  and checked against the closed form; two crossing creases = a rotation by
//  twice the angle; two parallel creases = a translation (a pleat); the
//  partial fold S(1, cos(pi t)); and det = +1 / -1 telling which side is up.
// ============================================================================
#include "Demos.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/fold/FoldEngine.h"
#include "../src/geometry/Line.h"
#include "../src/raster/Circle.h"
#include "../src/raster/Fill.h"
#include "../src/raster/Lines.h"
#include <cmath>
#include <cstdio>

namespace ori {

namespace {

// An asymmetric "flag" so mirror images are obvious.
Polygon flag(const Vec2& o, double k = 1.0) {
    return Polygon({o, o + Vec2{50, 0} * k, o + Vec2{38, 14} * k, o + Vec2{50, 28} * k, o + Vec2{12, 28} * k, o + Vec2{12, 60} * k, o + Vec2{0, 60} * k});
}

void drawFlag(Image& img, const Polygon& p, Color c, double alpha = 1.0) {
    if (alpha < 1.0) fillPolygonAlpha(img, p.v, c, alpha); else fillPolygon(img, p.v, c);
    drawPolyline(img, p.v, Stroke::smooth(Color::hex(0x2B2B33)), true);
}

void drawMatrix(Image& img, int x, int y, const Mat3& m, Color c) {
    for (int r = 0; r < 3; ++r) {
        char buf[64];
        std::snprintf(buf, sizeof buf, "%7.3F %7.3F %8.2F", m.m[r][0], m.m[r][1], m.m[r][2]);
        drawText(img, x, y + r * 11, buf, c);
    }
    img.fillRect(x - 5, y - 2, 1, 33, c);
    img.fillRect(x + textWidth("0000000 0000000 00000000") + 3, y - 2, 1, 33, c);
}

} // namespace

std::vector<std::string> demoTransformations(const DemoContext& ctx) {
    int top = 0;
    Image img = makeSheet("2D TRANSFORMATIONS: THE FOLD ENGINE MATHS", "HOMOGENEOUS 3X3 MATRICES - A FOLD IS A REFLECTION, A FOLD SEQUENCE IS A PRODUCT OF REFLECTIONS", top);
    const Palette pal = Palette::classic();
    const Color ink = Color::hex(0x2B2B33), purple = Color::hex(0x8E5BB5);

    // ---- 1: composition step by step ---------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(0, 3, 2, img.width(), img.height(), top), "REFLECTION BY COMPOSITION, STEP BY STEP");
        const Vec2 p{in.x + 50.0, in.y + 200.0}, q{in.x + 350.0, in.y + 100.0};
        const double th = (q - p).angle();
        drawCrease(img, p, q, CreaseType::Valley, pal, 2);
        const Polygon f = flag({in.x + 148.0, in.y + 115.0}, 0.7);
        // Each stage shown relative to the crease point p (drawn at p).
        const Mat3 stages[] = {
            Mat3{},
            Mat3::translate(p) * Mat3::rotate(-th) * Mat3::translate(-p),
            Mat3::translate(p) * Mat3::scale(1, -1) * Mat3::rotate(-th) * Mat3::translate(-p),
            Mat3::reflectLine(p, q),
        };
        const Color cs[] = {Color(170, 165, 158), Color::hex(0xE9C46A), purple, theme::Accent};
        const char* names[] = {"ORIGINAL", "R(-T): CREASE ON X-AXIS", "S(1,-1): MIRROR", "R(T) T(P): RESULT"};
        for (int i = 0; i < 4; ++i) {
            drawFlag(img, f.transformed(stages[i]), cs[i], i == 3 ? 1.0 : 0.45);
            img.fillRect(in.x + 14, in.y + 10 + i * 12, 8, 8, cs[i]);
            drawText(img, in.x + 28, in.y + 10 + i * 12, names[i], ink);
        }
        fillCircle(img, int(p.x), int(p.y), 4, ink);
        drawText(img, int(p.x) - 18, int(p.y) + 8, "P", ink);
        drawCaption(img, in.x + 12, in.bottom() - 34, "M = T(P) R(THETA) S(1,-1) R(-THETA) T(-P)\nRIGHT TO LEFT: MOVE, LEVEL, MIRROR, TURN BACK, MOVE BACK");
    }

    // ---- 2: composed vs closed form ---------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(1, 3, 2, img.width(), img.height(), top), "COMPOSED = CLOSED FORM (CHECKED AT RUN TIME)");
        const Vec2 a{40, 10}, b{140, 90};
        const Mat3 comp = Mat3::reflectLine(a, b), closed = Mat3::reflectLineClosedForm(a, b);
        drawText(img, in.x + 16, in.y + 14, "COMPOSED T R S R^-1 T^-1:", ink);
        drawMatrix(img, in.x + 30, in.y + 30, comp, theme::Accent);
        drawText(img, in.x + 16, in.y + 76, "CLOSED FORM [COS2T SIN2T; SIN2T -COS2T]:", ink);
        drawMatrix(img, in.x + 30, in.y + 92, closed, pal.valley);
        double maxDiff = 0;
        unsigned s = 9;
        for (int i = 0; i < 1000; ++i) {
            auto rnd = [&] { s = s * 1664525u + 1013904223u; return (s >> 8) % 20000 / 10.0 - 1000; };
            const Vec2 u{rnd(), rnd()}, v{rnd(), rnd()};
            if (distance(u, v) < 1e-3) continue;
            const Mat3 m1 = Mat3::reflectLine(u, v), m2 = Mat3::reflectLineClosedForm(u, v);
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 3; ++c) maxDiff = std::max(maxDiff, std::fabs(m1.m[r][c] - m2.m[r][c]));
        }
        char buf[260];
        std::snprintf(buf, sizeof buf, "DET = %.3F (A REFLECTION REVERSES ORIENTATION)\nM * M = IDENTITY: %s\n\n1000 RANDOM CREASES: MAX |COMPOSED - CLOSED|\n= %.1E", comp.determinant(),
                      (comp * comp).isIdentity(1e-9) ? "YES" : "NO", maxDiff);
        drawCaption(img, in.x + 16, in.y + 140, buf);
        drawCaption(img, in.x + 12, in.bottom() - 34, "THE ENGINE USES THE COMPOSED FORM; THE TESTS AND\nTHIS SHEET CHECK IT AGAINST THE CLOSED FORM");
    }

    // ---- 3: two crossing creases = rotation -------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(2, 3, 2, img.width(), img.height(), top), "TWO CROSSING CREASES = ROTATION BY 2 X ANGLE");
        const Vec2 c{in.x + 175.0, in.y + 125.0};
        const double alpha = degToRad(35);
        const Vec2 d1{1, 0}, d2{std::cos(alpha), -std::sin(alpha)};
        drawCrease(img, c - d1 * 160, c + d1 * 160, CreaseType::Valley, pal, 2);
        drawCrease(img, c - d2 * 150, c + d2 * 150, CreaseType::Mountain, pal, 2);
        const Polygon f = flag({c.x + 95, c.y + 18}, 0.65);
        const Mat3 r1 = Mat3::reflectLine(c, c + d1), r2 = Mat3::reflectLine(c, c + d2);
        drawFlag(img, f, Color(170, 165, 158), 0.6);
        drawFlag(img, f.transformed(r1), Color::hex(0xE9C46A), 0.6);
        drawFlag(img, f.transformed(r2 * r1), theme::Accent);
        const Mat3 rot = Mat3::rotateAbout(-2 * alpha, c);
        const bool same = (r2 * r1).nearlyEquals(rot, 1e-9);
        drawArc(img, int(c.x), int(c.y), 30, -alpha, 0, ink, 1);
        drawText(img, int(c.x) + 34, int(c.y) - 14, "35 DEG", ink);
        drawCaption(img, in.x + 12, in.bottom() - 34, std::string("REFLECT IN L1 THEN L2 = ROTATE 70 DEG ABOUT THE\nCROSSING. MATRIX CHECK: ") + (same ? "EQUAL" : "DIFFERENT"));
    }

    // ---- 4: two parallel creases = translation ---------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(3, 3, 2, img.width(), img.height(), top), "TWO PARALLEL CREASES = TRANSLATION (A PLEAT)");
        const double x1 = in.x + 150.0, x2 = in.x + 200.0;
        drawCrease(img, {x1, in.y + 10.0}, {x1, in.y + 230.0}, CreaseType::Valley, pal, 2);
        drawCrease(img, {x2, in.y + 10.0}, {x2, in.y + 230.0}, CreaseType::Mountain, pal, 2);
        const Polygon f = flag({in.x + 60.0, in.y + 80.0}, 1.0);
        const Mat3 r1 = Mat3::reflectLine({x1, 0}, {x1, 1}), r2 = Mat3::reflectLine({x2, 0}, {x2, 1});
        drawFlag(img, f, Color(170, 165, 158), 0.6);
        drawFlag(img, f.transformed(r1), Color::hex(0xE9C46A), 0.6);
        drawFlag(img, f.transformed(r2 * r1), theme::Accent);
        const bool same = (r2 * r1).nearlyEquals(Mat3::translate(2 * (x2 - x1), 0), 1e-9);
        drawStroke(img, {in.x + 60.0, in.y + 160.0}, {in.x + 160.0, in.y + 160.0}, Stroke::solid(ink));
        drawText(img, in.x + 70, in.y + 166, "2D = 100 PX", ink);
        drawCaption(img, in.x + 12, in.bottom() - 34, std::string("CREASES D = 50 APART: THE PRODUCT IS T(2D, 0).\nMATRIX CHECK: ") + (same ? "EQUAL" : "DIFFERENT"));
    }

    // ---- 5: partial fold ---------------------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(4, 3, 2, img.width(), img.height(), top), "PARTIAL FOLD M(T) = T R S(1, COS(PI T)) R^-1 T^-1");
        const Vec2 a{in.x + 180.0, in.y + 225.0}, b{in.x + 180.0, in.y + 15.0};
        const Polygon sheet = Polygon::rectangle(in.x + 60.0, in.y + 40.0, 240, 160);
        const auto stay = FoldEngine::clipHalfPlane(sheet.v, a, b, -1);
        const auto move = FoldEngine::clipHalfPlane(sheet.v, a, b, +1);
        fillPolygon(img, stay, pal.paperBack);
        drawPolyline(img, stay, Stroke::solid(ink), true);
        const double ts[] = {0.0, 0.25, 0.4, 0.6, 0.75, 1.0};
        for (double t : ts) {
            const Polygon m = Polygon(move).transformed(Mat3::partialFold(a, b, t));
            const Color c = t > 0.5 ? pal.paperFront : pal.paperBack;
            fillPolygonAlpha(img, m.v, c, 0.45);
            drawPolyline(img, m.v, Stroke::smooth(ink), true);
            char buf[16];
            std::snprintf(buf, sizeof buf, "%.2F", t);
            const Vec2 corner = m.v.size() > 1 ? Mat3::partialFold(a, b, t).apply(Vec2{in.x + 300.0, in.y + 40.0}) : a;
            drawText(img, int(corner.x) - 10, int(corner.y) - 12 - int(t * 4), buf, ink);
        }
        drawCrease(img, a, b, CreaseType::Valley, pal, 2);
        drawCaption(img, in.x + 12, in.bottom() - 34, "T = 0: FLAT. T = 0.5: UPRIGHT (DET 0). T = 1: THE FULL\nREFLECTION. PAST 0.5 THE FLAP SHOWS ITS OTHER SIDE");
    }

    // ---- 6: determinant sign = which side is up -----------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(5, 3, 2, img.width(), img.height(), top), "DET +1 / -1: WHICH SIDE OF THE PAPER FACES UP");
        const OrigamiModel napkin = napkinTriangleModel();
        const FoldState s = napkin.run().back();
        const Mat3 view = fitView(Polygon(s.sheet).bounds(), Rect{in.x, in.y, in.w - 120, in.h - 40}, 14);
        int up = 0, down = 0;
        for (const FoldPanel& p : s.panels) {
            std::vector<Vec2> o;
            for (const Vec2& v : p.sheetOutline()) o.push_back(view.apply(v));
            const double det = p.sheetToFolded.determinant();
            fillPolygon(img, o, det > 0 ? pal.paperFront : pal.paperBack);
            drawPolyline(img, o, Stroke::solid(ink), true);
            const Vec2 c = Polygon(o).centroid();
            drawText(img, int(c.x) - 8, int(c.y) - 3, det > 0 ? "+1" : "-1", ink);
            (det > 0 ? up : down)++;
        }
        const int lx = in.right() - 110;
        img.fillRect(lx, in.y + 40, 14, 14, pal.paperFront);
        drawText(img, lx + 20, in.y + 44, "DET +1", ink);
        img.fillRect(lx, in.y + 62, 14, 14, pal.paperBack);
        img.strokeRect(lx, in.y + 62, 14, 14, ink);
        drawText(img, lx + 20, in.y + 66, "DET -1", ink);
        drawCaption(img, lx, in.y + 90, std::to_string(up) + " PANELS\nUNCHANGED\n\n" + std::to_string(down) + " PANELS\nFLIPPED");
        drawCaption(img, in.x + 12, in.bottom() - 34, "NAPKIN TRIANGLE ON THE FLAT SHEET: EACH PANEL'S\nSHEET-TO-FOLDED MATRIX. ODD REFLECTIONS -> FACE DOWN");
    }

    const std::string out = ctx.path("07_transformations");
    img.save(out);
    return {out};
}

} // namespace ori
