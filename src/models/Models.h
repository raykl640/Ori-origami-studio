// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : models          Owner : Edgar Kareithi (Layering, panels & studio UI)
//  File   : Models.h
//  The origami model library. Each model is a paper outline plus a fold
//  sequence for the fold engine:
//    * dog face        - diamond, 4 valley folds (top down, two ears, snout)
//    * kite cone       - kite base on the 22.5 degree bisectors + mountain fold
//    * napkin triangle - valley, mountain, diagonal valley: 8 layers deep
//  plus the faceted paper-crane mascot (ear-clipping triangulation).
// ============================================================================
#pragma once
#include "../core/Image.h"
#include "../core/Layout.h"
#include "../editor/CreasePattern.h"
#include "../fold/FoldEngine.h"
#include "../fold/FoldSequence.h"
#include "../geometry/Polygon.h"
#include "../shading/Palette.h"
#include <string>
#include <vector>

namespace ori {

struct OrigamiModel {
    std::string key;               // short id used on the command line, e.g. "dog"
    std::string name;              // display name
    Polygon paper;
    std::vector<FoldStep> steps;
    bool whiteSideUp = true;
    std::string decoration;        // drawn on the finished model ("dog" or empty)

    std::vector<FoldState> run() const { return FoldEngine::run(paper.v, steps); }
    FoldSequence toSequence() const;
    static OrigamiModel fromSequence(const FoldSequence& seq, const std::string& key = "custom");
};

OrigamiModel dogFaceModel();
OrigamiModel kiteConeModel();
OrigamiModel napkinTriangleModel();
std::vector<OrigamiModel> allModels();
// Looks a model up by key ("dog", "kite", "napkin"); returns false if unknown.
bool findModel(const std::string& key, OrigamiModel& out);

// The crease pattern a fold sequence leaves on the flat sheet.
CreasePattern creasePatternOf(const FoldState& folded, const std::string& name = "");

// Eyes (midpoint circles), nose (midpoint ellipse), mouth and brows (Bezier)
// for the folded dog face. `view` maps model coordinates to pixels.
void drawDogFeatures(Image& img, const Mat3& view);
// Draws a named decoration ("dog"); unknown names are ignored.
void drawDecoration(Image& img, const std::string& decoration, const Mat3& view);

// Faceted paper crane: concave outline -> ear-clipping -> shaded facets,
// circle eye, ellipse shadow, Catmull-Rom wind lines. Fitted into `r`.
void drawCraneMascot(Image& img, const Rect& r, const Palette& pal);

} // namespace ori
