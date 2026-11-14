// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : raster          Owner : Mathew Nguithi (Rasterizer)
//  File   : Fill.h
//  Polygon filling:
//    * Scanline fill with an Edge Table (edges bucketed by their first
//      scanline) and an Active Edge Table (edges crossing the current
//      scanline, x updated incrementally by 1/m). Even-odd rule, so holes and
//      self-intersections work; pixel centres are sampled at (x+.5, y+.5)
//      with half-open spans, which is the top-left rule: shared edges are
//      never painted twice.
//    * Seed fills: 4- and 8-connected flood fill and boundary fill, using an
//      explicit stack (no recursion, so large regions cannot overflow).
// ============================================================================
#pragma once
#include "../core/Image.h"
#include "../math/Vec2.h"
#include <functional>
#include <vector>

namespace ori {

using Contour = std::vector<Vec2>;

struct ScanlineStats {
    int edges = 0;        // non-horizontal edges placed in the Edge Table
    int scanlines = 0;    // scanlines that had at least one active edge
    int spans = 0;        // horizontal spans emitted
    long pixels = 0;      // pixels covered
};

// Core scanline algorithm: calls span(y, xStart, xEndExclusive) for every
// span inside the contours (even-odd). Rows are limited to [0, maxY) when
// maxY > 0 so off-screen geometry costs nothing.
ScanlineStats scanlineSpans(const std::vector<Contour>& contours,
                            const std::function<void(int y, int x0, int x1)>& span, int maxY = -1);

ScanlineStats fillPolygon(Image& img, const Contour& poly, Color c);
ScanlineStats fillPolygon(Image& img, const std::vector<Contour>& contours, Color c);
ScanlineStats fillPolygonAlpha(Image& img, const std::vector<Contour>& contours, Color c, double alpha);
inline ScanlineStats fillPolygonAlpha(Image& img, const Contour& poly, Color c, double alpha) {
    return fillPolygonAlpha(img, std::vector<Contour>{poly}, c, alpha);
}

// Per-pixel colour (depth / lighting gradients): shader(x, y) -> colour.
using PixelShader = std::function<Color(int x, int y)>;
ScanlineStats fillPolygonShaded(Image& img, const std::vector<Contour>& contours, const PixelShader& shader);

// Replace the connected region of the seed's colour with `fill`.
// Returns the number of pixels filled.
long floodFill(Image& img, int x, int y, Color fill, bool eightConnected = false);
// Fill outward from the seed until pixels of `boundary` colour are reached.
long boundaryFill(Image& img, int x, int y, Color fill, Color boundary, bool eightConnected = false);

} // namespace ori
