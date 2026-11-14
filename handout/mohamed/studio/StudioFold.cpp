// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Studio : fold-sequence panel and fold tool   Owner : Mohamed Bile (Fold engine)
//  File   : StudioFold.cpp
//  * Draw a fold: drag across the folded preview. The line snaps to panel
//    corners and edge midpoints (then to the grid); Shift locks it to 22.5
//    degree angles. While dragging, a translucent ghost shows the result.
//  * The sequence list: select, rename, toggle valley/mountain, flip the
//    moving side, reorder and delete steps.
//  * Region 1: the crease pattern the whole sequence leaves on the sheet.
// ============================================================================
#include "Studio.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/models/Models.h"
#include "../src/raster/Circle.h"
#include "../src/raster/Lines.h"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace ori {

namespace {

std::string upper(std::string s) {
    for (char& c : s) c = char(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

std::string fit(const std::string& s, int width) {
    const std::size_t n = std::size_t(std::max(1, (width + 1) / 6));
    return s.size() <= n ? s : s.substr(0, n > 2 ? n - 2 : n) + "..";
}

bool isAutoLabel(const std::string& l) { return l == "Valley fold" || l == "Mountain fold"; }
std::string autoLabel(FoldKind k) { return k == FoldKind::Valley ? "Valley fold" : "Mountain fold"; }

// Paper area on each side of a fold line.
void sideAreas(const FoldState& s, const Vec2& a, const Vec2& b, double& moving, double& staying) {
    moving = staying = 0;
    for (const FoldPanel& p : s.panels) {
        moving += std::fabs(FoldEngine::signedArea(FoldEngine::clipHalfPlane(p.outline, a, b, +1)));
        staying += std::fabs(FoldEngine::signedArea(FoldEngine::clipHalfPlane(p.outline, a, b, -1)));
    }
}

} // namespace

// ------------------------------------------------------------- operations --
bool Studio::addFold(const Vec2& a, const Vec2& b, FoldKind kind, int index) {
    index = std::clamp(index, 0, stepCount());
    if (distance(a, b) < 1e-6) { setStatus("DRAG A LONGER LINE TO MAKE A FOLD"); return false; }
    const FoldState s = doc.seq.run()[std::size_t(index)];
    double moving = 0, staying = 0;
    sideAreas(s, a, b, moving, staying);
    if (moving < 1e-3) {
        setStatus("THAT LINE MISSES THE PAPER - NOTHING WOULD MOVE. THE PAPER RIGHT OF YOUR DRAG MOVES (F FLIPS IT)");
        return false;
    }
    checkpoint();
    doc.seq.steps.insert(doc.seq.steps.begin() + index, FoldStep{a, b, kind, autoLabel(kind)});
    selectedStep = index;
    timeline = index + 1;
    hiddenLayers.clear();
    highlightLayer = -1;
    std::string msg = "ADDED " + upper(autoLabel(kind)) + " AS STEP " + std::to_string(index + 1);
    if (staying < 1e-3) msg += " - IT TURNS THE WHOLE MODEL OVER";
    if (index + 1 < stepCount()) msg += " - LATER STEPS NOW FOLD THE NEW SHAPE";
    setStatus(msg + "   (CTRL+Z UNDOES)");
    return true;
}

void Studio::deleteStep(int i) {
    if (i < 0 || i >= stepCount()) return;
    checkpoint();
    doc.seq.steps.erase(doc.seq.steps.begin() + i);
    selectedStep = std::min(i, stepCount() - 1);
    timeline = std::min(timeline, double(stepCount()));
    hiddenLayers.clear();
    setStatus("DELETED STEP " + std::to_string(i + 1));
}

void Studio::moveStep(int i, int delta) {
    const int j = i + delta;
    if (i < 0 || i >= stepCount() || j < 0 || j >= stepCount()) return;
    checkpoint();
    std::swap(doc.seq.steps[std::size_t(i)], doc.seq.steps[std::size_t(j)]);
    selectedStep = j;
    timeline = j;
    hiddenLayers.clear();
    setStatus("MOVED STEP " + std::to_string(i + 1) + " TO POSITION " + std::to_string(j + 1));
}

void Studio::toggleStepKind(int i) {
    if (i < 0 || i >= stepCount()) return;
    checkpoint();
    FoldStep& s = doc.seq.steps[std::size_t(i)];
    s.kind = opposite(s.kind);
    if (isAutoLabel(s.label)) s.label = autoLabel(s.kind);
    setStatus("STEP " + std::to_string(i + 1) + " IS NOW A " + upper(foldKindName(s.kind)) + " FOLD");
}

void Studio::flipStep(int i) {
    if (i < 0 || i >= stepCount()) return;
    checkpoint();
    std::swap(doc.seq.steps[std::size_t(i)].a, doc.seq.steps[std::size_t(i)].b);
    setStatus("STEP " + std::to_string(i + 1) + ": THE OTHER SIDE OF THE CREASE MOVES NOW");
}

void Studio::setTimeline(double t) {
    timeline = std::clamp(t, 0.0, double(stepCount()));
    playing = false;
}

// ---------------------------------------------------------------- snapping --
Vec2 Studio::snapFoldPoint(const Vec2& screen, const FoldState& s, bool* snapped) const {
    const double radius = 10.0;   // pixels
    const Mat3 inv = previewView.inverse();
    double best = radius;
    Vec2 hit;
    bool found = false;
    auto consider = [&](const Vec2& m) {
        const double d = distance(previewView.apply(m), screen);
        if (d <= best) { best = d; hit = m; found = true; }
    };
    for (const FoldPanel& p : s.panels)
        for (std::size_t i = 0; i < p.outline.size(); ++i) {
            consider(p.outline[i]);
            consider((p.outline[i] + p.outline[(i + 1) % p.outline.size()]) * 0.5);
        }
    if (!found && foldSnapGrid && !s.sheet.empty()) {
        const BBox b = Polygon(s.sheet).bounds();
        const double step = std::max(b.width(), b.height()) / 8.0;
        const Vec2 m = inv.apply(screen);
        const Vec2 g{b.min.x + std::round((m.x - b.min.x) / step) * step, b.min.y + std::round((m.y - b.min.y) / step) * step};
        if (distance(previewView.apply(g), screen) <= radius) { hit = g; found = true; }
    }
    if (snapped) *snapped = found;
    return found ? hit : inv.apply(screen);
}

// ---------------------------------------------------------------- fold tool --
void Studio::handleFoldTool(const std::vector<FoldState>& st) {
    if (exploded) return;
    const Rect canvas = previewCanvas(previewRect);
    const UiInput& in = ui_.input();
    const Vec2 mouse{double(in.mouseX), double(in.mouseY)};
    const int k = int(std::lround(timeline));
    const bool integral = std::fabs(timeline - k) < 1e-6;
    const FoldState& s = st[std::size_t(std::clamp(k, 0, int(st.size()) - 1))];

    if (ui_.pressAvailable() && ui_.hover(canvas)) {
        ui_.usePress();
        if (playing || !integral) {          // first click stops / settles the animation
            setTimeline(std::round(timeline));
            return;
        }
        dragStart_ = snapFoldPoint(mouse, s, &dragSnapped_);
        dragEnd_ = dragStart_;
        ui_.startDrag("foldline");
    }

    const bool active = ui_.dragging("foldline");
    const bool ended = !active && ui_.dragEnded("foldline");
    if (!active && !ended) {
        // Hover feedback: show where a fold would start.
        if (ui_.hover(canvas) && integral && !playing && !ui_.anyDrag()) {
            bool sn = false;
            const Vec2 p = previewView.apply(snapFoldPoint(mouse, s, &sn));
            if (sn) drawRing(ui_.img(), int(std::lround(p.x)), int(std::lround(p.y)), 6, 2, ui::Accent);
        }
        return;
    }

    bool snappedEnd = false;
    dragEnd_ = snapFoldPoint(mouse, s, &snappedEnd);
    if (in.shift) dragEnd_ = constrainAngle(dragStart_, dragEnd_);
    const Vec2 a = previewView.apply(dragStart_), b = previewView.apply(dragEnd_);

    if (ended) {
        if (distance(a, b) >= 6.0) addFold(dragStart_, dragEnd_, newKind, k);
        return;
    }

    // Live preview of the fold being drawn.
    Image& img = ui_.img();
    if (distance(a, b) >= 6.0) {
        const FoldStep tmp{dragStart_, dragEnd_, newKind, ""};
        RenderOptions ghost = render;
        ghost.whiteSideUp = doc.seq.whiteSideUp;
        ghost.alpha = 0.45;
        ghost.shadow = false;
        drawFoldState(img, FoldEngine::fold(s, tmp), previewView, palette(), ghost);
        drawFoldDiagram(img, s, tmp, previewView, palette());
    }
    drawStroke(img, a, b, Stroke::solid(ui::Accent, 2));
    for (const Vec2& p : {a, b}) {
        fillCircle(img, int(std::lround(p.x)), int(std::lround(p.y)), 4, colors::White);
        drawRing(img, int(std::lround(p.x)), int(std::lround(p.y)), 5, 2, ui::Accent);
    }
    const double ang = std::fmod(radToDeg(-(dragEnd_ - dragStart_).angle()) + 360.0, 180.0);
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.1F DEG%s", ang, snappedEnd ? "  SNAPPED" : "");
    ui_.label(int(b.x) + 10, int(b.y) + 8, buf, ui::Text);
}

// ---------------------------------------------------------------- the list --
void Studio::drawSequencePanel(const Rect& r) {
    const Rect in = ui_.panel(r, "FOLD SEQUENCE (" + std::to_string(stepCount()) + " FOLDS)", 2);
    Image& img = ui_.img();
    const Palette pal = palette();
    const int controlsH = 112;
    const Rect list{in.x + 4, in.y + 4, in.w - 8, in.h - controlsH - 8};
    const int rowH = 36;
    const int n = stepCount();
    ui_.scrollArea(list, (n + 1) * rowH, sequenceScroll);

    ui_.beginClip(list);
    for (int i = 0; i <= n; ++i) {
        const Rect row{list.x, list.y + i * rowH - sequenceScroll, list.w - 6, rowH - 3};
        if (row.bottom() < list.y || row.y > list.bottom()) continue;
        const bool interactive = row.y >= list.y && row.bottom() <= list.bottom();
        if (i == n) {
            ui_.label(row.x + 8, row.y + 14, fit("+ DRAG ON THE PREVIEW TO ADD STEP " + std::to_string(n + 1), row.w - 10), ui::Muted);
            continue;
        }
        const FoldStep& step = doc.seq.steps[std::size_t(i)];
        const bool sel = i == selectedStep;
        const bool current = int(std::floor(timeline + 1e-9)) == i && timeline < n;
        img.fillRect(row.x, row.y, row.w, row.h, sel ? ui::AccentBg : current ? Color(246, 243, 236) : colors::White);
        img.strokeRect(row.x, row.y, row.w, row.h, sel ? ui::Accent : ui::Border);
        ui_.badge(Rect{row.x + 6, row.y + 8, 20, 18}, std::to_string(i + 1), pal.valley);
        // per-row buttons first so they take the click before the row does
        const int bx = row.right() - 4;
        const Rect del{bx - 18, row.y + 8, 18, 18}, down{bx - 44, row.y + 8, 24, 18}, up{bx - 70, row.y + 8, 24, 18};
        const Rect kind{bx - 100, row.y + 8, 24, 18};
        const bool valley = step.kind == FoldKind::Valley;
        img.fillRect(kind.x, kind.y, kind.w, kind.h, valley ? pal.valley : pal.mountain);
        ui_.labelCentered(kind, valley ? "V" : "M", colors::White);
        if (interactive && ui_.clickIn(kind)) { toggleStepKind(i); selectedStep = i; }
        if (ui_.button(up, "UP", false, interactive && i > 0)) moveStep(i, -1);
        if (ui_.button(down, "DN", false, interactive && i + 1 < n)) moveStep(i, +1);
        if (ui_.button(del, "X", false, interactive)) { deleteStep(i); break; }
        ui_.label(row.x + 32, row.y + 8, fit(upper(step.label), kind.x - row.x - 38));
        ui_.label(row.x + 32, row.y + 19, upper(foldKindName(step.kind)) + (valley ? " - ON TOP" : " - BEHIND"), ui::Muted);
        if (interactive && ui_.clickIn(row)) {
            selectedStep = i;
            setTimeline(i);
            if (ui_.input().doubleClick) { textBuffer_ = step.label; overlay = Overlay::Rename; }
        }
    }
    ui_.endClip();

    // controls
    int y = list.bottom() + 8;
    ui_.label(in.x + 8, y + 7, "NEXT FOLD:");
    if (ui_.button(Rect{in.x + 76, y, 96, 22}, "VALLEY (V)", newKind == FoldKind::Valley)) newKind = FoldKind::Valley;
    if (ui_.button(Rect{in.x + 178, y, 110, 22}, "MOUNTAIN (M)", newKind == FoldKind::Mountain)) newKind = FoldKind::Mountain;
    y += 28;
    const bool has = selectedStep >= 0 && selectedStep < n;
    const int bw = (in.w - 16 - 18) / 4;
    if (ui_.button(Rect{in.x + 8, y, bw, 22}, "FLIP (F)", false, has)) flipStep(selectedStep);
    if (ui_.button(Rect{in.x + 14 + bw, y, bw, 22}, "M/V", false, has)) toggleStepKind(selectedStep);
    if (ui_.button(Rect{in.x + 20 + 2 * bw, y, bw, 22}, "RENAME", false, has)) {
        textBuffer_ = doc.seq.steps[std::size_t(selectedStep)].label;
        overlay = Overlay::Rename;
    }
    if (ui_.button(Rect{in.x + 26 + 3 * bw, y, bw, 22}, "DELETE", false, has)) deleteStep(selectedStep);
    y += 30;
    bool white = doc.seq.whiteSideUp;
    if (ui_.toggle(Rect{in.x + 8, y, 160, 18}, "START WHITE SIDE UP", white)) {
        doc.seq.whiteSideUp = !white;   // undo needs the old value
        checkpoint();
        doc.seq.whiteSideUp = white;
    }
    ui_.toggle(Rect{in.x + 176, y, 110, 18}, "GRID SNAP (G)", foldSnapGrid);
}

// -------------------------------------------------------- generated pattern --
void Studio::drawGeneratedPattern(const Rect& r, const std::vector<FoldState>& st) {
    const Rect in = ui_.panel(r, "CREASE PATTERN (GENERATED)", 1);
    Image& img = ui_.img();
    const Palette pal = palette();
    const CreasePattern cp = creasePatternOf(st.back(), doc.seq.name);
    const Rect area{in.x, in.y, in.w, in.h - 32};
    const Mat3 view = fitView(cp.paper.bounds(), area, 16);
    CreaseViewOptions o;
    o.showGrid = false;
    drawCreasePattern(img, cp, view, pal, o);
    // Creases made by the selected step stand out.
    for (const FoldCrease& c : st.back().creases)
        if (c.step == selectedStep) drawStroke(img, view.apply(c.a), view.apply(c.b), Stroke::solid(ui::Accent.withAlpha(150), 5));
    ui_.label(in.x + 8, in.bottom() - 22, std::to_string(cp.count(CreaseType::Mountain)) + " M  " +
              std::to_string(cp.count(CreaseType::Valley)) + " V", ui::Muted);
    if (ui_.button(Rect{in.right() - 150, in.bottom() - 28, 142, 22}, "EDIT IN CP TAB")) {
        checkpoint();
        doc.cp = cp;
        doc.cp.name = doc.seq.name.empty() ? "Generated pattern" : doc.seq.name + " pattern";
        doc.cp.gridStep = std::max(cp.paper.bounds().width(), cp.paper.bounds().height()) / 8;
        cpPath.clear();
        tab = Tab::Crease;
        selectedCrease = -1;
        setStatus("COPIED THE GENERATED CREASE PATTERN INTO THE CREASE PATTERN TAB");
    }
}

// ------------------------------------------------------------------- keys --
void Studio::foldKey(const KeyPress& k) {
    const int n = stepCount();
    switch (k.key) {
        case Key::Escape:
            if (ui_.anyDrag()) ui_.cancelDrag(); else selectedStep = -1;
            return;
        case Key::Space:
            if (n == 0) { setStatus("ADD A FOLD FIRST"); return; }
            if (!playing && timeline >= n - 1e-9) timeline = 0;
            playing = !playing;
            return;
        case Key::Left:  setTimeline(std::ceil(timeline - 1e-9) - 1); return;
        case Key::Right: setTimeline(std::floor(timeline + 1e-9) + 1); return;
        case Key::Home:  setTimeline(0); return;
        case Key::End:   setTimeline(n); return;
        case Key::Up:
            if (k.alt) { moveStep(selectedStep, -1); return; }
            if (n) { selectedStep = std::max(0, selectedStep - 1); setTimeline(selectedStep); }
            return;
        case Key::Down:
            if (k.alt) { moveStep(selectedStep, +1); return; }
            if (n) { selectedStep = std::min(n - 1, selectedStep + 1); setTimeline(selectedStep); }
            return;
        case Key::Delete:
        case Key::Backspace: deleteStep(selectedStep); return;
        case Key::F2:
        case Key::Enter:
            if (selectedStep >= 0 && selectedStep < n) {
                textBuffer_ = doc.seq.steps[std::size_t(selectedStep)].label;
                overlay = Overlay::Rename;
            }
            return;
        default: break;
    }
    if (k.key != Key::Char) return;
    switch (k.ch) {
        case 'v': case 'm': {
            newKind = k.ch == 'v' ? FoldKind::Valley : FoldKind::Mountain;
            if (selectedStep >= 0 && selectedStep < n && doc.seq.steps[std::size_t(selectedStep)].kind != newKind)
                toggleStepKind(selectedStep);
            else
                setStatus(std::string("NEXT FOLD: ") + (newKind == FoldKind::Valley ? "VALLEY" : "MOUNTAIN"));
            break;
        }
        case 'f': flipStep(selectedStep); break;
        case '[': moveStep(selectedStep, -1); break;
        case ']': moveStep(selectedStep, +1); break;
        case 'g': foldSnapGrid = !foldSnapGrid; setStatus(foldSnapGrid ? "GRID SNAP ON" : "GRID SNAP OFF"); break;
        case 'd': showDiagram = !showDiagram; break;
        case 'x': render.alpha = render.alpha < 1.0 ? 1.0 : 0.5; break;
        case 'e': exploded = !exploded; break;
        default: break;
    }
}

} // namespace ori
