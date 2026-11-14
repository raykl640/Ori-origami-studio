// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demo 01 : Line drawing                     Owner : Mathew Nguithi
//  DDA vs Bresenham pixel by pixel, Bresenham in all 8 octants, aliased vs
//  Xiaolin Wu, brush width and dash patterns - and the project use: crease
//  notation in the editor.
// ============================================================================
#include "Demos.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/raster/Lines.h"
#include <cmath>
#include <cstdio>

namespace ori {

namespace {

// Magnified pixel grid: cols x rows cells of `cell` px; `on` cells are filled.
void drawPixelGrid(Image& img, int x, int y, int cols, int rows, int cell,
                   const std::vector<PixelPoint>& on, Color c) {
    img.fillRect(x, y, cols * cell, rows * cell, Color(252, 251, 248));
    for (const PixelPoint& p : on)
        if (p.x >= 0 && p.y >= 0 && p.x < cols && p.y < rows)
            img.fillRect(x + p.x * cell + 1, y + p.y * cell + 1, cell - 1, cell - 1, c);
    const Color g(225, 220, 210);
    for (int i = 0; i <= cols; ++i) img.fillRect(x + i * cell, y, 1, rows * cell + 1, g);
    for (int j = 0; j <= rows; ++j) img.fillRect(x, y + j * cell, cols * cell + 1, 1, g);
}

// Upscale a small image into magnified cells.
void drawZoomed(Image& img, const Image& src, int x, int y, int cell) {
    for (int j = 0; j < src.height(); ++j)
        for (int i = 0; i < src.width(); ++i)
            img.fillRect(x + i * cell + 1, y + j * cell + 1, cell - 1, cell - 1, src.getPixel(i, j));
    const Color g(225, 220, 210);
    for (int i = 0; i <= src.width(); ++i) img.fillRect(x + i * cell, y, 1, src.height() * cell + 1, g);
    for (int j = 0; j <= src.height(); ++j) img.fillRect(x, y + j * cell, src.width() * cell + 1, 1, g);
}

} // namespace

std::vector<std::string> demoLines(const DemoContext& ctx) {
    int top = 0;
    Image img = makeSheet("LINE DRAWING", "DDA AND BRESENHAM SCAN CONVERSION, ANTI-ALIASING AND CREASE LINE STYLES", top);
    const Palette pal = Palette::classic();
    const Color ink = Color::hex(0x2B2B33);
    const int X0 = 1, Y0 = 2, X1 = 28, Y1 = 12;     // the demo line, in grid cells

    // ---- 1: DDA ---------------------------------------------------------------
    const auto dda = lineDDA(X0, Y0, X1, Y1);
    const auto bres = lineBresenham(X0, Y0, X1, Y1);
    {
        const Rect in = drawPanel(img, gridCell(0, 3, 2, img.width(), img.height(), top), "DDA - FLOATING POINT INCREMENTS");
        const int cell = 11, gx = in.x + 22, gy = in.y + 18;
        drawPixelGrid(img, gx, gy, 30, 15, cell, dda, Color::hex(0x2A9D8F));
        drawStroke(img, {gx + (X0 + .5) * cell, gy + (Y0 + .5) * cell}, {gx + (X1 + .5) * cell, gy + (Y1 + .5) * cell}, Stroke::smooth(ink));
        char buf[200];
        std::snprintf(buf, sizeof buf, "STEPS = MAX(|DX|, |DY|) = %d\nX += DX/STEPS, Y += DY/STEPS, ROUND EACH\n%d PIXELS PLOTTED, BLACK = IDEAL LINE\n\nUSED FOR: FAST PREVIEW OF A CREASE\nWHILE THE USER IS STILL DRAGGING IT",
                      std::max(std::abs(X1 - X0), std::abs(Y1 - Y0)), int(dda.size()));
        drawCaption(img, in.x + 12, in.bottom() - 80, buf);
    }

    // ---- 2: Bresenham -----------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(1, 3, 2, img.width(), img.height(), top), "BRESENHAM - INTEGER DECISIONS");
        const int cell = 11, gx = in.x + 22, gy = in.y + 18;
        drawPixelGrid(img, gx, gy, 30, 15, cell, bres, theme::Accent);
        drawStroke(img, {gx + (X0 + .5) * cell, gy + (Y0 + .5) * cell}, {gx + (X1 + .5) * cell, gy + (Y1 + .5) * cell}, Stroke::smooth(ink));
        int differ = 0;
        for (std::size_t i = 0; i < bres.size() && i < dda.size(); ++i) differ += bres[i] != dda[i];
        char buf[220];
        std::snprintf(buf, sizeof buf, "ERR = DX + DY;  E2 = 2*ERR\nIF E2 >= DY STEP X.   IF E2 <= DX STEP Y\n%d PIXELS, %d DIFFER FROM DDA (INTEGER ONLY)\n\nUSED FOR: EVERY CREASE AND PANEL EDGE\nIN THE FINAL RENDER (NO FLOATING POINT)",
                      int(bres.size()), differ);
        drawCaption(img, in.x + 12, in.bottom() - 80, buf);
    }

    // ---- 3: all octants -----------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(2, 3, 2, img.width(), img.height(), top), "BRESENHAM IN ALL 8 OCTANTS");
        const int cx = in.cx(), cy = in.y + 128, R = 112;
        for (int i = 0; i < 32; ++i) {
            const double a = 2 * kPi * i / 32.0 + 0.05;
            const int x1 = cx + int(std::lround(R * std::cos(a))), y1 = cy + int(std::lround(R * std::sin(a)));
            const Color c = lerp(pal.mountain, pal.valley, (i % 8) / 7.0);
            drawLine(img, cx, cy, x1, y1, c);
        }
        drawCaption(img, in.x + 12, in.bottom() - 34, "32 SPOKES: SX, SY = +-1 AND THE SHARED ERROR TERM\nHANDLE EVERY SLOPE WITHOUT SPECIAL CASES");
    }

    // ---- 4: aliased vs Wu -------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(3, 3, 2, img.width(), img.height(), top), "ALIASED (BRESENHAM) VS ANTI-ALIASED (WU)");
        for (int i = 0; i < 6; ++i) {
            const double y = in.y + 18 + i * 13;
            drawLine(img, in.x + 20, int(y), in.x + 165, int(y + 46), ink);
            drawLineWu(img, in.x + 200, y, in.x + 345, y + 46, ink);
        }
        Image a(16, 8, colors::White), w(16, 8, colors::White);
        drawLine(a, 0, 1, 15, 6, ink);
        drawLineWu(w, 0, 1, 15, 6, ink);
        drawZoomed(img, a, in.x + 20, in.y + 150, 9);
        drawZoomed(img, w, in.x + 200, in.y + 150, 9);
        drawCaption(img, in.x + 12, in.bottom() - 34, "STAIRCASE: 1 PIXEL/COLUMN     WU: COVERAGE SPLIT\n                              BETWEEN 2 PIXELS");
    }

    // ---- 5: stroke styles ---------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(4, 3, 2, img.width(), img.height(), top), "STROKE STYLES = ORIGAMI CREASE NOTATION");
        struct Row { const char* name; const char* style; Stroke s; };
        const Row rows[] = {
            {"PAPER EDGE", "SOLID W2", Stroke::solid(pal.border, 2)},
            {"VALLEY", "DASHED", Stroke::dashed(pal.valley, 2)},
            {"MOUNTAIN", "DASH-DOT", Stroke::dashDot(pal.mountain, 2)},
            {"REFERENCE", "DOTTED", Stroke::dotted(Color(150, 150, 150))},
        };
        int y = in.y + 20;
        for (const Row& r : rows) {
            drawText(img, in.x + 16, y - 3, r.name, ink);
            drawText(img, in.x + 100, y - 3, r.style, theme::Caption);
            drawStroke(img, {in.x + 180.0, double(y)}, {in.right() - 24.0, double(y)}, r.s);
            y += 32;
        }
        drawText(img, in.x + 16, y - 3, "BRUSH WIDTH 1-6", ink);
        for (int wdt = 1; wdt <= 6; ++wdt) {
            const double x = in.x + 150.0 + wdt * 30;
            drawStroke(img, {x, y + 22.0}, {x + 14, y - 12.0}, Stroke::solid(ink, wdt));
        }
        drawCaption(img, in.x + 12, in.bottom() - 34, "DASH PATTERN = ON/OFF RUN LENGTHS COUNTED PER\nBRESENHAM STEP; WIDTH = ROUND BRUSH STAMP");
    }

    // ---- 6: project: preliminary base -----------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(5, 3, 2, img.width(), img.height(), top), "PROJECT: PRELIMINARY-BASE CREASE PATTERN");
        CreasePattern cp(Polygon::rectangle(0, 0, 400, 400), "preliminary base");
        cp.addCrease({0, 0}, {400, 400}, CreaseType::Valley);
        cp.addCrease({400, 0}, {0, 400}, CreaseType::Valley);
        cp.addCrease({0, 200}, {400, 200}, CreaseType::Mountain);
        cp.addCrease({200, 0}, {200, 400}, CreaseType::Mountain);
        const Mat3 view = fitView(cp.paper.bounds(), Rect{in.x, in.y, in.w, in.h - 26}, 12);
        CreaseViewOptions o;
        o.showGrid = true;
        drawCreasePattern(img, cp, view, pal, o);
        drawCreaseLegend(img, in.x + 20, in.bottom() - 16, pal);
    }

    const std::string out = ctx.path("01_lines");
    img.save(out);
    return {out};
}

} // namespace ori
