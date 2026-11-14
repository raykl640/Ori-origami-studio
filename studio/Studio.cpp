// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Studio : frame, layout, top bar, undo, shortcuts   Owner : Ray Lukorito (App shell)
//  File   : Studio.cpp
// ============================================================================
#include "Studio.h"
#include "../demos/Demos.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/models/Models.h"
#include "../src/raster/Lines.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <set>

namespace ori {

namespace fs = std::filesystem;

namespace {

std::string upper(std::string s) {
    for (char& c : s) c = char(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

// Text that fits in `width` pixels of the 5x7 font.
std::string fit(const std::string& s, int width) {
    const std::size_t n = std::size_t(std::max(1, (width + 1) / 6));
    return s.size() <= n ? s : s.substr(0, n > 2 ? n - 2 : n) + "..";
}

int buttonWidth(const std::string& label) { return textWidth(label) + 18; }

} // namespace

Vec2 constrainAngle(const Vec2& a, const Vec2& b) {
    const Vec2 d = b - a;
    const double len = d.length();
    if (len < 1e-9) return b;
    const double step = degToRad(22.5);
    const double ang = std::round(d.angle() / step) * step;
    return a + Vec2{std::cos(ang), std::sin(ang)} * len;
}

// ----------------------------------------------------------------- set-up --
Studio::Studio() {
    const OrigamiModel dog = dogFaceModel();
    doc.seq = dog.toSequence();
    doc.cp = CreasePattern(Polygon::rectangle(0, 0, 400, 400), "Untitled pattern");
    doc.cp.gridStep = 50;
    savedFold_ = doc.seq.toText();
    savedCp_ = doc.cp.toText();
    timeline = stepCount();
    setStatus("WELCOME TO ORI STUDIO - DRAG ACROSS THE PREVIEW TO ADD A FOLD, F1 FOR HELP");
}

Palette Studio::palette() const {
    switch (paletteIndex) {
        case 1: return Palette::indigo();
        case 2: return Palette::blueprint();
        default: return Palette::classic();
    }
}

std::string Studio::windowTitle() const {
    const bool dirty = tab == Tab::Fold ? foldDirty() : cpDirty();
    return "ORI Studio - " + baseName() + (dirty ? " *" : "");
}

bool Studio::needsAnimation() const { return playing; }

void Studio::requestQuit() {
    if (foldDirty() || cpDirty()) overlay = Overlay::ConfirmQuit;
    else exit_ = true;
}

// ------------------------------------------------------------------- undo --
void Studio::checkpoint() {
    undo_.push_back(doc);
    if (undo_.size() > 200) undo_.erase(undo_.begin());
    redo_.clear();
}

namespace {
void clampSelections(Studio& s) {
    if (s.selectedStep >= s.stepCount()) s.selectedStep = s.stepCount() - 1;
    if (s.selectedCrease >= int(s.doc.cp.creases.size())) s.selectedCrease = -1;
    s.timeline = std::clamp(s.timeline, 0.0, double(s.stepCount()));
}
} // namespace

bool Studio::undo() {
    if (undo_.empty()) { setStatus("NOTHING TO UNDO"); return false; }
    redo_.push_back(doc);
    doc = undo_.back();
    undo_.pop_back();
    clampSelections(*this);
    setStatus("UNDONE");
    return true;
}

bool Studio::redo() {
    if (redo_.empty()) { setStatus("NOTHING TO REDO"); return false; }
    undo_.push_back(doc);
    doc = redo_.back();
    redo_.pop_back();
    clampSelections(*this);
    setStatus("REDONE");
    return true;
}

// ----------------------------------------------------------------- layout --
void Studio::layout(int w, int h) {
    const int top = 52, bottom = h - 30, contentH = bottom - top;
    if (tab == Tab::Fold) {
        const int leftW = 300, rightW = 270;
        patternRect = Rect{8, top, leftW, std::min(330, contentH * 44 / 100)};
        sequenceRect = Rect{8, patternRect.bottom() + 8, leftW, bottom - patternRect.bottom() - 8};
        layerRect = Rect{w - rightW - 8, top, rightW, contentH};
        const int cx = leftW + 16, cw = layerRect.x - 8 - cx;
        paletteRect = Rect{cx, bottom - 160, cw, 160};
        previewRect = Rect{cx, top, cw, paletteRect.y - 8 - top};
    } else {
        canvasRect = Rect{226, top, w - 226 - 266, contentH};
    }
}

// ------------------------------------------------------------------ frame --
void Studio::frame(Image& target, const UiInput& input, double dt) {
    statusAge += dt;
    if (playing) advancePlayback(dt);
    for (const std::string& f : input.droppedFiles) open(f);

    const bool modal = overlay != Overlay::None;
    UiInput panelInput = input;
    if (modal) {                       // dialogs are modal: the studio behind them sees no input
        panelInput.clearEvents();
        panelInput.mouseX = panelInput.mouseY = -10000;
        panelInput.mouseDown = false;
    }
    target.clear(ui::Bg);
    layout(target.width(), target.height());
    ui_.begin(target, panelInput);
    if (!modal) handleGlobalKeys();

    drawTopBar(Rect{0, 0, target.width(), 44});
    if (tab == Tab::Fold) {
        // Panels can edit the sequence (a click, a finished drag), so each
        // group works from freshly folded states - folding is cheap.
        auto fresh = [&] {
            timeline = std::clamp(timeline, 0.0, double(stepCount()));
            return states();
        };
        drawGeneratedPattern(patternRect, fresh());
        drawSequencePanel(sequenceRect);
        {
            const auto st = fresh();
            drawPreview(previewRect, st);
            handleFoldTool(st);
        }
        const auto st = fresh();
        const FoldState& shown = st[std::size_t(std::lround(std::floor(timeline + 1e-9)))];
        drawLayerPanel(layerRect, st);
        drawPalettePanel(paletteRect, shown.layerCount());
    } else {
        drawCreaseTab(Rect{8, 52, target.width() - 16, target.height() - 82});
    }
    drawStatusBar(Rect{0, target.height() - 24, target.width(), 24});

    if (modal) {
        ui_.begin(target, input);
        drawOverlay();
    }
}

// --------------------------------------------------------------- top bar --
void Studio::drawTopBar(const Rect& r) {
    Image& img = ui_.img();
    img.fillRect(r.x, r.y, r.w, r.h, ui::Bar);
    img.fillRect(r.x, r.bottom() - 3, r.w, 3, ui::Accent);
    drawText(img, 12, 12, "ORI", ui::Accent, 3);
    drawText(img, 70, 10, "STUDIO", colors::White, 2);
    const bool dirty = tab == Tab::Fold ? foldDirty() : cpDirty();
    drawText(img, 70, 28, upper(fit(baseName(), 150)) + (dirty ? " *" : ""), Color(175, 170, 162));

    int x = 240;
    const struct { const char* label; Tab t; } tabs[] = {{"FOLD SEQUENCE", Tab::Fold}, {"CREASE PATTERN", Tab::Crease}};
    for (const auto& t : tabs) {
        const Rect b{x, 9, buttonWidth(t.label) + 10, 26};
        if (ui_.button(b, t.label, tab == t.t)) {
            tab = t.t;
            ui_.cancelDrag();
        }
        x = b.right() + 6;
    }

    const char* right[] = {"NEW", "OPEN", "SAVE", "SAVE AS", "EXPORT", "UNDO", "REDO", "HELP"};
    int total = 0;
    for (const char* l : right) total += buttonWidth(l) + 6;
    int bx = std::max(x + 20, r.right() - 10 - total);
    for (const char* l : right) {
        const std::string label = l;
        const Rect b{bx, 9, buttonWidth(l), 26};
        const bool enabled = !(label == "UNDO" && undo_.empty()) && !(label == "REDO" && redo_.empty());
        if (ui_.button(b, label, false, enabled)) {
            if (label == "NEW") overlay = Overlay::New;
            else if (label == "OPEN") { overlay = Overlay::Open; openScroll_ = 0; }
            else if (label == "SAVE") save();
            else if (label == "SAVE AS") { textBuffer_ = baseName(); overlay = Overlay::SaveAs; }
            else if (label == "EXPORT") exportCurrent();
            else if (label == "UNDO") undo();
            else if (label == "REDO") redo();
            else if (label == "HELP") overlay = Overlay::Help;
        }
        bx = b.right() + 6;
    }
}

void Studio::drawStatusBar(const Rect& r) {
    Image& img = ui_.img();
    img.fillRect(r.x, r.y, r.w, r.h, Color(226, 221, 211));
    img.fillRect(r.x, r.y, r.w, 1, ui::Border);
    std::string hint;
    if (tab == Tab::Fold)
        hint = "DRAG ON THE PREVIEW: NEW FOLD   SHIFT: 22.5 DEG   M/V: KIND   F: FLIP   SPACE: PLAY   F1: HELP";
    else
        hint = tool == CreaseTool::Draw ? "DRAG: DRAW CREASE   SHIFT: 22.5 DEG   M/V/A: TYPE   S: SELECT   E: ERASE   F1: HELP"
             : tool == CreaseTool::Select ? "CLICK: SELECT   T: TOGGLE M/V   DEL: DELETE   D: DRAW   F1: HELP"
                                          : "CLICK A CREASE TO ERASE IT   D: DRAW   S: SELECT   F1: HELP";
    const int hintW = textWidth(hint);
    drawText(img, 10, r.y + 9, fit(upper(status), r.w - hintW - 40), statusAge < 6 ? ui::Text : ui::Muted);
    drawText(img, r.right() - hintW - 10, r.y + 9, hint, ui::Muted);
}

// -------------------------------------------------------------- keyboard --
void Studio::handleGlobalKeys() {
    for (const KeyPress& k : ui_.input().keys) {
        if (k.ctrl) {
            if (k.is('z') && k.shift) redo();
            else if (k.is('z')) undo();
            else if (k.is('y')) redo();
            else if (k.is('s') && k.shift) { textBuffer_ = baseName(); overlay = Overlay::SaveAs; }
            else if (k.is('s')) save();
            else if (k.is('o')) { overlay = Overlay::Open; openScroll_ = 0; }
            else if (k.is('n')) overlay = Overlay::New;
            else if (k.is('e')) exportCurrent();
            else if (k.is('q') || k.is('w')) requestQuit();
            else if (k.is('1')) tab = Tab::Fold;
            else if (k.is('2')) tab = Tab::Crease;
            continue;
        }
        if (k.key == Key::F1 || k.is('?')) { overlay = Overlay::Help; continue; }
        if (k.key == Key::Tab) { tab = tab == Tab::Fold ? Tab::Crease : Tab::Fold; ui_.cancelDrag(); continue; }
        if (tab == Tab::Fold) foldKey(k); else creaseKey(k);
    }
}

} // namespace ori
