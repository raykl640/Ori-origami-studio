// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Studio : depth palette and animation timeline   Owner : Matthew Wachira (Colour & curves)
//  File   : StudioPalette.cpp
//  Region 5: live render settings - depth shading and its step per layer,
//  lighting with a draggable light-direction dial, outlines, shadow, X-ray
//  opacity and the three palettes.
//  The timeline: play / pause, scrub through every fold (positions between
//  steps show the fold in progress, eased by the Bezier timing curve).
// ============================================================================
#include "Studio.h"
#include "../src/raster/Circle.h"
#include "../src/raster/Lines.h"
#include "../src/shading/Palette.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ori {

namespace {
constexpr double kSecondsPerFold = 1.2;
}

void Studio::advancePlayback(double dt) {
    const int n = stepCount();
    if (n == 0) { playing = false; return; }
    timeline += dt / kSecondsPerFold;
    if (timeline >= n) {
        timeline = n;
        playing = false;
    }
}

void Studio::drawTimeline(const Rect& r) {
    Image& img = ui_.img();
    const int n = stepCount();
    const Rect play{r.x, r.y + 8, 44, 30};
    const bool canPlay = n > 0;
    if (ui_.button(play, playing ? "PAUSE" : "PLAY", playing, canPlay)) {
        if (!playing && timeline >= n - 1e-9) timeline = 0;
        playing = !playing;
    }
    const Rect track{play.right() + 18, r.y + 8, r.w - play.w - 90, 30};
    const int ty = track.y + 10;
    img.fillRect(track.x, ty - 2, track.w, 4, Color(225, 220, 210));
    if (n == 0) {
        ui_.label(track.x, ty + 12, "NO FOLDS YET", ui::Muted);
        return;
    }
    auto xAt = [&](double t) { return track.x + int(std::lround(t / n * (track.w - 1))); };
    img.fillRect(track.x, ty - 2, xAt(timeline) - track.x, 4, ui::Accent);
    for (int i = 0; i <= n; ++i) {
        const int x = xAt(i);
        const bool done = i <= timeline + 1e-9;
        fillCircle(img, x, ty, 4, done ? ui::Accent : Color(200, 195, 186));
        if (n <= 30 || i % 5 == 0) {
            const std::string t = std::to_string(i);
            ui_.label(x - textWidth(t) / 2, ty + 10, t, i == int(std::lround(timeline)) ? ui::Text : ui::Muted);
        }
    }
    const int hx = xAt(timeline);
    fillCircle(img, hx, ty, 7, colors::White);
    drawRing(img, hx, ty, 7, 2, ui::Text);

    // scrub: drag anywhere on the track; release settles on the nearest step
    if (ui_.clickIn(Rect{track.x - 8, track.y, track.w + 16, track.h})) ui_.startDrag("timeline");
    const bool dragging = ui_.dragging("timeline");
    const bool ended = !dragging && ui_.dragEnded("timeline");
    if (dragging || ended) {
        playing = false;
        const double t = std::clamp(double(ui_.input().mouseX - track.x) / std::max(1, track.w - 1), 0.0, 1.0) * n;
        timeline = ended ? std::round(t) : t;
        if (ended) selectedStep = int(timeline) < n ? int(timeline) : selectedStep;
    }
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.1F / %d", timeline, n);
    ui_.label(track.right() + 12, ty - 3, buf);
}

void Studio::drawPalettePanel(const Rect& r, int layers) {
    const Rect in = ui_.panel(r, "DEPTH PALETTE", 5);
    Image& img = ui_.img();
    const Palette pal = palette();

    // depth swatches for the current number of layers
    const int shown = std::clamp(layers, 1, 10);
    drawDepthPalette(img, in.x + 10, in.y + 12, 150, 56, pal, shown);
    ui_.label(in.x + 10, in.y + 92, "FRONT / BACK BY DEPTH", ui::Muted);

    // switches
    const int tx = in.x + 178;
    ui_.toggle(Rect{tx, in.y + 8, 130, 18}, "DEPTH SHADING", render.depthShading);
    ui_.toggle(Rect{tx, in.y + 30, 130, 18}, "LIGHTING", render.lighting);
    ui_.toggle(Rect{tx, in.y + 52, 130, 18}, "OUTLINES", render.outlines);
    ui_.toggle(Rect{tx, in.y + 74, 130, 18}, "DROP SHADOW", render.shadow);
    bool xray = render.alpha < 1.0;
    if (ui_.toggle(Rect{tx, in.y + 96, 130, 18}, "X-RAY", xray)) render.alpha = xray ? 0.5 : 1.0;

    // sliders
    const int sx = tx + 140, sw = std::max(80, in.right() - 200 - sx);
    char buf[48];
    std::snprintf(buf, sizeof buf, "DEPTH STEP PER LAYER  %.2F", render.depthPerLevel);
    ui_.label(sx, in.y + 10, buf);
    ui_.slider("depthstep", Rect{sx, in.y + 22, sw, 18}, render.depthPerLevel, 0.0, 0.25);
    double opacity = render.alpha < 1.0 ? render.alpha : 0.5;
    std::snprintf(buf, sizeof buf, "X-RAY OPACITY  %.2F", opacity);
    ui_.label(sx, in.y + 52, buf, render.alpha < 1.0 ? ui::Text : ui::Muted);
    if (ui_.slider("xray", Rect{sx, in.y + 64, sw, 18}, opacity, 0.15, 0.95)) render.alpha = opacity;
    ui_.label(sx, in.y + 96, "DRAG THE DIAL TO MOVE THE LIGHT", ui::Muted);

    // light-direction dial
    const int dcx = in.right() - 160, dcy = in.y + 52, dr = 28;
    fillCircle(img, dcx, dcy, dr, Color(250, 248, 244));
    drawCircle(img, dcx, dcy, dr, ui::Text);
    const Rect dial{dcx - dr, dcy - dr, 2 * dr + 1, 2 * dr + 1};
    if (ui_.clickIn(dial)) ui_.startDrag("light");
    if (ui_.dragging("light") || ui_.dragEnded("light")) {
        const Vec2 d{double(ui_.input().mouseX - dcx), double(ui_.input().mouseY - dcy)};
        if (d.length() > 2) render.lightDir = d.normalized();
    }
    const Vec2 l = render.lightDir.normalized();
    drawStroke(img, {double(dcx), double(dcy)}, {dcx + l.x * (dr - 4), dcy + l.y * (dr - 4)}, Stroke::solid(ui::Accent, 2));
    fillCircle(img, int(dcx + l.x * (dr - 4)), int(dcy + l.y * (dr - 4)), 4, ui::Accent);
    ui_.label(dcx - 14, dcy + dr + 6, "LIGHT", ui::Muted);

    // palettes
    const Palette pals[3] = {Palette::classic(), Palette::indigo(), Palette::blueprint()};
    for (int i = 0; i < 3; ++i) {
        const Rect b{in.right() - 112, in.y + 8 + i * 36, 104, 30};
        const bool sel = i == paletteIndex;
        img.fillRect(b.x, b.y, b.w, b.h, sel ? ui::AccentBg : ui::Button);
        img.strokeRect(b.x, b.y, b.w, b.h, sel ? ui::Accent : ui::Border);
        img.fillRect(b.x + 6, b.y + 6, 14, 18, pals[i].paperFront);
        img.fillRect(b.x + 20, b.y + 6, 10, 18, pals[i].paperBack);
        img.strokeRect(b.x + 6, b.y + 6, 24, 18, ui::Border);
        std::string name = pals[i].name;
        for (char& c : name) c = char(std::toupper(static_cast<unsigned char>(c)));
        ui_.label(b.x + 38, b.y + 12, name, sel ? ui::Accent : ui::Text);
        if (ui_.clickIn(b)) paletteIndex = i;
    }
}

} // namespace ori
