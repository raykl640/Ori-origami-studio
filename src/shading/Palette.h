// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : shading         Owner : Matthew Wachira (Colour, shading & curves)
//  File   : Palette.h
//  Colour system: paper colours (origami paper is coloured on the front and
//  white on the back), mountain / valley colour coding and depth-based
//  shading so stacked layers stay readable.
// ============================================================================
#pragma once
#include "../core/Image.h"
#include "../geometry/Polygon.h"
#include "../math/Vec2.h"
#include <string>

namespace ori {

struct Palette {
    std::string name;
    Color paperFront;   // coloured side of the kami
    Color paperBack;    // white side
    Color mountain;     // mountain crease colour (CP software convention: red)
    Color valley;       // valley crease colour (blue)
    Color border;       // paper outline
    Color outline;      // folded panel edges
    Color background;
    Color grid;

    static Palette classic();     // coral kami on warm studio grey
    static Palette indigo();      // indigo kami
    static Palette blueprint();   // high-contrast print / projector theme
};

// Depth shading: rank 0 is the top layer (full brightness); every level below
// moves `perLevel` further towards a warm shadow colour, capped at `maxDarken`. A fixed
// step per level keeps a 2-layer fold from looking as dark as a 10-layer one.
constexpr double kDepthPerLevel = 0.12;
constexpr double kDepthMax = 0.60;
Color depthShade(Color base, int rankFromTop, int layerCount,
                 double perLevel = kDepthPerLevel, double maxDarken = kDepthMax);

// Soft directional light across a panel - brighter towards `lightDir`.
// Returns a brightness factor in roughly [1 - amount, 1 + amount].
double lightingFactor(const Vec2& p, const BBox& box, const Vec2& lightDir, double amount = 0.07);

// Draws the "depth palette" UI swatch strip (region 5 of the studio UI).
void drawDepthPalette(Image& img, int x, int y, int w, int h, const Palette& pal, int layers);

} // namespace ori
