// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : raster          Owner : Mathew Nguithi (Rasterizer)
//  File   : Lines.h
//  Line scan conversion written from scratch:
//    * DDA        - floating point increments (x += dx/steps, y += dy/steps)
//    * Bresenham  - integer-only decisions, every octant
//    * Xiaolin Wu - anti-aliased line, coverage split between two pixels
//  plus styled strokes (brush width + dash patterns) that encode origami
//  crease notation: paper edge = solid, valley = dashed, mountain = dash-dot.
// ============================================================================
#pragma once
#include "../core/Image.h"
#include "../math/Vec2.h"
#include <vector>

namespace ori {

struct PixelPoint {
    int x = 0, y = 0;
    bool operator==(const PixelPoint& o) const { return x == o.x && y == o.y; }
    bool operator!=(const PixelPoint& o) const { return !(*this == o); }
};

// ---- scan conversion: the pixels a line covers, in drawing order ---------
std::vector<PixelPoint> lineDDA(int x0, int y0, int x1, int y1);
std::vector<PixelPoint> lineBresenham(int x0, int y0, int x1, int y1);

// ---- plotting -------------------------------------------------------------
void drawLineDDA(Image& img, int x0, int y0, int x1, int y1, Color c);
void drawLine(Image& img, int x0, int y0, int x1, int y1, Color c);          // Bresenham
void drawLineWu(Image& img, double x0, double y0, double x1, double y1, Color c);

// Fixed-pattern crease lines on top of Bresenham (pattern counted per pixel step).
void drawValleyCrease(Image& img, int x0, int y0, int x1, int y1, Color c, int dash = 5, int gap = 5);
void drawMountainCrease(Image& img, int x0, int y0, int x1, int y1, Color c, int dash = 5, int gap = 3, int dot = 1);

// ---- styled strokes -------------------------------------------------------
// `pattern` holds alternating on / off run lengths in pixel steps; empty means
// solid. Width > 1 stamps a round brush at every Bresenham step.
struct Stroke {
    Color color = colors::Black;
    int width = 1;
    std::vector<int> pattern;
    bool antialias = false;   // width-1 solid strokes only: use Xiaolin Wu

    static Stroke solid(Color c, int width = 1);
    static Stroke dashed(Color c, int width = 1, int dash = 8, int gap = 5);
    static Stroke dotted(Color c, int width = 1);
    static Stroke dashDot(Color c, int width = 1);
    static Stroke smooth(Color c);                      // anti-aliased hairline
    Stroke withWidth(int w) const { Stroke s = *this; s.width = w; return s; }
};

// Stamp a filled round brush of the given width centred on (x, y).
void stampBrush(Image& img, int x, int y, int width, Color c);

void drawStroke(Image& img, const Vec2& a, const Vec2& b, const Stroke& s);
// The dash pattern continues across the joints, so a dashed curve made of
// many short segments still looks evenly dashed.
void drawPolyline(Image& img, const std::vector<Vec2>& pts, const Stroke& s, bool closed = false);

} // namespace ori
