// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : models          Owner : Edgar Kareithi (Layering, panels & studio UI)
//  File   : Models.cpp
// ============================================================================
#include "Models.h"
#include "../curves/Curves.h"
#include "../editor/CreaseRenderer.h"
#include "../raster/Circle.h"
#include "../raster/Fill.h"
#include "../raster/Lines.h"
#include <cmath>

namespace ori {

// ----------------------------------------------------------------- models --
OrigamiModel dogFaceModel() {
    OrigamiModel m;
    m.key = "dog";
    m.name = "Dog face";
    m.paper = Polygon({{200, 0}, {400, 200}, {200, 400}, {0, 200}});   // a square turned into a diamond
    m.whiteSideUp = true;
    m.decoration = "dog";
    m.steps = {
        {{400, 200}, {0, 200}, FoldKind::Valley, "Fold top corner down"},
        {{172, 200}, {52, 252}, FoldKind::Valley, "Fold left ear down"},
        {{348, 252}, {228, 200}, FoldKind::Valley, "Fold right ear down"},
        {{0, 350}, {400, 350}, FoldKind::Valley, "Fold snout up"},
    };
    return m;
}

OrigamiModel kiteConeModel() {
    // The kite folds lie on the bisectors of the 45 degree angle at the top
    // corner, i.e. 22.5 degrees either side of the vertical diagonal. They
    // meet the lower edges at (200 -/+ 400 tan 22.5 / ..., ...) - computed.
    const double s = std::sin(degToRad(22.5)), c = std::cos(degToRad(22.5));
    const double t = 400.0 / (s + c);                          // where the bisector meets y = 200 + x
    const Vec2 left{200 - s * t, c * t}, right{200 + s * t, c * t};
    OrigamiModel m;
    m.key = "kite";
    m.name = "Kite cone";
    m.paper = Polygon({{200, 0}, {400, 200}, {200, 400}, {0, 200}});
    m.whiteSideUp = false;
    m.steps = {
        {{200, 0}, left, FoldKind::Valley, "Fold left edge to the centre"},
        {right, {200, 0}, FoldKind::Valley, "Fold right edge to the centre"},
        {{400, 110}, {0, 110}, FoldKind::Mountain, "Fold the tip behind"},
    };
    return m;
}

OrigamiModel napkinTriangleModel() {
    OrigamiModel m;
    m.key = "napkin";
    m.name = "Napkin triangle";
    m.paper = Polygon::rectangle(0, 0, 400, 400);
    m.whiteSideUp = true;
    m.steps = {
        {{200, 400}, {200, 0}, FoldKind::Valley, "Book fold right to left"},
        {{0, 200}, {200, 200}, FoldKind::Mountain, "Fold bottom half behind"},
        {{200, 200}, {0, 0}, FoldKind::Valley, "Fold corner down to a triangle"},
    };
    return m;
}

std::vector<OrigamiModel> allModels() { return {dogFaceModel(), kiteConeModel(), napkinTriangleModel()}; }

bool findModel(const std::string& key, OrigamiModel& out) {
    for (const OrigamiModel& m : allModels())
        if (m.key == key) { out = m; return true; }
    return false;
}

FoldSequence OrigamiModel::toSequence() const {
    FoldSequence s;
    s.name = name;
    s.paper = paper.v;
    s.steps = steps;
    s.whiteSideUp = whiteSideUp;
    s.decoration = decoration;
    return s;
}

OrigamiModel OrigamiModel::fromSequence(const FoldSequence& seq, const std::string& key) {
    OrigamiModel m;
    m.key = key;
    m.name = seq.name.empty() ? key : seq.name;
    m.paper = Polygon(seq.paper);
    m.steps = seq.steps;
    m.whiteSideUp = seq.whiteSideUp;
    m.decoration = seq.decoration;
    return m;
}

CreasePattern creasePatternOf(const FoldState& folded, const std::string& name) {
    CreasePattern cp(Polygon(folded.sheet), name);
    for (const FoldCrease& c : folded.creases)
        cp.creases.push_back({c.a, c.b, c.kind == FoldKind::Valley ? CreaseType::Valley : CreaseType::Mountain});
    return cp;
}

// ------------------------------------------------------------ decorations --
void drawDogFeatures(Image& img, const Mat3& view) {
    const double k = std::sqrt(std::fabs(view.determinant()));   // model -> pixel scale
    const Color ink = Color::hex(0x1E1A1A);
    auto P = [&](double x, double y) { return view.apply(Vec2{x, y}); };
    auto R = [&](double r) { return std::max(1, int(std::lround(r * k))); };

    // Eyes: filled midpoint circles with a highlight.
    for (double ex : {164.0, 236.0}) {
        const Vec2 e = P(ex, 252);
        fillCircle(img, int(e.x), int(e.y), R(10), ink);
        const Vec2 h = P(ex + 3.5, 248.5);
        fillCircle(img, int(h.x), int(h.y), R(3), colors::White);
    }
    // Brows: short cubic Bezier arcs.
    for (double ex : {164.0, 236.0}) {
        const double dir = ex < 200 ? -1.0 : 1.0;
        const std::vector<Vec2> brow = {P(ex - 12 * dir, 232), P(ex - 4 * dir, 223), P(ex + 6 * dir, 223), P(ex + 13 * dir, 230)};
        drawCurve(img, bezierAdaptive(brow, 0.3), Stroke::solid(ink, std::max(2, R(2.5))));
    }
    // Nose: midpoint ellipse with a highlight.
    const Vec2 n = P(200, 308);
    fillEllipse(img, int(n.x), int(n.y), R(13), R(9), ink);
    const Vec2 nh = P(195, 304);
    fillEllipse(img, int(nh.x), int(nh.y), R(4), R(2.5), Color(110, 100, 100));
    // Mouth: two Bezier curves from under the nose.
    const int w = std::max(2, R(2.2));
    drawCurve(img, bezierAdaptive({P(200, 316), P(200, 330), P(190, 336), P(180, 328)}, 0.3), Stroke::solid(ink, w));
    drawCurve(img, bezierAdaptive({P(200, 316), P(200, 330), P(210, 336), P(220, 328)}, 0.3), Stroke::solid(ink, w));
}

void drawDecoration(Image& img, const std::string& decoration, const Mat3& view) {
    if (decoration == "dog") drawDogFeatures(img, view);
}

// ------------------------------------------------------------ crane mascot --
void drawCraneMascot(Image& img, const Rect& r, const Palette& pal) {
    // Concave outline of a crane facing left (local units, y down).
    const Polygon crane({{18, 62}, {40, 50}, {54, 60}, {118, 140}, {172, 18}, {200, 126},
                         {298, 60}, {258, 150}, {205, 182}, {140, 178}, {44, 70}});
    const BBox b{{0, 0}, {310, 215}};
    const Mat3 view = fitView(b, r, 10);
    const double k = std::sqrt(std::fabs(view.determinant()));

    // Ground shadow: translucent midpoint ellipse.
    const Vec2 sh = view.apply(Vec2{180, 200});
    fillEllipseAlpha(img, int(sh.x), int(sh.y), int(95 * k), int(9 * k), Color(70, 60, 50), 0.22);

    // Facets: ear-clipping triangulation, each facet lit by its own direction.
    const auto tris = crane.triangulate();
    const Vec2 light = Vec2{-0.5, -1.0}.normalized();
    int i = 0;
    for (const auto& t : tris) {
        const Vec2 c = (t[0] + t[1] + t[2]) / 3.0;
        const Vec2 longest = distance(t[0], t[1]) > distance(t[1], t[2]) ? t[1] - t[0] : t[2] - t[1];
        const double facing = std::fabs(dot(longest.normalized().perp(), light));
        const double f = 0.78 + 0.32 * facing + 0.05 * ((i++ % 2) ? 1 : -1) - 0.0006 * (c.y - 100);
        std::vector<Vec2> s;
        for (const Vec2& p : t) s.push_back(view.apply(p));
        fillPolygon(img, s, scale(pal.paperFront, f));
    }
    // Facet edges as faint folds, then the outline.
    for (const auto& t : tris)
        for (int e = 0; e < 3; ++e)
            drawStroke(img, view.apply(t[std::size_t(e)]), view.apply(t[std::size_t((e + 1) % 3)]),
                       Stroke::smooth(Color(255, 240, 230, 150)));
    drawPolyline(img, crane.transformed(view).v, Stroke::smooth(pal.outline), true);

    // Eye.
    const Vec2 eye = view.apply(Vec2{38, 58});
    fillCircle(img, int(eye.x), int(eye.y), std::max(2, int(3 * k)), Color::hex(0x1E1A1A));

    // Wind lines: Catmull-Rom splines through wavy points.
    for (int w = 0; w < 3; ++w) {
        std::vector<Vec2> pts;
        for (int j = 0; j <= 4; ++j)
            pts.push_back(view.apply(Vec2{222.0 + j * 20, 8.0 + w * 13 + ((j % 2) ? -4.0 : 4.0)}));
        drawCurve(img, catmullRom(pts, 12), Stroke::solid(Color(140, 170, 205), 2));
    }
}

} // namespace ori
