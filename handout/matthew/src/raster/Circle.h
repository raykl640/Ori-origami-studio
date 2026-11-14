// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : raster          Owner : Matthew Wachira (Colour, shading & curves)
//  File   : Circle.h
//  Midpoint circle (one octant computed, mirrored 8 ways) and midpoint
//  ellipse (region 1 where the slope is > -1, region 2 after it), with
//  filled, translucent and arc variants. In ORI they draw vertex markers,
//  selection / rotate handles and the dog character's eyes and nose.
// ============================================================================
#pragma once
#include "../core/Image.h"
#include "Lines.h"
#include <vector>

namespace ori {

// The points the midpoint algorithm computes for the octant from (0, r) to
// x = y, relative to the centre. Mirroring them 8 ways gives the full circle.
std::vector<PixelPoint> circleOctant(int r);

// The points of one ellipse quadrant (x >= 0, y >= 0) relative to the centre,
// in the order computed. `region1Count` receives how many came from region 1.
std::vector<PixelPoint> ellipseQuadrant(int rx, int ry, int* region1Count = nullptr);

void drawCircle(Image& img, int cx, int cy, int r, Color c);
void fillCircle(Image& img, int cx, int cy, int r, Color c);
// Translucent fill; every pixel is blended exactly once.
void fillCircleAlpha(Image& img, int cx, int cy, int r, Color c, double alpha);
// Ring of the given thickness (outer radius r).
void drawRing(Image& img, int cx, int cy, int r, int thickness, Color c);

void drawEllipse(Image& img, int cx, int cy, int rx, int ry, Color c);
void fillEllipse(Image& img, int cx, int cy, int rx, int ry, Color c);
void fillEllipseAlpha(Image& img, int cx, int cy, int rx, int ry, Color c, double alpha);

// Circular arc from angle a0 to a1 (radians, y down so positive = clockwise
// on screen), plotted from the midpoint circle's points.
void drawArc(Image& img, int cx, int cy, int r, double a0, double a1, Color c, int width = 1);

} // namespace ori
