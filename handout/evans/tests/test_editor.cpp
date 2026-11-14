// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : crease pattern model, snapping, .cp / .svg, face filling
//  Owner : Evans Mwendwa (Crease pattern editor)
// ============================================================================
#include "TestFramework.h"
#include "../src/editor/CreasePattern.h"
#include "../src/editor/CreaseRenderer.h"
#include <cmath>

using namespace ori;

namespace {

CreasePattern preliminary() {
    CreasePattern cp(Polygon::rectangle(0, 0, 400, 400), "preliminary base");
    cp.addCrease({0, 0}, {400, 400}, CreaseType::Valley);
    cp.addCrease({400, 0}, {0, 400}, CreaseType::Valley);
    cp.addCrease({0, 200}, {400, 200}, CreaseType::Mountain);
    cp.addCrease({200, 0}, {200, 400}, CreaseType::Mountain);
    return cp;
}

} // namespace

TEST(editor_creases_are_trimmed_to_the_paper) {
    CreasePattern cp(Polygon({{200, 0}, {400, 200}, {200, 400}, {0, 200}}));   // diamond
    CHECK(cp.addCrease({-100, 200}, {500, 200}, CreaseType::Valley));
    CHECK(nearlyEqual(cp.creases[0].a, {0, 200}, 1e-9) && nearlyEqual(cp.creases[0].b, {400, 200}, 1e-9));
    CHECK(!cp.addCrease({0, 0}, {50, 0}, CreaseType::Valley));                  // outside: rejected
    CHECK(cp.addFullCrease({200, 10}, {200, 20}, CreaseType::Mountain));
    CHECK_NEAR(cp.creases[1].segment().length(), 400.0, 1e-9);
    CHECK(cp.count(CreaseType::Valley) == 1 && cp.count(CreaseType::Mountain) == 1);
    cp.toggle(0);
    CHECK(cp.count(CreaseType::Mountain) == 2);
    CHECK(cp.creaseAt({100, 203}, 5) == 0);
    CHECK(cp.creaseAt({100, 230}, 5) == -1);
    CHECK(cp.removeCrease(0) && cp.creases.size() == 1 && !cp.removeCrease(9));
}

TEST(editor_vertices_intersections_and_split) {
    const CreasePattern cp = preliminary();
    CHECK(cp.intersections().size() == 1);
    CHECK(nearlyEqual(cp.intersections()[0], {200, 200}));
    CHECK(cp.vertices().size() == 4 + 4 + 1);   // corners, edge midpoints, centre
    const CreasePattern s = cp.split();
    CHECK(s.creases.size() == 8);               // every crease cut at the centre
    for (const Crease& c : s.creases) CHECK_NEAR(c.segment().length(), c.a.x == c.b.x || c.a.y == c.b.y ? 200.0 : 200 * std::sqrt(2.0), 1e-9);
}

TEST(editor_snapping_vertex_then_grid) {
    const CreasePattern cp = preliminary();
    CHECK(nearlyEqual(cp.snap({205, 196}, 10, 50), {200, 200}));   // vertex wins
    CHECK(nearlyEqual(cp.snap({130, 70}, 10, 50), {150, 50}));     // otherwise grid
    CHECK(nearlyEqual(cp.snap({130, 70}, 10, 0), {130, 70}));      // no grid: unchanged
    CHECK(nearlyEqual(cp.snap({390, 3}, 15, 50), {400, 0}));       // paper corner
}

TEST(editor_cp_text_roundtrip_and_oripa_import) {
    CreasePattern cp = preliminary();
    cp.gridStep = 25;
    CreasePattern back;
    std::string err;
    CHECK(CreasePattern::fromText(cp.toText(), back, &err));
    CHECK(back.name == cp.name && back.gridStep == 25);
    CHECK(back.paper.size() == 4 && back.creases.size() == cp.creases.size());
    for (std::size_t i = 0; i < cp.creases.size(); ++i) {
        CHECK(back.creases[i].type == cp.creases[i].type);
        CHECK(nearlyEqual(back.creases[i].a, cp.creases[i].a) && nearlyEqual(back.creases[i].b, cp.creases[i].b));
    }
    // ORIPA numeric lines: 1 = border, 2 = mountain, 3 = valley.
    const std::string oripa = "1 0 0 100 0\n1 100 0 100 100\n1 100 100 0 100\n1 0 100 0 0\n3 0 0 100 100\n2 0 100 100 0\n";
    CreasePattern o;
    CHECK(CreasePattern::fromText(oripa, o, &err));
    CHECK(o.paper.size() == 4 && std::fabs(o.paper.area() - 10000) < 1e-9);
    CHECK(o.count(CreaseType::Valley) == 1 && o.count(CreaseType::Mountain) == 1);
    // Errors carry a line number.
    CHECK(!CreasePattern::fromText("paper 0 0 1 0 1 1\nX 1 2 3 4\n", o, &err));
    CHECK(err.find("line 2") != std::string::npos);
    CHECK(!CreasePattern::fromText("M 0 0 1 1\n", o, &err));   // no paper
    CHECK(!CreasePattern::load("does/not/exist.cp", o, &err));
}

TEST(editor_svg_export) {
    const std::string svg = preliminary().toSVG(2.0, 10);
    CHECK(svg.find("<svg") != std::string::npos && svg.find("</svg>") != std::string::npos);
    std::size_t lines = 0, pos = 0;
    while ((pos = svg.find("<line", pos)) != std::string::npos) { ++lines; ++pos; }
    CHECK(lines == 4);
    CHECK(svg.find("width=\"820\"") != std::string::npos);   // 400 * 2 + 2 * 10
    CHECK(svg.find("class=\"mountain\"") != std::string::npos && svg.find("class=\"valley\"") != std::string::npos);
}

TEST(editor_fit_view_and_face_two_colouring) {
    const Mat3 v = fitView(BBox{{0, 0}, {400, 200}}, Rect{10, 10, 220, 220}, 10);
    CHECK(nearlyEqual(v.apply({0, 0}), {20, 70}, 1e-9));
    CHECK(nearlyEqual(v.apply({400, 200}), {220, 170}, 1e-9));
    const CreasePattern cp = preliminary();
    Image img(220, 220, colors::White);
    const Color a = colors::Red, b = colors::Blue;
    CHECK(fillCreaseFaces(img, cp, fitView(cp.paper.bounds(), Rect{0, 0, 220, 220}, 10), a, b) == 8);
    long ra = 0, rb = 0;
    for (const Color& p : img.pixels()) { ra += p.sameRGB(a); rb += p.sameRGB(b); }
    CHECK(ra > 15000 && rb > 15000 && std::labs(ra - rb) < 800);   // half and half
    // Neighbouring faces across the vertical crease differ.
    CHECK(!img.getPixel(105, 60).sameRGB(img.getPixel(115, 60)));
}

TEST(editor_face_colouring_survives_half_pixel_views_and_split_creases) {
    // The square base as folded: one book-fold half reversed, so the vertical
    // line is two creases. A view with a .5 px offset used to leave a 1 px gap
    // where a crease met the paper edge, and two faces merged.
    CreasePattern cp(Polygon::rectangle(0, 0, 400, 400));
    cp.addCrease({0, 0}, {400, 400}, CreaseType::Mountain);
    cp.addCrease({400, 0}, {0, 400}, CreaseType::Mountain);
    cp.addCrease({0, 200}, {400, 200}, CreaseType::Valley);
    cp.addCrease({200, 0}, {200, 200}, CreaseType::Valley);
    cp.addCrease({200, 200}, {200, 400}, CreaseType::Mountain);
    for (double off : {0.0, 0.25, 0.5, 0.75}) {
        const Mat3 view = Mat3::translate(315.5 + off, 108 + off) * Mat3::scale(1.522, 1.522);
        Image img(1000, 760, colors::White);
        CHECK(fillCreaseFaces(img, cp, view, colors::Red, colors::Blue) == 8);
        // walking round the centre, neighbouring faces alternate
        const Vec2 ring[8] = {{100, 30}, {300, 30}, {370, 100}, {370, 300}, {300, 370}, {100, 370}, {30, 300}, {30, 100}};
        for (int i = 0; i < 8; ++i) {
            const Vec2 p = view.apply(ring[i]), q = view.apply(ring[(i + 1) % 8]);
            CHECK(!img.getPixel(int(p.x), int(p.y)).sameRGB(img.getPixel(int(q.x), int(q.y))));
        }
    }
}
