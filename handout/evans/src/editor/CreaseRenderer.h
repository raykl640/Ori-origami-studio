// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : editor          Owner : Evans Mwendwa (Crease pattern editor)
//  File   : CreaseRenderer.h
//  Draws a crease pattern in origami notation with the project rasterizer:
//  paper fill (scanline), grid, mountain creases red dash-dot, valley creases
//  blue dashed, paper edge solid, vertex markers (midpoint circles).
//  `view` maps pattern coordinates to pixels (see fitView).
// ============================================================================
#pragma once
#include "../core/Image.h"
#include "../core/Layout.h"
#include "../geometry/Polygon.h"
#include "../math/Mat3.h"
#include "../shading/Palette.h"
#include "CreasePattern.h"

namespace ori {

struct CreaseViewOptions {
    bool fillPaper = true;
    bool showGrid = true;
    bool showVertices = true;
    bool showBorder = true;
    bool showAux = true;
    int creaseWidth = 2;
    double gridStep = 0.0;   // pattern units; 0 = use the pattern's own, else 1/8 of the paper
    int highlight = -1;      // crease index drawn thicker (selection)
};

// Uniform scale + translation that fits `world` centred inside `screen`
// with `margin` pixels to spare: T(screen centre) S(k) T(-world centre).
Mat3 fitView(const BBox& world, const Rect& screen, int margin = 10);

void drawCreasePattern(Image& img, const CreasePattern& cp, const Mat3& view, const Palette& pal,
                       const CreaseViewOptions& opt = {});

// One crease in notation style (used by the editor, fold diagrams and the UI).
void drawCrease(Image& img, const Vec2& a, const Vec2& b, CreaseType t, const Palette& pal, int width = 2);

// "--- MOUNTAIN  --- VALLEY  --- PAPER EDGE" key at (x, y).
void drawCreaseLegend(Image& img, int x, int y, const Palette& pal);

// Fill every face of the pattern by region (boundary) fill, alternating two
// colours: neighbouring faces of a flat-foldable pattern always differ.
// Returns the number of faces found.
int fillCreaseFaces(Image& img, const CreasePattern& cp, const Mat3& view, Color a, Color b);

} // namespace ori
