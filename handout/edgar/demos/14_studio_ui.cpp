// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demo 14 : Studio interface                  Owner : Edgar Kareithi
//  The 5 interface regions from the initial design - crease editor, fold
//  sequence panel, folded preview, layer inspector and depth palette - with
//  every pixel drawn by the project's own rasterizer, from live model data.
// ============================================================================
#include "Demos.h"
#include "../src/curves/Curves.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/layering/LayerRenderer.h"
#include "../src/raster/Circle.h"
#include "../src/raster/Fill.h"
#include "../src/raster/Lines.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace ori {

namespace {

Rect numberedPanel(Image& img, const Rect& r, int n, const std::string& title) {
    const Rect in = drawPanel(img, r, "  " + title);
    fillCircle(img, r.x + 16, r.y + 11, 6, theme::Accent);
    drawText(img, r.x + 14, r.y + 8, std::to_string(n), colors::White);
    return in;
}

std::string upper(std::string s) {
    for (char& c : s) c = char(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

// Toolbar icon button.
void iconButton(Image& img, int x, int y, bool active, int kind, const Palette& pal) {
    const int s = 24;
    img.fillRect(x, y, s, s, active ? theme::Accent : Color(62, 60, 72));
    const Color c = colors::White;
    const double cx = x + s / 2.0, cy = y + s / 2.0;
    switch (kind) {
        case 0: drawStroke(img, {x + 6.0, y + 18.0}, {x + 18.0, y + 6.0}, Stroke::solid(c, 2)); break;   // crease tool
        case 1:                                                                                           // M/V toggle
            drawStroke(img, {x + 5.0, cy - 4}, {x + 19.0, cy - 4}, Stroke::dashDot(pal.mountain, 2));
            drawStroke(img, {x + 5.0, cy + 4}, {x + 19.0, cy + 4}, Stroke::dashed(Color(140, 180, 240), 2));
            break;
        case 2:                                                                                           // grid snap
            for (int j = 0; j < 3; ++j)
                for (int i = 0; i < 3; ++i) img.fillRect(x + 6 + i * 5, y + 6 + j * 5, 2, 2, c);
            break;
        case 3: {                                                                                         // fold
            drawArc(img, int(cx), int(cy) + 3, 7, -kPi, 0, c, 2);
            drawArrowHead(img, {cx + 7, cy + 6}, {0, 1}, 6, c, ArrowHead::Filled);
            break;
        }
        case 4:                                                                                           // flip
            img.strokeRect(x + 6, y + 6, 12, 12, c);
            fillPolygon(img, std::vector<Vec2>{{x + 6.0, y + 6.0}, {x + 18.0, y + 6.0}, {x + 18.0, y + 18.0}}, c);
            break;
        default:                                                                                          // export
            drawStroke(img, {cx, y + 5.0}, {cx, y + 15.0}, Stroke::solid(c, 2));
            drawArrowHead(img, {cx, y + 17.0}, {0, 1}, 6, c, ArrowHead::Filled);
            img.fillRect(x + 6, y + 19, 12, 2, c);
            break;
    }
}

void eyeIcon(Image& img, int cx, int cy, Color c) {
    drawEllipse(img, cx, cy, 8, 5, c);
    fillCircle(img, cx, cy, 2, c);
}

} // namespace

std::vector<std::string> demoStudioUI(const DemoContext& ctx) {
    Image img(kSheetW, kSheetH, theme::Background);
    const Palette pal = Palette::classic();
    const OrigamiModel dog = dogFaceModel();
    const auto states = dog.run();
    const FoldState& done = states.back();
    RenderOptions opt;
    opt.whiteSideUp = dog.whiteSideUp;
    const Color ink = Color::hex(0x2B2B33);

    // ---- title bar + toolbar ------------------------------------------------------------
    img.fillRect(0, 0, img.width(), 44, theme::HeaderBg);
    drawText(img, 14, 12, "ORI", theme::Accent, 3);
    drawText(img, 72, 10, "STUDIO", colors::White, 2);
    drawText(img, 72, 28, "DOG_FACE.ORI - " + std::to_string(dog.steps.size()) + " FOLDS", Color(170, 166, 160));
    for (int i = 0; i < 6; ++i) iconButton(img, 420 + i * 36, 10, i == 0, i, pal);
    drawText(img, 860, 18, "PURE C++ RASTERIZER - NO GRAPHICS LIBRARY", Color(170, 166, 160));

    // ---- 1: crease editor ------------------------------------------------------------------
    {
        const Rect in = numberedPanel(img, Rect{12, 56, 280, 344}, 1, "CREASE EDITOR");
        const CreasePattern cp = creasePatternOf(done, dog.name);
        const Mat3 view = fitView(cp.paper.bounds(), Rect{in.x, in.y, in.w, in.h - 26}, 14);
        CreaseViewOptions o;
        o.gridStep = 50;
        drawCreasePattern(img, cp, view, pal, o);
        // the crease being drawn: snapped start vertex, rubber-band line, cursor
        const Vec2 a = view.apply(Vec2{200, 0}), cur = view.apply(Vec2{290, 125});
        drawStroke(img, a, cur, Stroke::dashed(Color::hex(0x8E5BB5), 2));
        drawRing(img, int(a.x), int(a.y), 6, 2, Color::hex(0x8E5BB5));
        drawStroke(img, cur - Vec2{6, 0}, cur + Vec2{6, 0}, Stroke::solid(ink));
        drawStroke(img, cur - Vec2{0, 6}, cur + Vec2{0, 6}, Stroke::solid(ink));
        drawCreaseLegend(img, in.x + 8, in.bottom() - 14, pal);
    }

    // ---- 2: fold sequence -------------------------------------------------------------------
    {
        const Rect in = numberedPanel(img, Rect{12, 412, 280, 376}, 2, "FOLD SEQUENCE");
        const BBox world = panelBounds(states.front().panels);
        for (std::size_t i = 0; i < dog.steps.size(); ++i) {
            const int y = in.y + 8 + int(i) * 58;
            const bool current = i + 1 == dog.steps.size();
            if (current) img.fillRect(in.x + 2, y - 3, in.w - 4, 54, Color(252, 232, 222));
            Image thumb(48, 48, current ? Color(252, 232, 222) : theme::Panel);
            RenderOptions t = opt;
            t.shadow = false;
            t.outlines = false;
            drawFoldState(thumb, states[i + 1], fitView(world, Rect{0, 0, 48, 48}, 3), pal, t);
            img.blit(thumb, in.x + 8, y);
            img.fillRect(in.x + 66, y + 14, 16, 16, pal.valley);
            drawText(img, in.x + 71, y + 19, std::to_string(i + 1), colors::White);
            drawText(img, in.x + 90, y + 14, upper(dog.steps[i].label), ink);
            drawText(img, in.x + 90, y + 25, upper(foldKindName(dog.steps[i].kind)), theme::Caption);
        }
        // timeline: one knob per fold
        const int ty = in.bottom() - 26, tx0 = in.x + 14, tx1 = in.right() - 14;
        drawStroke(img, {double(tx0), double(ty)}, {double(tx1), double(ty)}, Stroke::solid(theme::Accent, 2));
        for (std::size_t i = 0; i <= dog.steps.size(); ++i) {
            const int x = tx0 + int((tx1 - tx0) * i / dog.steps.size());
            fillCircle(img, x, ty, 4, theme::Accent);
        }
        drawRing(img, tx1, ty, 7, 2, theme::Accent);
    }

    // ---- 3: folded preview --------------------------------------------------------------------
    {
        const Rect in = numberedPanel(img, Rect{304, 56, 580, 544}, 3, "FOLDED PREVIEW");
        const Mat3 view = drawFoldStateFitted(img, done, Rect{in.x + 40, in.y + 30, in.w - 80, in.h - 90}, pal, opt, 20);
        drawDecoration(img, dog.decoration, view);
        char buf[160];
        std::snprintf(buf, sizeof buf, "%d PANELS  -  %d LAYERS  -  PAINTER'S ORDER  -  DEPTH + LIGHT SHADING",
                      int(done.panels.size()), done.layerCount());
        drawText(img, in.x + 12, in.bottom() - 18, buf, theme::Caption);
    }

    // ---- 4: layer inspector -------------------------------------------------------------------------
    {
        const Rect in = numberedPanel(img, Rect{896, 56, 292, 732}, 4, "LAYER INSPECTOR");
        const int layers = done.layerCount();
        const int rowH = std::min(86, (in.h - 8) / std::max(1, layers));
        for (int i = 0; i < layers; ++i) {
            const int layer = layers - 1 - i;
            const int y = in.y + 4 + i * rowH;
            const Rect row{in.x + 6, y, in.w - 12, rowH - 6};
            img.fillRect(row.x, row.y, row.w, row.h, i == 0 ? Color(252, 232, 222) : Color(250, 248, 244));
            img.strokeRect(row.x, row.y, row.w, row.h, theme::Border);
            drawLayerThumbnail(img, done, layer, Rect{row.x + 4, row.y + 4, 80, row.h - 8}, pal, opt);
            int panels = 0;
            for (const FoldPanel& p : done.panels) panels += p.layer == layer;
            drawText(img, row.x + 96, row.y + row.h / 2 - 10, "LAYER " + std::to_string(layer), ink);
            drawText(img, row.x + 96, row.y + row.h / 2 + 2, std::to_string(panels) + (panels == 1 ? " PANEL" : " PANELS"), theme::Caption);
            eyeIcon(img, row.right() - 20, row.y + row.h / 2, ink);
        }
    }

    // ---- 5: depth palette ---------------------------------------------------------------------------
    {
        const Rect in = numberedPanel(img, Rect{304, 612, 580, 176}, 5, "DEPTH PALETTE");
        drawDepthPalette(img, in.x + 12, in.y + 14, 220, 64, pal, done.layerCount());
        // depth-step slider
        drawText(img, in.x + 260, in.y + 12, "DEPTH STEP PER LAYER", ink);
        const int sx0 = in.x + 260, sx1 = in.x + 400, sy = in.y + 32;
        img.fillRect(sx0, sy - 2, sx1 - sx0, 4, Color(225, 220, 210));
        const int knob = sx0 + int((sx1 - sx0) * kDepthPerLevel / 0.25);
        img.fillRect(sx0, sy - 2, knob - sx0, 4, theme::Accent);
        fillCircle(img, knob, sy, 7, colors::White);
        drawRing(img, knob, sy, 7, 2, ink);
        char buf[16];
        std::snprintf(buf, sizeof buf, "%.2F", kDepthPerLevel);
        drawText(img, sx1 + 8, sy - 3, buf, ink);
        // light direction dial
        drawText(img, in.x + 260, in.y + 56, "LIGHT DIRECTION", ink);
        const int dx = in.x + 300, dy = in.y + 100;
        fillCircle(img, dx, dy, 22, Color(250, 248, 244));
        drawCircle(img, dx, dy, 22, ink);
        const Vec2 l = opt.lightDir.normalized();
        drawStroke(img, {double(dx), double(dy)}, {dx + l.x * 20, dy + l.y * 20}, Stroke::solid(theme::Accent, 2));
        fillCircle(img, int(dx + l.x * 20), int(dy + l.y * 20), 3, theme::Accent);
        // palettes
        const Palette pals[3] = {Palette::classic(), Palette::indigo(), Palette::blueprint()};
        for (int k = 0; k < 3; ++k) {
            const int y = in.y + 14 + k * 40;
            img.fillRect(in.right() - 120, y, 20, 26, pals[k].paperFront);
            img.fillRect(in.right() - 100, y, 14, 26, pals[k].paperBack);
            img.strokeRect(in.right() - 120, y, 34, 26, k == 0 ? theme::Accent : theme::Border);
            drawText(img, in.right() - 78, y + 10, upper(pals[k].name), ink);
        }
    }

    const std::string out = ctx.path("14_studio_ui");
    img.save(out);
    return {out};
}

} // namespace ori
