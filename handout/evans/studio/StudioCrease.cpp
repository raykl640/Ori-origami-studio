// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Studio : crease-pattern editor tab           Owner : Evans Mwendwa (Crease editor)
//  File   : StudioCrease.cpp
//  Draw mountain / valley / auxiliary creases with vertex-then-grid
//  snapping (Shift: 22.5 degree angles), select, toggle and erase them,
//  change the paper and grid, 2-colour the faces and check every interior
//  vertex against Maekawa (|M - V| = 2) and Kawasaki (alternate angles sum
//  to 180 degrees).
// ============================================================================
#include "Studio.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/raster/Circle.h"
#include "../src/raster/Clipping.h"
#include "../src/raster/Fill.h"
#include "../src/raster/Lines.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

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

const char* typeName(CreaseType t) {
    return t == CreaseType::Mountain ? "MOUNTAIN" : t == CreaseType::Valley ? "VALLEY" : "AUXILIARY";
}

Polygon paperShape(int kind) {
    if (kind == 1) return Polygon({{200, 0}, {400, 200}, {200, 400}, {0, 200}});
    if (kind == 2) return Polygon::rectangle(0, 0, 400, 200);
    return Polygon::rectangle(0, 0, 400, 400);
}

double paperSize(const CreasePattern& cp) {
    const BBox b = cp.paper.bounds();
    return std::max(b.width(), b.height());
}

} // namespace

// ------------------------------------------------------------- operations --
bool Studio::addCreaseFromEditor(const Vec2& a, const Vec2& b) {
    CreasePattern trial = doc.cp;
    const bool ok = fullLines ? trial.addFullCrease(a, b, drawType) : trial.addCrease(a, b, drawType, true, 1e-3);
    if (!ok) {
        setStatus("THAT CREASE IS OUTSIDE THE PAPER");
        return false;
    }
    checkpoint();
    doc.cp = trial;
    selectedCrease = int(doc.cp.creases.size()) - 1;
    const Crease& c = doc.cp.creases.back();
    char buf[96];
    std::snprintf(buf, sizeof buf, "ADDED %s CREASE, LENGTH %.1F", typeName(c.type), c.segment().length());
    setStatus(buf);
    return true;
}

void Studio::setPaper(int kind) {
    CreasePattern next(paperShape(kind), doc.cp.name);
    next.gridStep = paperSize(next) / 8;
    for (const Crease& c : doc.cp.creases) next.addCrease(c.a, c.b, c.type, true, 1e-3);   // trimmed to the new sheet
    doc.cp = next;
    selectedCrease = -1;
}

std::vector<Studio::VertexCheck> Studio::flatFoldabilityChecks() const {
    std::vector<VertexCheck> out;
    const CreasePattern s = doc.cp.split();
    const auto edges = s.paper.edges();
    std::vector<Vec2> verts;
    for (const Crease& c : s.creases) {
        if (c.type == CreaseType::Aux) continue;
        for (const Vec2& p : {c.a, c.b}) {
            bool dup = false;
            for (const Vec2& q : verts) dup = dup || nearlyEqual(p, q, 1e-6);
            if (!dup) verts.push_back(p);
        }
    }
    for (const Vec2& v : verts) {
        bool onEdge = false;
        for (const Segment& e : edges) onEdge = onEdge || distanceToSegment(v, e) < 1e-6;
        if (onEdge) continue;                           // the rules apply to interior vertices
        std::vector<double> angles;
        int m = 0, vv = 0;
        for (const Crease& c : s.creases) {
            if (c.type == CreaseType::Aux) continue;
            Vec2 other;
            if (nearlyEqual(c.a, v, 1e-6)) other = c.b;
            else if (nearlyEqual(c.b, v, 1e-6)) other = c.a;
            else continue;
            angles.push_back((other - v).angle());
            (c.type == CreaseType::Mountain ? m : vv)++;
        }
        VertexCheck chk;
        chk.p = v;
        chk.maekawa = std::abs(m - vv) == 2;
        std::sort(angles.begin(), angles.end());
        if (angles.size() % 2 != 0 || angles.size() < 2) {
            chk.kawasaki = false;
        } else {
            double alt = 0;
            for (std::size_t i = 0; i < angles.size(); i += 2) {
                const double next = i + 1 < angles.size() ? angles[i + 1] : angles[0] + 2 * kPi;
                alt += next - angles[i];
            }
            chk.kawasaki = std::fabs(alt - kPi) < degToRad(0.5);
        }
        out.push_back(chk);
    }
    return out;
}

// ---------------------------------------------------------------- snapping --
Vec2 Studio::snapCreasePoint(const Vec2& screen, bool* snapped) const {
    const Mat3 inv = canvasView.inverse();
    const Vec2 m = inv.apply(screen);
    const double k = std::sqrt(std::fabs(canvasView.determinant()));
    const double radius = 10.0 / std::max(1e-9, k);            // 10 pixels in pattern units
    Vec2 p = m;
    if (snapVertices) p = doc.cp.snap(m, radius, 0.0);
    bool hit = !nearlyEqual(p, m, 1e-12);
    if (!hit && snapGrid && doc.cp.gridStep > 0) {
        const Vec2 g = doc.cp.snap(m, 0.0, doc.cp.gridStep);
        if (distance(g, m) <= radius) { p = g; hit = true; }
    }
    if (snapped) *snapped = hit;
    return p;
}

// ------------------------------------------------------------------ the tab --
void Studio::drawCreaseTab(const Rect& content) {
    Image& img = ui_.img();
    const Palette pal = palette();
    CreasePattern& cp = doc.cp;
    const UiInput& in = ui_.input();
    const Vec2 mouse{double(in.mouseX), double(in.mouseY)};

    // ---- tools (left) --------------------------------------------------------
    const Rect tools = ui_.panel(Rect{content.x, content.y, 210, content.h}, "TOOLS");
    int y = tools.y + 8;
    const int bw = tools.w - 16;
    ui_.label(tools.x + 8, y, "TOOL", ui::Muted); y += 12;
    const struct { const char* l; CreaseTool t; } toolList[] = {{"DRAW (D)", CreaseTool::Draw}, {"SELECT (S)", CreaseTool::Select}, {"ERASE (E)", CreaseTool::Erase}};
    for (const auto& t : toolList) {
        if (ui_.button(Rect{tools.x + 8, y, bw, 24}, t.l, tool == t.t)) { tool = t.t; ui_.cancelDrag(); }
        y += 28;
    }
    y += 6;
    ui_.label(tools.x + 8, y, "CREASE TYPE", ui::Muted); y += 12;
    const struct { const char* l; CreaseType t; Color c; } types[] = {
        {"MOUNTAIN (M)", CreaseType::Mountain, pal.mountain}, {"VALLEY (V)", CreaseType::Valley, pal.valley}, {"AUXILIARY (A)", CreaseType::Aux, ui::Muted}};
    for (const auto& t : types) {
        if (ui_.button(Rect{tools.x + 8, y, bw, 24}, t.l, drawType == t.t)) {
            drawType = t.t;
            if (tool == CreaseTool::Erase) tool = CreaseTool::Draw;
        }
        img.fillRect(tools.x + 12, y + 8, 8, 8, t.c);
        y += 28;
    }
    y += 6;
    ui_.label(tools.x + 8, y, "OPTIONS", ui::Muted); y += 12;
    ui_.toggle(Rect{tools.x + 8, y, bw, 18}, "SNAP TO VERTICES", snapVertices); y += 22;
    ui_.toggle(Rect{tools.x + 8, y, bw, 18}, "SNAP TO GRID (G)", snapGrid); y += 22;
    ui_.toggle(Rect{tools.x + 8, y, bw, 18}, "SHOW GRID", showGrid); y += 22;
    ui_.toggle(Rect{tools.x + 8, y, bw, 18}, "FULL-LENGTH LINES (L)", fullLines); y += 22;
    ui_.toggle(Rect{tools.x + 8, y, bw, 18}, "2-COLOUR FACES", showFaces); y += 22;
    ui_.toggle(Rect{tools.x + 8, y, bw, 18}, "FLAT-FOLD CHECKS", showChecks); y += 28;

    const int divisions = cp.gridStep > 0 ? int(std::lround(paperSize(cp) / cp.gridStep)) : 8;
    ui_.label(tools.x + 8, y + 7, "GRID 1/" + std::to_string(divisions));
    auto setDivisions = [&](int d) {
        d = std::clamp(d, 2, 32);
        if (d == divisions) return;
        checkpoint();
        cp.gridStep = paperSize(cp) / d;
        setStatus("GRID: " + std::to_string(d) + " DIVISIONS");
    };
    if (ui_.button(Rect{tools.right() - 66, y, 28, 22}, "-")) setDivisions(divisions / 2);
    if (ui_.button(Rect{tools.right() - 34, y, 28, 22}, "+")) setDivisions(divisions * 2);
    y += 32;
    ui_.label(tools.x + 8, y, "PAPER", ui::Muted); y += 12;
    const char* papers[] = {"SQUARE", "DIAMOND", "RECT"};
    const int pw = (bw - 8) / 3;
    for (int i = 0; i < 3; ++i)
        if (ui_.button(Rect{tools.x + 8 + i * (pw + 4), y, pw, 24}, papers[i])) {
            checkpoint();
            setPaper(i);
            setStatus(std::string("PAPER: ") + papers[i] + " (CREASES TRIMMED TO IT)");
        }
    y += 32;
    if (ui_.button(Rect{tools.x + 8, y, bw, 24}, "CLEAR ALL CREASES", false, !cp.creases.empty())) {
        checkpoint();
        cp.creases.clear();
        selectedCrease = -1;
        setStatus("CLEARED - CTRL+Z BRINGS THEM BACK");
    }

    // ---- canvas (centre) ---------------------------------------------------------
    const Rect canvasIn = ui_.panel(canvasRect, "CREASE PATTERN EDITOR - " + upper(fit(cp.name, 40)), 1);
    const Rect area = inset(canvasIn, 4);
    canvasView = fitView(cp.paper.bounds(), Rect{area.x, area.y, area.w, area.h - 18}, 30);
    const double k = std::sqrt(std::fabs(canvasView.determinant()));

    if (showFaces) {
        fillPolygon(img, cp.paper.transformed(canvasView).v, pal.paperBack);
        fillCreaseFaces(img, cp, canvasView, pal.paperFront, pal.paperBack);
    }
    CreaseViewOptions o;
    o.fillPaper = !showFaces;
    o.showGrid = showGrid;
    o.gridStep = cp.gridStep;
    o.highlight = selectedCrease;
    drawCreasePattern(img, cp, canvasView, pal, o);
    std::vector<VertexCheck> checks;
    if (showChecks) {
        checks = flatFoldabilityChecks();
        for (const VertexCheck& c : checks) {
            const Vec2 s = canvasView.apply(c.p);
            drawRing(img, int(std::lround(s.x)), int(std::lround(s.y)), 7, 2, c.maekawa && c.kawasaki ? Color::hex(0x2A9D8F) : pal.mountain);
        }
    }
    drawCreaseLegend(img, area.x + 8, area.bottom() - 10, pal);

    // interaction
    const bool inCanvas = ui_.hover(area);
    const int hoverCrease = inCanvas ? cp.creaseAt(canvasView.inverse().apply(mouse), 8.0 / std::max(1e-9, k)) : -1;
    if (tool != CreaseTool::Draw && hoverCrease >= 0 && !ui_.anyDrag()) {
        const Crease& c = cp.creases[std::size_t(hoverCrease)];
        drawStroke(img, canvasView.apply(c.a), canvasView.apply(c.b),
                   Stroke::solid((tool == CreaseTool::Erase ? pal.mountain : ui::Accent).withAlpha(140), 6));
    }
    if (inCanvas && in.rightPressed && hoverCrease >= 0) {      // right-click: toggle M/V with any tool
        checkpoint();
        cp.toggle(std::size_t(hoverCrease));
        selectedCrease = hoverCrease;
        setStatus(std::string("CREASE IS NOW ") + typeName(cp.creases[std::size_t(hoverCrease)].type));
    }
    if (ui_.pressAvailable() && inCanvas) {
        ui_.usePress();
        if (tool == CreaseTool::Draw) {
            dragStart_ = snapCreasePoint(mouse, &dragSnapped_);
            dragEnd_ = dragStart_;
            ui_.startDrag("crease");
        } else if (tool == CreaseTool::Select) {
            selectedCrease = hoverCrease;
        } else if (hoverCrease >= 0) {
            checkpoint();
            cp.removeCrease(std::size_t(hoverCrease));
            selectedCrease = -1;
            setStatus("ERASED A CREASE");
        }
    }
    const bool active = ui_.dragging("crease");
    const bool ended = !active && ui_.dragEnded("crease");
    if (active || ended) {
        bool sn = false;
        dragEnd_ = snapCreasePoint(mouse, &sn);
        if (in.shift) dragEnd_ = constrainAngle(dragStart_, dragEnd_);
        const Vec2 a = canvasView.apply(dragStart_), b = canvasView.apply(dragEnd_);
        if (ended) {
            if (distance(a, b) >= 5.0) addCreaseFromEditor(dragStart_, dragEnd_);
        } else {
            Vec2 pa = a, pb = b;
            if (fullLines) {
                Vec2 ca, cb;
                if (clipLineToConvex(dragStart_, dragEnd_, cp.paper.v, ca, cb)) { pa = canvasView.apply(ca); pb = canvasView.apply(cb); }
            }
            drawCrease(img, pa, pb, drawType, pal, 2);
            for (const Vec2& p : {a, b}) drawRing(img, int(std::lround(p.x)), int(std::lround(p.y)), 5, 2, ui::Accent);
            char buf[64];
            std::snprintf(buf, sizeof buf, "%.1F  %.1F DEG%s", distance(dragStart_, dragEnd_),
                          std::fmod(radToDeg(-(dragEnd_ - dragStart_).angle()) + 360.0, 180.0), sn ? "  SNAPPED" : "");
            ui_.label(int(b.x) + 10, int(b.y) + 8, buf);
        }
    } else if (tool == CreaseTool::Draw && inCanvas) {
        bool sn = false;
        const Vec2 p = canvasView.apply(snapCreasePoint(mouse, &sn));
        if (sn) drawRing(img, int(std::lround(p.x)), int(std::lround(p.y)), 6, 2, ui::Accent);
    }

    // ---- info (right) ---------------------------------------------------------------
    const Rect info = ui_.panel(Rect{canvasRect.right() + 8, content.y, content.right() - canvasRect.right() - 8, content.h}, "PATTERN");
    int iy = info.y + 10;
    auto line = [&](const std::string& s, Color c = ui::Text) { ui_.label(info.x + 10, iy, fit(s, info.w - 20), c); iy += 13; };
    const BBox b = cp.paper.bounds();
    char buf[96];
    std::snprintf(buf, sizeof buf, "PAPER %.0F X %.0F, %d CORNERS", b.width(), b.height(), int(cp.paper.size()));
    line(buf);
    line(std::to_string(cp.creases.size()) + " CREASES: " + std::to_string(cp.count(CreaseType::Mountain)) + " M, " +
         std::to_string(cp.count(CreaseType::Valley)) + " V, " + std::to_string(cp.count(CreaseType::Aux)) + " AUX");
    line(std::to_string(cp.vertices().size()) + " VERTICES, " + std::to_string(cp.intersections().size()) + " CROSSINGS");
    line(cpPath.empty() ? "NOT SAVED YET" : "FILE: " + upper(cpPath), ui::Muted);
    iy += 10;

    ui_.label(info.x + 10, iy, "SELECTED CREASE", ui::Accent); iy += 14;
    const bool has = selectedCrease >= 0 && selectedCrease < int(cp.creases.size());
    if (has) {
        const Crease& c = cp.creases[std::size_t(selectedCrease)];
        line(typeName(c.type));
        std::snprintf(buf, sizeof buf, "LENGTH %.1F, ANGLE %.1F DEG", c.segment().length(),
                      std::fmod(radToDeg(-(c.b - c.a).angle()) + 360.0, 180.0));
        line(buf);
        std::snprintf(buf, sizeof buf, "(%.1F, %.1F) - (%.1F, %.1F)", c.a.x, c.a.y, c.b.x, c.b.y);
        line(buf, ui::Muted);
    } else {
        line(tool == CreaseTool::Select ? "CLICK A CREASE" : "USE SELECT (S) TO PICK ONE", ui::Muted);
        iy += 26;
    }
    const int hw = (info.w - 26) / 2;
    if (ui_.button(Rect{info.x + 10, iy, hw, 24}, "TOGGLE M/V (T)", false, has && cp.creases[std::size_t(selectedCrease)].type != CreaseType::Aux)) {
        checkpoint();
        cp.toggle(std::size_t(selectedCrease));
    }
    if (ui_.button(Rect{info.x + 16 + hw, iy, hw, 24}, "DELETE (DEL)", false, has)) {
        checkpoint();
        cp.removeCrease(std::size_t(selectedCrease));
        selectedCrease = -1;
    }
    iy += 38;

    ui_.label(info.x + 10, iy, "FLAT-FOLDABILITY", ui::Accent); iy += 14;
    int mOk = 0, kOk = 0;
    for (const VertexCheck& c : (showChecks ? checks : flatFoldabilityChecks())) { mOk += c.maekawa; kOk += c.kawasaki; }
    const int nv = int(showChecks ? checks.size() : flatFoldabilityChecks().size());
    line(std::to_string(nv) + " INTERIOR VERTICES");
    line("MAEKAWA |M-V|=2:     " + std::to_string(mOk) + "/" + std::to_string(nv), mOk == nv ? Color::hex(0x2A9D8F) : pal.mountain);
    line("KAWASAKI ALT = 180:  " + std::to_string(kOk) + "/" + std::to_string(nv), kOk == nv ? Color::hex(0x2A9D8F) : pal.mountain);
    line(nv == 0 ? "DRAW CREASES THAT MEET INSIDE" : (mOk == nv && kOk == nv ? "EVERY VERTEX CAN FOLD FLAT" : "RED RINGS: VERTICES THAT FAIL"), ui::Muted);
    line("(LOCAL TESTS - THEY DO NOT CHECK", ui::Muted);
    line(" LAYER COLLISIONS)", ui::Muted);
    iy += 10;

    ui_.label(info.x + 10, iy, "FILE", ui::Accent); iy += 14;
    if (ui_.button(Rect{info.x + 10, iy, hw, 24}, "SAVE .CP")) save();
    if (ui_.button(Rect{info.x + 16 + hw, iy, hw, 24}, "PNG + SVG")) exportCurrent();
    iy += 30;
    if (ui_.button(Rect{info.x + 10, iy, info.w - 20, 24}, "SAVE AS...")) {
        textBuffer_ = baseName();
        overlay = Overlay::SaveAs;
    }
}

// ------------------------------------------------------------------- keys --
void Studio::creaseKey(const KeyPress& k) {
    CreasePattern& cp = doc.cp;
    const bool has = selectedCrease >= 0 && selectedCrease < int(cp.creases.size());
    if (k.key == Key::Escape) { if (ui_.anyDrag()) ui_.cancelDrag(); else selectedCrease = -1; return; }
    if (k.key == Key::Delete || k.key == Key::Backspace) {
        if (has) { checkpoint(); cp.removeCrease(std::size_t(selectedCrease)); selectedCrease = -1; setStatus("DELETED THE CREASE"); }
        return;
    }
    if (k.key != Key::Char) return;
    switch (k.ch) {
        case 'd': tool = CreaseTool::Draw; break;
        case 's': tool = CreaseTool::Select; break;
        case 'e': tool = CreaseTool::Erase; break;
        case 'm': case 'v': case 'a': {
            const CreaseType t = k.ch == 'm' ? CreaseType::Mountain : k.ch == 'v' ? CreaseType::Valley : CreaseType::Aux;
            drawType = t;
            if (has && tool == CreaseTool::Select && cp.creases[std::size_t(selectedCrease)].type != t) {
                checkpoint();
                cp.creases[std::size_t(selectedCrease)].type = t;
            }
            setStatus(std::string("CREASE TYPE: ") + typeName(t));
            break;
        }
        case 't':
            if (has) { checkpoint(); cp.toggle(std::size_t(selectedCrease)); }
            break;
        case 'l': fullLines = !fullLines; setStatus(fullLines ? "FULL-LENGTH LINES ON" : "FULL-LENGTH LINES OFF"); break;
        case 'g': snapGrid = !snapGrid; setStatus(snapGrid ? "GRID SNAP ON" : "GRID SNAP OFF"); break;
        case '+': case '=': case '-': {
            const int d = cp.gridStep > 0 ? int(std::lround(paperSize(cp) / cp.gridStep)) : 8;
            const int nd = std::clamp(k.ch == '-' ? d / 2 : d * 2, 2, 32);
            if (nd != d) { checkpoint(); cp.gridStep = paperSize(cp) / nd; setStatus("GRID: " + std::to_string(nd) + " DIVISIONS"); }
            break;
        }
        default: break;
    }
}

} // namespace ori
