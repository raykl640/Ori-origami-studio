// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : fold engine (split, reflect, restack, crease generation)
//  Owner : Mohamed Bile (Fold transformation engine)
// ============================================================================
#include "TestFramework.h"
#include "../src/fold/FoldEngine.h"
#include <cmath>

using namespace ori;

namespace {

const std::vector<Vec2> kSquare = {{0, 0}, {100, 0}, {100, 100}, {0, 100}};

// Fold lines for a 100 x 100 square. The moving side is to the right of a -> b
// on screen, so these move the right half / bottom half / bottom-right triangle.
const FoldStep kBookValley    {{50, 100}, {50, 0}, FoldKind::Valley, "book fold"};
const FoldStep kBookMountain  {{50, 100}, {50, 0}, FoldKind::Mountain, "book fold (mountain)"};
const FoldStep kHalfAgain     {{0, 50}, {50, 50}, FoldKind::Valley, "fold in half again"};
const FoldStep kDiagonal      {{0, 0}, {100, 100}, FoldKind::Valley, "diagonal"};

bool insideBox(const std::vector<Vec2>& pts, double x0, double y0, double x1, double y1) {
    for (const Vec2& p : pts)
        if (p.x < x0 - 1e-6 || p.x > x1 + 1e-6 || p.y < y0 - 1e-6 || p.y > y1 + 1e-6) return false;
    return true;
}

double absArea(const std::vector<Vec2>& poly) { return std::fabs(FoldEngine::signedArea(poly)); }

} // namespace

TEST(fold_half_plane_clip) {
    CHECK_NEAR(absArea(FoldEngine::clipHalfPlane(kSquare, {50, 100}, {50, 0}, +1)), 5000.0, 1e-9);
    CHECK_NEAR(absArea(FoldEngine::clipHalfPlane(kSquare, {50, 100}, {50, 0}, -1)), 5000.0, 1e-9);
    // Diagonal cut gives two triangles.
    const auto tri = FoldEngine::clipHalfPlane(kSquare, {0, 0}, {100, 100}, +1);
    CHECK_NEAR(absArea(tri), 5000.0, 1e-9);
    // Line outside the square: everything on one side, nothing on the other.
    CHECK_NEAR(absArea(FoldEngine::clipHalfPlane(kSquare, {200, 100}, {200, 0}, -1)), 10000.0, 1e-9);
    CHECK(FoldEngine::clipHalfPlane(kSquare, {200, 100}, {200, 0}, +1).empty());
    CHECK(FoldEngine::side({50, 100}, {50, 0}, {80, 50}) > 0);   // right half is the moving side
}

TEST(fold_valley_book_fold) {
    const FoldState s = FoldEngine::fold(FoldEngine::flat(kSquare), kBookValley);
    CHECK(s.panels.size() == 2);
    CHECK(s.layerCount() == 2);
    CHECK_NEAR(FoldEngine::paperArea(s), 10000.0, 1e-6);
    for (const FoldPanel& p : s.panels) CHECK(insideBox(p.outline, 0, 0, 50, 100));
    const auto stack = s.bottomToTop();
    CHECK(stack[0].faceUp && !stack[1].faceUp);                 // the flap lands on top, face-down
    CHECK(stack[1].sheetToFolded.flipsOrientation());
    CHECK(s.creases.size() == 1);
    CHECK(s.creases[0].kind == FoldKind::Valley);
    CHECK_NEAR(s.creases[0].a.x, 50.0, 1e-9);
    CHECK_NEAR(s.creases[0].b.x, 50.0, 1e-9);
    CHECK_NEAR(distance(s.creases[0].a, s.creases[0].b), 100.0, 1e-9);
}

TEST(fold_mountain_goes_underneath) {
    const FoldState s = FoldEngine::fold(FoldEngine::flat(kSquare), kBookMountain);
    const auto stack = s.bottomToTop();
    CHECK(!stack[0].faceUp && stack[1].faceUp);                 // the flap is tucked behind
    CHECK(s.creases.size() == 1 && s.creases[0].kind == FoldKind::Mountain);
}

TEST(fold_twice_assigns_mountain_and_valley_per_layer) {
    const auto states = FoldEngine::run(kSquare, {kBookValley, kHalfAgain});
    CHECK(states.size() == 3);
    const FoldState& s = states.back();
    CHECK(s.panels.size() == 4);
    CHECK(s.layerCount() == 4);
    CHECK_NEAR(FoldEngine::paperArea(s), 10000.0, 1e-6);
    for (const FoldPanel& p : s.panels) CHECK(insideBox(p.outline, 0, 0, 50, 50));
    // 1 crease from step 1, plus the second fold crossing both layers.
    CHECK(s.creases.size() == 3);
    int valleys = 0, mountains = 0;
    for (const FoldCrease& c : s.creases) {
        if (c.step != 1) continue;
        CHECK_NEAR(c.a.y, 50.0, 1e-9);
        CHECK_NEAR(c.b.y, 50.0, 1e-9);
        const double midX = 0.5 * (c.a.x + c.b.x);
        // Left half of the sheet was face-up -> valley; right half was flipped -> mountain.
        if (c.kind == FoldKind::Valley) { ++valleys; CHECK(midX < 50); }
        else                            { ++mountains; CHECK(midX > 50); }
    }
    CHECK(valleys == 1 && mountains == 1);
}

TEST(fold_panels_tile_the_original_sheet) {
    const FoldStep third{{100, 0}, {0, 100}, FoldKind::Mountain, "corner"};
    const FoldState s = FoldEngine::run(kSquare, {kDiagonal, kBookValley, third}).back();
    double sheetArea = 0;
    for (const FoldPanel& p : s.panels) {
        const auto flat = p.sheetOutline();
        CHECK(insideBox(flat, 0, 0, 100, 100));                 // every piece came from the sheet
        sheetArea += absArea(flat);
        for (std::size_t i = 0; i < flat.size(); ++i)            // matrix and outline agree
            CHECK(nearlyEqual(p.sheetToFolded.apply(flat[i]), p.outline[i], 1e-7));
        CHECK(p.faceUp == !p.sheetToFolded.flipsOrientation()); // face-down <=> odd number of reflections
    }
    CHECK_NEAR(sheetArea, 10000.0, 1e-6);                        // no paper lost or duplicated
    CHECK_NEAR(FoldEngine::paperArea(s), 10000.0, 1e-6);
}

TEST(fold_line_missing_the_paper_changes_nothing) {
    const FoldStep miss{{200, 100}, {200, 0}, FoldKind::Valley, "outside"};
    const FoldState s = FoldEngine::fold(FoldEngine::flat(kSquare), miss);
    CHECK(s.panels.size() == 1 && s.creases.empty() && s.panels[0].faceUp);
}

TEST(fold_in_progress_matches_endpoints) {
    const FoldState flat = FoldEngine::flat(kSquare);
    const FoldState done = FoldEngine::fold(flat, kBookValley);
    const auto start = FoldEngine::inProgress(flat, kBookValley, 0.0);
    const auto end = FoldEngine::inProgress(flat, kBookValley, 1.0);
    const auto stack = done.bottomToTop();
    CHECK(start.size() == 2 && end.size() == 2);
    double a0 = 0;
    for (const FoldPanel& p : start) a0 += absArea(p.outline);
    CHECK_NEAR(a0, 10000.0, 1e-6);                               // t = 0: still flat
    for (std::size_t i = 0; i < end.size(); ++i) {
        CHECK(end[i].faceUp == stack[i].faceUp);
        CHECK(end[i].outline.size() == stack[i].outline.size());
        for (std::size_t k = 0; k < end[i].outline.size(); ++k)
            CHECK(nearlyEqual(end[i].outline[k], stack[i].outline[k], 1e-9));
    }
    // Half way the flap stands upright: it projects to a line (zero area).
    const auto mid = FoldEngine::inProgress(flat, kBookValley, 0.5);
    CHECK_NEAR(absArea(mid.back().outline), 0.0, 1e-6);
}
