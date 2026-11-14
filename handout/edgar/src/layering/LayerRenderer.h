// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : layering        Owner : Edgar Kareithi (Layering, panels & studio UI)
//  File   : LayerRenderer.h
//  Turns a folded stack of panels into pixels:
//    * Painter's algorithm - panels sorted by layer, drawn bottom first, so
//      later (higher) panels overwrite what they cover.
//    * Depth shading      - every layer below the top is darkened a step.
//    * Lighting gradient  - per-pixel brightness across the model.
//    * Translucent X-ray  - each panel alpha-blended: C = a*panel + (1-a)*below.
//    * Exploded view      - isometric S(1, .5) R(45 deg), each layer lifted.
//    * Layer inspector    - one thumbnail per layer over a ghost of the stack.
//    * Fold diagrams      - the next crease plus a Bezier fold arrow.
// ============================================================================
#pragma once
#include "../core/Image.h"
#include "../core/Layout.h"
#include "../editor/CreaseRenderer.h"   // fitView
#include "../fold/FoldEngine.h"
#include "../geometry/Polygon.h"
#include "../math/Mat3.h"
#include "../shading/Palette.h"
#include <vector>

namespace ori {

struct RenderOptions {
    bool whiteSideUp = true;     // the flat sheet shows its white side (origami convention)
    bool depthShading = true;
    double depthPerLevel = kDepthPerLevel;   // darkening per layer below the top
    bool lighting = true;
    bool outlines = true;
    bool shadow = true;
    bool reverseOrder = false;   // paint top first - shows why z-ordering matters
    double alpha = 1.0;          // < 1: translucent X-ray preview
    int maxPanels = -1;          // paint only the first N panels (progressive painter's)
    Vec2 lightDir{-0.6, -0.8};   // light comes from the top left
};

// Bounding box of panels (folded coordinates).
BBox panelBounds(const std::vector<FoldPanel>& panels);

// The colour a panel is painted (before lighting). rankFromTop 0 = top layer.
Color panelColor(const FoldPanel& p, int rankFromTop, int layerCount, const Palette& pal, const RenderOptions& opt);

// Paint panels already ordered bottom-to-top. `view` maps folded coordinates to pixels.
void drawPanels(Image& img, const std::vector<FoldPanel>& bottomToTop, const Mat3& view,
                const Palette& pal, const RenderOptions& opt = {});
void drawFoldState(Image& img, const FoldState& s, const Mat3& view, const Palette& pal,
                   const RenderOptions& opt = {});

// Fit a folded state into a screen rectangle and draw it; returns the view used.
Mat3 drawFoldStateFitted(Image& img, const FoldState& s, const Rect& r, const Palette& pal,
                         const RenderOptions& opt = {}, int margin = 16);

// Isometric exploded stack with "L0".."Ln" labels.
void drawExplodedView(Image& img, const FoldState& s, const Rect& r, const Palette& pal,
                      double lift = 18.0, const RenderOptions& opt = {});

// One layer in colour over a pale ghost of the whole stack.
void drawLayerThumbnail(Image& img, const FoldState& s, int layer, const Rect& r, const Palette& pal,
                        const RenderOptions& opt = {});
// A grid of thumbnails (top layer first); returns how many were drawn.
int drawLayerInspector(Image& img, const FoldState& s, const Rect& r, const Palette& pal, int cols,
                       const RenderOptions& opt = {});

// The next fold as an origami diagram: the crease across the model (valley
// dashed, mountain dash-dot) and a curved arrow from the moving flap to where
// it lands. Uses `view` from drawFoldStateFitted.
void drawFoldDiagram(Image& img, const FoldState& s, const FoldStep& next, const Mat3& view, const Palette& pal);

// Area-weighted centre of the paper on the moving side of a fold.
Vec2 movingCentroid(const FoldState& s, const FoldStep& step);

} // namespace ori
