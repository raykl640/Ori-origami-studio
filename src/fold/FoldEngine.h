// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : fold            Owner : Mohamed Bile (Fold transformation engine)
//  File   : FoldEngine.h
//  Folds a flat sheet through a sequence of straight valley / mountain folds.
//
//  The folded model is a stack of PANELS. Each panel is a flat polygon of
//  paper plus the matrix that took it from the flat sheet to where it now
//  lies. One fold step:
//    1. SPLIT   every panel along the fold line (polygon clipping against
//               the two half-planes of the line - Sutherland-Hodgman with a
//               single clip edge).
//    2. REFLECT the pieces on the moving side with Mat3::reflectLine - the
//               reflection built by composing translate, rotate and scale.
//    3. RESTACK the moved pieces: a valley fold lands them on top of the
//               stack in reverse order, a mountain fold tucks them underneath.
//    4. CREASE  the line where a panel was split is mapped back to the flat
//               sheet with the inverse matrix and recorded as a mountain or
//               valley crease, so the engine also produces the crease pattern.
//
//  This module only depends on math/, so it can be built and tested on its
//  own. Panels use plain vertex lists; geometry::Polygon can be built from
//  FoldPanel::outline directly.
// ============================================================================
#pragma once
#include "../math/Mat3.h"
#include "../math/Vec2.h"
#include <string>
#include <vector>

namespace ori {

enum class FoldKind { Valley, Mountain };

inline FoldKind opposite(FoldKind k) { return k == FoldKind::Valley ? FoldKind::Mountain : FoldKind::Valley; }
inline const char* foldKindName(FoldKind k) { return k == FoldKind::Valley ? "valley" : "mountain"; }

// One fold, given in the coordinates of the CURRENT folded model.
// The paper on the moving side - to the right of a -> b on screen
// (cross(b - a, p - a) > 0 with y pointing down) - is folded over the line.
struct FoldStep {
    Vec2 a, b;
    FoldKind kind = FoldKind::Valley;
    std::string label;
};

struct FoldPanel {
    int id = 0;                     // stable id, inherited by both halves of a split
    std::vector<Vec2> outline;      // where the panel lies now (folded coordinates)
    Mat3 sheetToFolded;             // folded point = sheetToFolded * flat-sheet point
    int layer = 0;                  // stacking order: 0 = bottom, larger = nearer the viewer
    bool faceUp = true;             // true when the front (coloured) side faces the viewer

    // The same piece of paper on the unfolded sheet.
    std::vector<Vec2> sheetOutline() const;
};

// A crease on the FLAT sheet, as it reads on the front side of the paper.
struct FoldCrease {
    Vec2 a, b;
    FoldKind kind = FoldKind::Valley;
    int step = 0;                   // index of the fold step that made it
};

struct FoldState {
    std::vector<Vec2> sheet;        // the original flat paper outline
    std::vector<FoldPanel> panels;
    std::vector<FoldCrease> creases;
    int stepsApplied = 0;

    // Panels ordered for the painter's algorithm (draw first = bottom).
    std::vector<FoldPanel> bottomToTop() const;
    int layerCount() const;
};

class FoldEngine {
public:
    // The unfolded sheet as a single face-up panel.
    static FoldState flat(const std::vector<Vec2>& sheet);

    // Apply one fold and return the new state (the input is not modified).
    static FoldState fold(const FoldState& state, const FoldStep& step);

    // Every state from flat to fully folded: result[0] is flat,
    // result[i] is after steps[0..i-1].
    static std::vector<FoldState> run(const std::vector<Vec2>& sheet, const std::vector<FoldStep>& steps);

    // One frame of a fold in progress, t in [0, 1], for animation. The moving
    // pieces use Mat3::partialFold(t); they turn face-down after t = 0.5.
    // Returned bottom-to-top, ready to paint.
    static std::vector<FoldPanel> inProgress(const FoldState& state, const FoldStep& step, double t);

    // ---- helpers (public so they can be unit tested and reused) ------------
    // Signed distance of p from the line a -> b; positive on the moving side.
    static double side(const Vec2& a, const Vec2& b, const Vec2& p);
    // Keep the part of `poly` with side(a, b, p) * sign >= 0 (sign = +1 or -1).
    // Sutherland-Hodgman against one clip edge.
    static std::vector<Vec2> clipHalfPlane(const std::vector<Vec2>& poly, const Vec2& a, const Vec2& b, int sign);
    // Signed shoelace area (sign depends on vertex order).
    static double signedArea(const std::vector<Vec2>& poly);
    // Total paper area of a state (panels do not overlap on the sheet, so this
    // must always equal the area of the flat sheet).
    static double paperArea(const FoldState& state);
};

} // namespace ori
