// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demo 04 : Polygon clipping and filling     Owner : Mathew Nguithi
//  Sutherland-Hodgman (with each of its 4 stages), scanline fill with an
//  Edge Table / Active Edge Table, even-odd holes and self-intersection,
//  4- vs 8-connected flood fill and boundary fill, and region fill that
//  2-colours the faces of a crease pattern.
// ============================================================================
#include "Demos.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/raster/Circle.h"
#include "../src/raster/Clipping.h"
#include "../src/raster/Fill.h"
#include "../src/raster/Lines.h"
#include <cmath>
#include <cstdio>

namespace ori {

namespace {

std::vector<Vec2> flapShape(const Vec2& o, double k) {
    // A folded flap that sticks out of the sheet on several sides (7 vertices).
    const std::vector<Vec2> base = {{-20, 30}, {60, -40}, {95, 40}, {175, 10}, {120, 110}, {150, 190}, {30, 150}};
    std::vector<Vec2> out;
    for (const Vec2& p : base) out.push_back(o + p * k);
    return out;
}

void zoomCells(Image& img, const Image& src, int x, int y, int cell) {
    for (int j = 0; j < src.height(); ++j)
        for (int i = 0; i < src.width(); ++i)
            img.fillRect(x + i * cell, y + j * cell, cell - 1, cell - 1, src.getPixel(i, j));
}

} // namespace

std::vector<std::string> demoClippingFilling(const DemoContext& ctx) {
    int top = 0;
    Image img = makeSheet("POLYGON CLIPPING AND FILLING", "SUTHERLAND-HODGMAN, SCANLINE (ET/AET), EVEN-ODD, BOUNDARY AND FLOOD FILL", top);
    const Palette pal = Palette::classic();
    const Color ink = Color::hex(0x2B2B33);

    // ---- 1: Sutherland-Hodgman --------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(0, 3, 2, img.width(), img.height(), top), "SUTHERLAND-HODGMAN: CLIP A FLAP TO THE SHEET");
        const ClipRect sheet{in.x + 90.0, in.y + 50.0, in.x + 290.0, in.y + 225.0};
        img.fillRect(int(sheet.xmin), int(sheet.ymin), int(sheet.xmax - sheet.xmin), int(sheet.ymax - sheet.ymin), pal.paperBack);
        const auto flap = flapShape({in.x + 85.0, in.y + 62.0}, 1.05);
        const auto clipped = clipPolygonToRect(flap, sheet);
        fillPolygon(img, clipped, pal.paperFront);
        drawPolyline(img, flap, Stroke::dashed(Color(150, 150, 150)), true);
        drawPolyline(img, clipped, Stroke::solid(ink), true);
        drawPolyline(img, sheet.polygon(), Stroke::solid(ink, 2), true);
        char buf[160];
        std::snprintf(buf, sizeof buf, "SUBJECT %d VERTICES -> CLIPPED %d VERTICES\nCLIP AGAINST EACH WINDOW EDGE IN TURN", int(flap.size()), int(clipped.size()));
        drawCaption(img, in.x + 12, in.bottom() - 34, buf);
    }

    // ---- 2: the four stages -----------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(1, 3, 2, img.width(), img.height(), top), "THE FOUR CLIPPING STAGES");
        const char* names[4] = {"1. TOP EDGE", "2. RIGHT EDGE", "3. BOTTOM EDGE", "4. LEFT EDGE"};
        for (int k = 0; k < 4; ++k) {
            const int ox = in.x + 15 + (k % 2) * 180, oy = in.y + 8 + (k / 2) * 125;
            const ClipRect win{ox + 40.0, oy + 22.0, ox + 140.0, oy + 105.0};
            const auto flap = flapShape({ox + 35.0, oy + 18.0}, 0.55);
            const auto stages = sutherlandHodgmanStages(flap, win.polygon());
            fillPolygon(img, stages[std::size_t(k)], pal.paperFront);
            drawPolyline(img, stages[std::size_t(k)], Stroke::solid(ink), true);
            const auto w = win.polygon();
            drawStroke(img, w[std::size_t(k)], w[std::size_t((k + 1) % 4)], Stroke::solid(pal.valley, 3));
            drawText(img, ox, oy, std::string(names[k]) + " (" + std::to_string(stages[std::size_t(k)].size()) + ")", ink);
        }
        drawCaption(img, in.x + 12, in.bottom() - 34, "IN->IN: KEEP NEXT     IN->OUT: KEEP INTERSECTION\nOUT->IN: INTERSECTION + NEXT     OUT->OUT: NOTHING");
    }

    // ---- 3: scanline ET/AET --------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(2, 3, 2, img.width(), img.height(), top), "SCANLINE FILL: EDGE TABLE + ACTIVE EDGE TABLE");
        const double x = in.x + 25, y = in.y + 10;
        const std::vector<Vec2> crane = {{x + 8, y + 85}, {x + 40, y + 70}, {x + 58, y + 82}, {x + 120, y + 150}, {x + 185, y + 15},
                                         {x + 220, y + 140}, {x + 320, y + 50}, {x + 280, y + 170}, {x + 225, y + 215},
                                         {x + 150, y + 210}, {x + 45, y + 96}};
        const Color c0 = Color::hex(0xF2A65A), c1 = Color::hex(0xD9481E);
        const auto st = fillPolygonShaded(img, {crane}, [&](int, int py) { return lerp(c0, c1, (py - y) / 215.0); });
        drawPolyline(img, crane, Stroke::smooth(ink), true);
        // the scanline with the most active edges, and its crossings
        auto crossings = [&](double sy) {
            std::vector<double> xs;
            for (std::size_t i = 0; i < crane.size(); ++i) {
                const Vec2 a = crane[i], b = crane[(i + 1) % crane.size()];
                if ((a.y <= sy) != (b.y <= sy)) xs.push_back(a.x + (sy - a.y) * (b.x - a.x) / (b.y - a.y));
            }
            return xs;
        };
        double sy = y + 0.5;
        for (double t = y + 0.5; t < y + 215; t += 1.0)
            if (crossings(t).size() > crossings(sy).size()) sy = t;
        const auto xs = crossings(sy);
        drawStroke(img, {in.x + 5.0, sy}, {in.right() - 5.0, sy}, Stroke::dotted(pal.valley));
        for (double cx : xs) { fillCircle(img, int(cx), int(sy), 4, colors::White); drawCircle(img, int(cx), int(sy), 4, pal.valley); }
        char buf[220];
        std::snprintf(buf, sizeof buf, "%d EDGES IN THE ET, %d SPANS ON %d SCANLINES.\nBLUE SCANLINE CROSSES %d EDGES -> %d SPAN(S). SORT BY X,\nFILL PAIRS, THEN X += 1/M ON EVERY ACTIVE EDGE",
                      st.edges, st.spans, st.scanlines, int(xs.size()), int(xs.size() / 2));
        drawCaption(img, in.x + 12, in.bottom() - 45, buf);
    }

    // ---- 4: even-odd: holes and self-intersection --------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(3, 3, 2, img.width(), img.height(), top), "EVEN-ODD RULE: HOLES AND SELF-INTERSECTION");
        const Vec2 c{in.x + 95.0, in.y + 105.0};
        const Contour outer = {{c.x - 70, c.y - 70}, {c.x + 70, c.y - 70}, {c.x + 70, c.y + 70}, {c.x - 70, c.y + 70}};
        const Contour hole = Polygon::regular(6, c, 38).v;
        fillPolygon(img, std::vector<Contour>{outer, hole}, pal.paperFront);
        drawText(img, int(c.x) - 52, int(c.y) + 85, "KIRIGAMI WINDOW", ink);
        const Vec2 s{in.x + 270.0, in.y + 105.0};
        Contour star;
        for (int i = 0; i < 5; ++i) {
            const double a = -kPi / 2 + i * 4 * kPi / 5;
            star.push_back({s.x + 75 * std::cos(a), s.y + 75 * std::sin(a)});
        }
        fillPolygon(img, star, Color::hex(0x3D4E9E));
        drawPolyline(img, star, Stroke::smooth(ink), true);
        drawText(img, int(s.x) - 28, int(s.y) + 85, "PENTAGRAM", ink);
        drawCaption(img, in.x + 12, in.bottom() - 34, "A PIXEL IS INSIDE WHEN A RAY CROSSES AN ODD NUMBER\nOF EDGES: SO HOLES AND THE STAR CENTRE STAY EMPTY");
    }

    // ---- 5: flood fill 4 vs 8, boundary fill ---------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(4, 3, 2, img.width(), img.height(), top), "SEED FILLS: 4-CONNECTED VS 8-CONNECTED");
        const Color line = ink, bg = Color(252, 251, 248);
        auto base = [&] {
            Image s(19, 19, bg);
            drawLine(s, 9, 1, 17, 9, line); drawLine(s, 17, 9, 9, 17, line);
            drawLine(s, 9, 17, 1, 9, line); drawLine(s, 1, 9, 9, 1, line);
            return s;
        };
        Image f4 = base(), f8 = base(), b4 = base();
        const long n4 = floodFill(f4, 9, 9, Color::hex(0x8CC084), false);
        const long n8 = floodFill(f8, 9, 9, Color::hex(0xE4572E), true);
        const long nb = boundaryFill(b4, 9, 9, Color::hex(0xF2A65A), line, false);
        const int cell = 6;
        const char* labels[3] = {"FLOOD 4", "FLOOD 8", "BOUNDARY 4"};
        const Image* imgs[3] = {&f4, &f8, &b4};
        const long counts[3] = {n4, n8, nb};
        for (int k = 0; k < 3; ++k) {
            const int ox = in.x + 15 + k * 122, oy = in.y + 22;
            drawText(img, ox, oy - 12, labels[k], ink);
            zoomCells(img, *imgs[k], ox, oy, cell);
            fillCircle(img, ox + 9 * cell + cell / 2, oy + 9 * cell + cell / 2, 4, colors::White);
            drawCircle(img, ox + 9 * cell + cell / 2, oy + 9 * cell + cell / 2, 4, ink);
            const std::string t = std::to_string(counts[k]) + " PX" + (k == 1 ? " - LEAKS" : "");
            drawText(img, ox, oy + 19 * cell + 6, t, k == 1 ? pal.mountain : theme::Caption);
        }
        drawCaption(img, in.x + 12, in.y + 180, "BRESENHAM DIAGONALS TOUCH ONLY AT CORNERS.\n8-CONNECTED FILL SLIPS THROUGH THOSE GAPS,\n4-CONNECTED FILL CANNOT.\n\nEXPLICIT STACK, NOT RECURSION: NO STACK OVERFLOW\nON LARGE REGIONS.");
    }

    // ---- 6: project: 2-colouring crease pattern faces ----------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(5, 3, 2, img.width(), img.height(), top), "REGION FILL: 2-COLOURING CREASE PATTERN FACES");
        CreasePattern cp(Polygon::rectangle(0, 0, 400, 400));
        cp.addCrease({0, 0}, {400, 400}, CreaseType::Valley);
        cp.addCrease({400, 0}, {0, 400}, CreaseType::Valley);
        cp.addCrease({0, 200}, {400, 200}, CreaseType::Mountain);
        cp.addCrease({200, 0}, {200, 400}, CreaseType::Mountain);
        const Mat3 view = fitView(cp.paper.bounds(), Rect{in.x, in.y, in.w, in.h - 40}, 12);
        const int faces = fillCreaseFaces(img, cp, view, pal.paperFront, pal.paperBack);
        for (const Crease& c : cp.creases) drawStroke(img, view.apply(c.a), view.apply(c.b), Stroke::solid(ink, 2));
        drawPolyline(img, cp.paper.transformed(view).v, Stroke::solid(ink, 2), true);
        drawCaption(img, in.x + 12, in.bottom() - 34, std::to_string(faces) + " FACES, ONE SEED EACH. NEIGHBOURS ALWAYS DIFFER:\nFLAT-FOLDED FACES ALTERNATE FRONT/BACK UP");
    }

    const std::string out = ctx.path("04_clipping_filling");
    img.save(out);
    return {out};
}

} // namespace ori
