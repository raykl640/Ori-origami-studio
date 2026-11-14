// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Studio : folded preview and layer inspector   Owner : Edgar Kareithi (Layering)
//  File   : StudioPreview.cpp
//  Region 3: the folded model at the timeline position (in-progress folds
//  between steps), with the next fold's diagram, X-ray and exploded views.
//  Region 4: one row per layer - thumbnail, panel count, which side shows;
//  click a row to highlight that layer, click the eye to hide it.
// ============================================================================
#include "Studio.h"
#include "../src/curves/Curves.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/models/Models.h"
#include "../src/raster/Circle.h"
#include "../src/raster/Lines.h"
#include <algorithm>
#include <cmath>

namespace ori {

namespace {

BBox unionBounds(const std::vector<FoldState>& st) {
    BBox b = panelBounds(st.front().panels);
    for (const FoldState& s : st) {
        const BBox o = panelBounds(s.panels);
        b.min.x = std::min(b.min.x, o.min.x); b.min.y = std::min(b.min.y, o.min.y);
        b.max.x = std::max(b.max.x, o.max.x); b.max.y = std::max(b.max.y, o.max.y);
    }
    return b;
}

void eyeIcon(Image& img, int cx, int cy, bool open, Color c) {
    drawEllipse(img, cx, cy, 8, 5, c);
    fillCircle(img, cx, cy, 2, c);
    if (!open) drawStroke(img, {cx - 9.0, cy + 6.0}, {cx + 9.0, cy - 6.0}, Stroke::solid(c, 2));
}

} // namespace

void Studio::drawPreview(const Rect& r, const std::vector<FoldState>& st) {
    const Rect in = ui_.panel(r, "FOLDED PREVIEW", 3);
    Image& img = ui_.img();
    const Palette pal = palette();
    const int n = stepCount();
    const int k = std::clamp(int(std::floor(timeline + 1e-9)), 0, n);
    const double frac = timeline - k;
    const FoldState& s = st[std::size_t(k)];

    // toolbar: where we are + view toggles
    char info[160];
    if (frac > 1e-6 && k < n)
        std::snprintf(info, sizeof info, "FOLDING STEP %d OF %d  (%d%%)", k + 1, n, int(std::lround(frac * 100)));
    else
        std::snprintf(info, sizeof info, "%s  -  %d PANEL%s, %d LAYER%s", k == 0 ? "FLAT SHEET" : k == n ? "FINISHED MODEL" : ("AFTER STEP " + std::to_string(k)).c_str(),
                      int(s.panels.size()), s.panels.size() == 1 ? "" : "S", s.layerCount(), s.layerCount() == 1 ? "" : "S");
    ui_.label(in.x + 10, in.y + 9, info);
    int bx = in.right() - 8;
    auto viewToggle = [&](const std::string& label, bool on) {
        const int w = textWidth(label) + 16;
        bx -= w + 4;
        return ui_.button(Rect{bx, in.y + 3, w, 20}, label, on);
    };
    if (viewToggle("EXPLODED (E)", exploded)) exploded = !exploded;
    if (viewToggle("X-RAY (X)", render.alpha < 1.0)) render.alpha = render.alpha < 1.0 ? 1.0 : 0.5;
    if (viewToggle("DIAGRAM (D)", showDiagram)) showDiagram = !showDiagram;

    const Rect canvas = previewCanvas(r);
    previewView = fitView(unionBounds(st), canvas, 34);
    if (int(hiddenLayers.size()) != s.layerCount()) hiddenLayers.assign(std::size_t(s.layerCount()), false);

    RenderOptions opt = render;
    opt.whiteSideUp = doc.seq.whiteSideUp;
    if (exploded) {
        drawExplodedView(img, s, inset(canvas, 20), pal, 18, opt);
    } else if (frac > 1e-6 && k < n) {
        drawPanels(img, FoldEngine::inProgress(s, doc.seq.steps[std::size_t(k)], easeInOut(frac)), previewView, pal, opt);
    } else {
        std::vector<FoldPanel> shown;
        for (const FoldPanel& p : s.bottomToTop())
            if (p.layer >= int(hiddenLayers.size()) || !hiddenLayers[std::size_t(p.layer)]) shown.push_back(p);
        drawPanels(img, shown, previewView, pal, opt);
        if (highlightLayer >= 0)
            for (const FoldPanel& p : s.panels)
                if (p.layer == highlightLayer) {
                    std::vector<Vec2> o;
                    for (const Vec2& v : p.outline) o.push_back(previewView.apply(v));
                    drawPolyline(img, o, Stroke::solid(ui::Accent, 3), true);
                }
        if (k == n && n > 0) drawDecoration(img, doc.seq.decoration, previewView);
        if (k < n && showDiagram && !ui_.dragging("foldline")) {
            drawFoldDiagram(img, s, doc.seq.steps[std::size_t(k)], previewView, pal);
            ui_.label(canvas.x + 10, canvas.y + 4, "NEXT: STEP " + std::to_string(k + 1) + " - " +
                      std::string(doc.seq.steps[std::size_t(k)].kind == FoldKind::Valley ? "VALLEY" : "MOUNTAIN"), ui::Muted);
        }
    }
    if (n == 0 && !ui_.anyDrag())
        ui_.labelCentered(Rect{canvas.x, canvas.bottom() - 26, canvas.w, 14}, "DRAG ACROSS THE PAPER TO MAKE YOUR FIRST FOLD", ui::Accent);
    else if (k < n && !ui_.anyDrag() && !exploded && frac < 1e-6)
        ui_.labelCentered(Rect{canvas.x, canvas.bottom() - 26, canvas.w, 14},
                          "DRAG ACROSS THE MODEL TO INSERT A FOLD HERE (BEFORE STEP " + std::to_string(k + 1) + ")", ui::Muted);

    drawTimeline(Rect{in.x + 8, canvas.bottom() + 4, in.w - 16, 50});
}

void Studio::drawLayerPanel(const Rect& r, const std::vector<FoldState>& st) {
    const Rect in = ui_.panel(r, "LAYER INSPECTOR", 4);
    Image& img = ui_.img();
    const int n = stepCount();
    const int k = std::clamp(int(std::floor(timeline + 1e-9)), 0, n);
    const FoldState& s = st[std::size_t(k)];
    const int layers = s.layerCount();
    if (int(hiddenLayers.size()) != layers) hiddenLayers.assign(std::size_t(layers), false);

    ui_.label(in.x + 8, in.y + 9, std::to_string(layers) + (layers == 1 ? " LAYER" : " LAYERS") + ", TOP FIRST");
    const bool anyHidden = std::find(hiddenLayers.begin(), hiddenLayers.end(), true) != hiddenLayers.end();
    if (ui_.button(Rect{in.right() - 82, in.y + 3, 76, 20}, "SHOW ALL", false, anyHidden || highlightLayer >= 0)) {
        hiddenLayers.assign(std::size_t(layers), false);
        highlightLayer = -1;
    }

    const Rect list{in.x + 4, in.y + 28, in.w - 8, in.h - 32};
    const int rowH = 72;
    ui_.scrollArea(list, layers * rowH, layerScroll);
    ui_.beginClip(list);
    RenderOptions opt = render;
    opt.whiteSideUp = doc.seq.whiteSideUp;
    for (int i = 0; i < layers; ++i) {
        const int layer = layers - 1 - i;
        const Rect row{list.x, list.y + i * rowH - layerScroll, list.w - 6, rowH - 6};
        if (row.bottom() < list.y || row.y > list.bottom()) continue;
        const bool interactive = row.y >= list.y && row.bottom() <= list.bottom();
        const bool hl = layer == highlightLayer;
        const bool hidden = hiddenLayers[std::size_t(layer)];
        img.fillRect(row.x, row.y, row.w, row.h, hl ? ui::AccentBg : Color(250, 248, 244));
        img.strokeRect(row.x, row.y, row.w, row.h, hl ? ui::Accent : ui::Border);
        drawLayerThumbnail(img, s, layer, Rect{row.x + 4, row.y + 4, 86, row.h - 8}, palette(), opt);
        int count = 0, coloured = 0;
        for (const FoldPanel& p : s.panels)
            if (p.layer == layer) { ++count; coloured += (p.faceUp != opt.whiteSideUp); }
        ui_.label(row.x + 98, row.y + 14, "LAYER " + std::to_string(layer), hidden ? ui::Muted : ui::Text);
        ui_.label(row.x + 98, row.y + 28, std::to_string(count) + (count == 1 ? " PANEL" : " PANELS"), ui::Muted);
        ui_.label(row.x + 98, row.y + 42, coloured == count ? "COLOURED SIDE" : coloured == 0 ? "WHITE SIDE" : "MIXED", ui::Muted);
        const Rect eye{row.right() - 34, row.y + row.h / 2 - 12, 28, 24};
        eyeIcon(img, eye.x + 14, eye.y + 12, !hidden, ui_.hover(eye) ? ui::Accent : ui::Text);
        if (interactive && ui_.clickIn(eye)) {
            hiddenLayers[std::size_t(layer)] = !hidden;
            setStatus(std::string(hidden ? "SHOWING" : "HIDING") + " LAYER " + std::to_string(layer));
        } else if (interactive && ui_.clickIn(row)) {
            highlightLayer = hl ? -1 : layer;
        }
    }
    ui_.endClip();
}

} // namespace ori
