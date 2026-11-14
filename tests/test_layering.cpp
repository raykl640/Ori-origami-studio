// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : model library, painter's algorithm, translucency, inspector
//  Owner : Edgar Kareithi (Layering, panels & studio UI)
// ============================================================================
#include "TestFramework.h"
#include "../src/layering/LayerRenderer.h"
#include "../src/models/Models.h"
#include <cmath>

using namespace ori;

TEST(models_fold_to_the_documented_stacks) {
    struct Expect { OrigamiModel m; int panels, layers, mountain, valley; };
    const Expect cases[] = {
        {dogFaceModel(), 8, 8, 3, 4},
        {kiteConeModel(), 6, 6, 1, 4},
        {napkinTriangleModel(), 8, 8, 3, 4},
    };
    for (const Expect& e : cases) {
        const auto states = e.m.run();
        CHECK(states.size() == e.m.steps.size() + 1);
        const FoldState& s = states.back();
        CHECK(int(s.panels.size()) == e.panels);
        CHECK(s.layerCount() == e.layers);
        // Folding never creates or loses paper.
        for (const FoldState& st : states) CHECK_NEAR(FoldEngine::paperArea(st), e.m.paper.area(), 1e-6);
        const CreasePattern cp = creasePatternOf(s, e.m.name);
        CHECK(cp.count(CreaseType::Mountain) == e.mountain);
        CHECK(cp.count(CreaseType::Valley) == e.valley);
        // Every crease lies on the flat sheet.
        for (const Crease& c : cp.creases) CHECK(cp.paper.contains(c.segment().midpoint()));
        OrigamiModel found;
        CHECK(findModel(e.m.key, found) && found.name == e.m.name);
    }
    OrigamiModel none;
    CHECK(!findModel("unicorn", none));
}

TEST(models_kite_folds_lie_on_the_22_5_degree_bisectors) {
    const OrigamiModel kite = kiteConeModel();
    for (int i = 0; i < 2; ++i) {
        const Vec2 d = (kite.steps[std::size_t(i)].b - kite.steps[std::size_t(i)].a).normalized();
        CHECK_NEAR(std::fabs(d.x), std::sin(degToRad(22.5)), 1e-9);   // 22.5 degrees off the vertical
        // The end point lands on the lower-left / lower-right edge (y = 200 + x or y = 600 - x).
        const Vec2 e = i == 0 ? kite.steps[0].b : kite.steps[1].a;
        CHECK_NEAR(e.y, i == 0 ? 200 + e.x : 600 - e.x, 1e-9);
    }
}

TEST(models_sequence_roundtrip) {
    for (const OrigamiModel& m : allModels()) {
        FoldSequence back;
        std::string err;
        CHECK(FoldSequence::fromText(m.toSequence().toText(), back, &err));
        const OrigamiModel r = OrigamiModel::fromSequence(back, m.key);
        CHECK(r.steps.size() == m.steps.size() && r.whiteSideUp == m.whiteSideUp && r.decoration == m.decoration);
        CHECK(r.run().back().panels.size() == m.run().back().panels.size());
    }
}

TEST(layering_painters_algorithm_top_layer_wins) {
    // Book fold of a white-side-up square: the flap (face down) lands on top and shows colour.
    const FoldState s = FoldEngine::fold(FoldEngine::flat({{0, 0}, {100, 0}, {100, 100}, {0, 100}}),
                                         {{50, 100}, {50, 0}, FoldKind::Valley, ""});
    const Palette pal = Palette::classic();
    RenderOptions o;
    o.lighting = false;
    o.depthShading = false;
    o.outlines = false;
    o.shadow = false;
    Image img(100, 100, colors::White);
    drawFoldState(img, s, Mat3{}, pal, o);
    CHECK(img.getPixel(25, 50) == pal.paperFront);      // flap on top: coloured side
    CHECK(img.getPixel(75, 50) == colors::White);       // nothing right of the fold
    o.reverseOrder = true;                               // paint top first: the bug
    Image wrong(100, 100, colors::White);
    drawFoldState(wrong, s, Mat3{}, pal, o);
    CHECK(wrong.getPixel(25, 50) == pal.paperBack);
    // Progressive painting: only the bottom panel.
    o.reverseOrder = false;
    o.maxPanels = 1;
    Image one(100, 100, colors::White);
    drawFoldState(one, s, Mat3{}, pal, o);
    CHECK(one.getPixel(25, 50) == pal.paperBack);
}

TEST(layering_depth_shading_and_translucency) {
    const Palette pal = Palette::classic();
    FoldPanel p;
    p.faceUp = false;
    RenderOptions o;
    const Color top = panelColor(p, 0, 4, pal, o), deeper = panelColor(p, 3, 4, pal, o);
    CHECK(top == pal.paperFront);
    CHECK(deeper.r + deeper.g + deeper.b < top.r + top.g + top.b);   // lower layers are darker
    o.depthShading = false;
    CHECK(panelColor(p, 3, 4, pal, o) == pal.paperFront);
    // Alpha: two layers at 0.5 over white.
    const FoldState s = FoldEngine::fold(FoldEngine::flat({{0, 0}, {100, 0}, {100, 100}, {0, 100}}),
                                         {{50, 100}, {50, 0}, FoldKind::Valley, ""});
    RenderOptions x;
    x.alpha = 0.5;
    x.outlines = false;
    x.depthShading = false;
    Image img(100, 100, colors::White);
    drawFoldState(img, s, Mat3{}, pal, x);
    const Color c = img.getPixel(25, 50);
    const Color expect = blendOver(blendOver(colors::White, pal.paperBack, 0.5), pal.paperFront, 0.5);
    CHECK(std::abs(c.r - expect.r) <= 1 && std::abs(c.g - expect.g) <= 1 && std::abs(c.b - expect.b) <= 1);
}

TEST(layering_inspector_and_diagram_helpers) {
    const OrigamiModel dog = dogFaceModel();
    const auto states = dog.run();
    Image img(600, 400, colors::White);
    CHECK(drawLayerInspector(img, states.back(), Rect{0, 0, 600, 400}, Palette::classic(), 4) == 8);
    // The moving flap of step 1 (top half of the diamond) has its centre above the crease.
    const Vec2 c = movingCentroid(states[0], dog.steps[0]);
    CHECK_NEAR(c.x, 200.0, 1e-9);
    CHECK_NEAR(c.y, 200.0 - 200.0 / 3.0, 1e-9);
    const BBox b = panelBounds(states.back().panels);
    CHECK(b.width() > 0 && b.height() > 0 && b.max.y <= 350 + 1e-9);   // snout folded up to y = 350
}
