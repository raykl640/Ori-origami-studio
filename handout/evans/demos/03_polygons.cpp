// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demo 03 : Polygon representation and operations    Owner : Evans Mwendwa
//  Ordered vertex lists; shoelace area, perimeter, centroid; convexity;
//  even-odd point-in-polygon; matrix transforms of a panel; ear-clipping;
//  convex hull; splitting a panel along a crease.
// ============================================================================
#include "Demos.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/fold/FoldEngine.h"
#include "../src/geometry/Polygon.h"
#include "../src/raster/Circle.h"
#include "../src/raster/Fill.h"
#include "../src/raster/Lines.h"
#include <cmath>
#include <cstdio>

namespace ori {

namespace {

void outline(Image& img, const Polygon& p, Color c, int w = 2) { drawPolyline(img, p.v, Stroke::solid(c, w), true); }

void numberVertices(Image& img, const Polygon& p, Color c) {
    const Vec2 ctr = p.centroid();
    for (std::size_t i = 0; i < p.size(); ++i) {
        fillCircle(img, int(p.v[i].x), int(p.v[i].y), 3, c);
        const Vec2 out = p.v[i] + (p.v[i] - ctr).normalized() * 12.0;
        drawText(img, int(out.x) - 3, int(out.y) - 3, std::to_string(i), c);
    }
}

} // namespace

std::vector<std::string> demoPolygons(const DemoContext& ctx) {
    int top = 0;
    Image img = makeSheet("POLYGON REPRESENTATION AND OPERATIONS", "ORDERED VERTEX LISTS: AREA, CENTROID, CONVEXITY, INSIDE TEST, TRANSFORMS, TRIANGULATION, HULL, SPLIT", top);
    const Palette pal = Palette::classic();
    const Color ink = Color::hex(0x2B2B33);

    // ---- 1: representation + measures -------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(0, 3, 2, img.width(), img.height(), top), "ORDERED VERTEX LIST + SHOELACE");
        const double x = in.x + 60, y = in.y + 30;
        const Polygon p({{x, y + 40}, {x + 120, y}, {x + 250, y + 50}, {x + 200, y + 170}, {x + 90, y + 190}, {x + 20, y + 130}});
        fillPolygon(img, p.v, pal.paperFront.withAlpha(255));
        outline(img, p, ink);
        numberVertices(img, p, ink);
        const Vec2 c = p.centroid();
        drawStroke(img, c - Vec2{6, 0}, c + Vec2{6, 0}, Stroke::solid(colors::White, 2));
        drawStroke(img, c - Vec2{0, 6}, c + Vec2{0, 6}, Stroke::solid(colors::White, 2));
        char buf[220];
        std::snprintf(buf, sizeof buf, "AREA = 1/2 |SUM X_I Y_I+1 - X_I+1 Y_I| = %.0f\nPERIMETER = %.1f   CENTROID = (%.1f, %.1f)\nCONVEX: %s   ORDER: %s",
                      p.area(), p.perimeter(), c.x - in.x, c.y - in.y, p.isConvex() ? "YES" : "NO",
                      p.isCounterClockwise() ? "CLOCKWISE ON SCREEN" : "COUNTER-CLOCKWISE ON SCREEN");
        drawCaption(img, in.x + 12, in.bottom() - 45, buf);
    }

    // ---- 2: convexity + point in polygon ------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(1, 3, 2, img.width(), img.height(), top), "CONVEXITY + EVEN-ODD POINT IN POLYGON");
        const double x = in.x + 30, y = in.y + 25;
        // a kite-cone-like concave outline
        const Polygon p({{x + 20, y + 10}, {x + 300, y + 30}, {x + 180, y + 95}, {x + 310, y + 200}, {x + 40, y + 215}, {x + 110, y + 110}});
        fillPolygon(img, p.v, pal.paperBack);
        outline(img, p, ink);
        int inside = 0, total = 0;
        for (int j = 0; j < 14; ++j)
            for (int i = 0; i < 22; ++i) {
                const Vec2 q{x + 5 + i * 15.0, y + 3 + j * 17.0};
                const bool in_ = p.contains(q);
                inside += in_;
                ++total;
                if (in_) fillCircle(img, int(q.x), int(q.y), 3, theme::Accent);
                else drawCircle(img, int(q.x), int(q.y), 2, Color(170, 165, 158));
            }
        // the ray for one sample point
        const Vec2 s{x + 140, y + 160};
        drawStroke(img, s, {in.right() - 10.0, s.y}, Stroke::dashed(pal.valley));
        fillCircle(img, int(s.x), int(s.y), 5, pal.valley);
        char buf[200];
        std::snprintf(buf, sizeof buf, "CONVEX: %s (TURN DIRECTION CHANGES)\nRAY TO +X: ODD CROSSINGS = INSIDE. %d/%d SAMPLES IN", p.isConvex() ? "YES" : "NO", inside, total);
        drawCaption(img, in.x + 12, in.bottom() - 34, buf);
    }

    // ---- 3: transforms of a panel ---------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(2, 3, 2, img.width(), img.height(), top), "MATRIX TRANSFORMS OF A PANEL");
        const Vec2 o{in.x + 30.0, in.y + 25.0};
        const Polygon panel({{o.x, o.y}, {o.x + 80, o.y}, {o.x + 80, o.y + 50}, {o.x, o.y + 70}});
        const Vec2 mid = panel.centroid();
        const double mirrorY = in.y + 178.0;
        struct T { const char* name; Mat3 m; Color c; };
        const T ts[] = {
            {"ORIGINAL", Mat3{}, Color(160, 155, 150)},
            {"TRANSLATE", Mat3::translate(120, 0), Color::hex(0x2A9D8F)},
            {"ROTATE 30", Mat3::translate(245, 12) * Mat3::rotateAbout(degToRad(-30), mid), pal.valley},
            {"SCALE 1.4", Mat3::translate(0, 100) * Mat3::scaleAbout(1.4, 1.4, o), Color::hex(0x8E5BB5)},
            {"REFLECT", Mat3::reflectLine({0, mirrorY}, {1, mirrorY}) * Mat3::translate(190, 75), theme::Accent},
        };
        drawPolyline(img, panel.transformed(Mat3::translate(190, 75)).v, Stroke::dotted(Color(150, 150, 150)), true);
        for (const T& t : ts) {
            const Polygon q = panel.transformed(t.m);
            fillPolygonAlpha(img, q.v, t.c, 0.35);
            outline(img, q, t.c);
            const Vec2 c = q.centroid();
            drawText(img, int(c.x) - textWidth(t.name) / 2, int(c.y) - 3, t.name, ink);
        }
        drawStroke(img, {in.x + 180.0, mirrorY}, {in.right() - 10.0, mirrorY}, Stroke::dashed(pal.valley));
        drawCaption(img, in.x + 12, in.bottom() - 34, "EVERY VERTEX * MAT3 (HOMOGENEOUS). THE REFLECTED COPY\nHAS DET = -1: ITS VERTEX ORDER IS REVERSED");
    }

    // ---- 4: ear clipping -------------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(3, 3, 2, img.width(), img.height(), top), "EAR-CLIPPING TRIANGULATION");
        const double x = in.x + 30, y = in.y + 15;
        const Polygon crane({{x + 8, y + 70}, {x + 40, y + 52}, {x + 58, y + 64}, {x + 130, y + 150}, {x + 190, y + 15},
                             {x + 220, y + 135}, {x + 325, y + 62}, {x + 280, y + 165}, {x + 225, y + 200},
                             {x + 150, y + 196}, {x + 45, y + 78}});
        const auto tris = crane.triangulate();
        int i = 0;
        for (const auto& t : tris) {
            const std::vector<Vec2> tv(t.begin(), t.end());
            fillPolygon(img, tv, scale(pal.paperFront, 0.8 + 0.08 * (i++ % 5)));
            drawPolyline(img, tv, Stroke::smooth(colors::White), true);
        }
        outline(img, crane, ink);
        char buf[160];
        std::snprintf(buf, sizeof buf, "%d VERTICES -> %d TRIANGLES (N - 2). AN EAR: CONVEX\nCORNER WITH NO OTHER VERTEX INSIDE - CUT IT, REPEAT",
                      int(crane.size()), int(tris.size()));
        drawCaption(img, in.x + 12, in.bottom() - 34, buf);
    }

    // ---- 5: convex hull ------------------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(4, 3, 2, img.width(), img.height(), top), "CONVEX HULL (MONOTONE CHAIN)");
        std::vector<Vec2> pts;
        unsigned s = 77;
        for (int i = 0; i < 40; ++i) {
            s = s * 1103515245u + 12345u;
            const double a = (s >> 8) % 1000 / 1000.0 * 2 * kPi;
            s = s * 1103515245u + 12345u;
            const double r = 20 + (s >> 8) % 1000 / 1000.0 * 95;
            pts.push_back({in.cx() + r * std::cos(a) * 1.4, in.y + 120 + r * std::sin(a)});
        }
        const Polygon hull = convexHull(pts);
        fillPolygonAlpha(img, hull.v, pal.valley, 0.12);
        outline(img, hull, pal.valley);
        for (const Vec2& p : pts) fillCircle(img, int(p.x), int(p.y), 3, ink);
        for (const Vec2& p : hull.v) drawCircle(img, int(p.x), int(p.y), 6, theme::Accent);
        char buf[160];
        std::snprintf(buf, sizeof buf, "%d POINTS -> %d HULL VERTICES. SORT BY X, BUILD LOWER\nAND UPPER CHAINS, POP WHILE NOT TURNING LEFT", int(pts.size()), int(hull.size()));
        drawCaption(img, in.x + 12, in.bottom() - 34, buf);
    }

    // ---- 6: project: split a panel along a crease -------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(5, 3, 2, img.width(), img.height(), top), "PROJECT: SPLIT A PANEL ALONG A CREASE");
        const Vec2 c{in.x + 110.0, in.y + 115.0};
        const Polygon sheet({{c.x, c.y - 85}, {c.x + 85, c.y}, {c.x, c.y + 85}, {c.x - 85, c.y}});
        const Vec2 a{c.x - 110, c.y + 40}, b{c.x + 110, c.y - 30};
        const auto keep = FoldEngine::clipHalfPlane(sheet.v, b, a, +1);
        const auto move = FoldEngine::clipHalfPlane(sheet.v, b, a, -1);
        fillPolygon(img, keep, pal.paperBack);
        fillPolygon(img, move, pal.paperFront);
        outline(img, Polygon(keep), ink);
        outline(img, Polygon(move), ink);
        drawCrease(img, a, b, CreaseType::Valley, pal, 2);
        // the two halves pulled apart on the right
        const Vec2 shift{170, 0}, gapV = (b - a).perp().normalized() * 10.0;
        const Polygon k2 = Polygon(keep).translated(shift + gapV), m2 = Polygon(move).translated(shift - gapV);
        fillPolygon(img, k2.v, pal.paperBack);
        fillPolygon(img, m2.v, pal.paperFront);
        outline(img, k2, ink);
        outline(img, m2, ink);
        numberVertices(img, m2, ink);
        char buf[200];
        std::snprintf(buf, sizeof buf, "HALF-PLANE CLIP ON EACH SIDE OF THE CREASE:\n4 VERTICES -> %d + %d. AREAS %.0f + %.0f = %.0f",
                      int(keep.size()), int(move.size()), Polygon(keep).area(), Polygon(move).area(), sheet.area());
        drawCaption(img, in.x + 12, in.bottom() - 34, buf);
    }

    const std::string out = ctx.path("03_polygons");
    img.save(out);
    return {out};
}

} // namespace ori
