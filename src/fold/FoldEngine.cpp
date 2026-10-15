// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : fold            Owner : Mohamed Bile (Fold transformation engine)
//  File   : FoldEngine.cpp
// ============================================================================
#include "FoldEngine.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace ori {

namespace {

constexpr double kSideEps = 1e-7;   // points closer than this to the fold line are "on" it
constexpr double kAreaEps = 1e-6;   // slivers smaller than this are discarded

std::vector<Vec2> transformAll(const Mat3& m, const std::vector<Vec2>& pts) {
    std::vector<Vec2> out;
    out.reserve(pts.size());
    for (const Vec2& p : pts) out.push_back(m.apply(p));
    return out;
}

// Remove consecutive duplicate vertices left behind by clipping.
std::vector<Vec2> dedupe(const std::vector<Vec2>& poly) {
    std::vector<Vec2> out;
    for (const Vec2& p : poly)
        if (out.empty() || !nearlyEqual(out.back(), p, 1e-9)) out.push_back(p);
    while (out.size() > 1 && nearlyEqual(out.front(), out.back(), 1e-9)) out.pop_back();
    return out;
}

// Renumber layers 0..n-1, keeping their relative order (ties broken by id so
// the result is deterministic).
void normaliseLayers(std::vector<FoldPanel>& panels) {
    std::vector<std::size_t> order(panels.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](std::size_t i, std::size_t j) {
        if (panels[i].layer != panels[j].layer) return panels[i].layer < panels[j].layer;
        return panels[i].id < panels[j].id;
    });
    for (std::size_t r = 0; r < order.size(); ++r) panels[order[r]].layer = static_cast<int>(r);
}

std::vector<FoldPanel> sortedBottomToTop(std::vector<FoldPanel> panels) {
    std::stable_sort(panels.begin(), panels.end(),
                     [](const FoldPanel& p, const FoldPanel& q) { return p.layer < q.layer; });
    return panels;
}

// Split one panel along the fold line. Pieces thinner than kAreaEps are dropped.
void splitPanel(const FoldPanel& panel, const FoldStep& step, std::vector<Vec2>& stay, std::vector<Vec2>& move) {
    stay = dedupe(FoldEngine::clipHalfPlane(panel.outline, step.a, step.b, -1));
    move = dedupe(FoldEngine::clipHalfPlane(panel.outline, step.a, step.b, +1));
    if (std::fabs(FoldEngine::signedArea(stay)) < kAreaEps) stay.clear();
    if (std::fabs(FoldEngine::signedArea(move)) < kAreaEps) move.clear();
}

// The part of the fold line that crosses `piece`: its vertices that lie on the
// line, reduced to the two extremes along the line direction.
bool creaseSegment(const std::vector<Vec2>& piece, const FoldStep& step, Vec2& p0, Vec2& p1) {
    const Vec2 dir = (step.b - step.a).normalized();
    double lo = std::numeric_limits<double>::max(), hi = -lo;
    for (const Vec2& v : piece) {
        if (std::fabs(FoldEngine::side(step.a, step.b, v)) > kSideEps * 10) continue;
        const double s = dot(v - step.a, dir);
        if (s < lo) { lo = s; p0 = v; }
        if (s > hi) { hi = s; p1 = v; }
    }
    return hi - lo > 1e-6;
}

} // namespace

// ------------------------------------------------------------------ FoldPanel --
std::vector<Vec2> FoldPanel::sheetOutline() const {
    return transformAll(sheetToFolded.inverse(), outline);
}

// ------------------------------------------------------------------ FoldState --
std::vector<FoldPanel> FoldState::bottomToTop() const { return sortedBottomToTop(panels); }

int FoldState::layerCount() const {
    int n = 0;
    for (const FoldPanel& p : panels) n = std::max(n, p.layer + 1);
    return n;
}

// ----------------------------------------------------------------- FoldEngine --
double FoldEngine::side(const Vec2& a, const Vec2& b, const Vec2& p) {
    const Vec2 d = b - a;
    const double len = d.length();
    if (len < kEps) return 0.0;
    return cross(d, p - a) / len;
}

std::vector<Vec2> FoldEngine::clipHalfPlane(const std::vector<Vec2>& poly, const Vec2& a, const Vec2& b, int sign) {
    std::vector<Vec2> out;
    if (poly.size() < 3) return out;
    auto inside = [&](double s) { return s * sign >= -kSideEps; };
    for (std::size_t i = 0; i < poly.size(); ++i) {
        const Vec2& cur = poly[i];
        const Vec2& nxt = poly[(i + 1) % poly.size()];
        const double sc = side(a, b, cur), sn = side(a, b, nxt);
        const bool inC = inside(sc), inN = inside(sn);
        // The four Sutherland-Hodgman cases for the edge cur -> nxt.
        if (inC && inN) {
            out.push_back(nxt);                                  // in -> in : keep nxt
        } else if (inC && !inN) {
            out.push_back(lerp(cur, nxt, sc / (sc - sn)));       // in -> out: keep the crossing
        } else if (!inC && inN) {
            out.push_back(lerp(cur, nxt, sc / (sc - sn)));       // out -> in: crossing, then nxt
            out.push_back(nxt);
        }                                                        // out -> out: nothing
    }
    return out;
}

double FoldEngine::signedArea(const std::vector<Vec2>& poly) {
    double a = 0.0;
    for (std::size_t i = 0; i < poly.size(); ++i) a += cross(poly[i], poly[(i + 1) % poly.size()]);
    return 0.5 * a;
}

double FoldEngine::paperArea(const FoldState& state) {
    double total = 0.0;
    for (const FoldPanel& p : state.panels) total += std::fabs(signedArea(p.outline));
    return total;
}

FoldState FoldEngine::flat(const std::vector<Vec2>& sheet) {
    FoldState s;
    s.sheet = sheet;
    FoldPanel p;
    p.id = 0;
    p.outline = sheet;
    s.panels.push_back(p);
    return s;
}

FoldState FoldEngine::fold(const FoldState& state, const FoldStep& step) {
    FoldState next;
    next.sheet = state.sheet;
    next.creases = state.creases;
    next.stepsApplied = state.stepsApplied + 1;
    if ((step.b - step.a).length() < kEps) {   // no fold line: nothing to do
        next.panels = state.panels;
        return next;
    }

    const Mat3 reflect = Mat3::reflectLine(step.a, step.b);
    int minLayer = std::numeric_limits<int>::max(), maxLayer = std::numeric_limits<int>::min();
    for (const FoldPanel& p : state.panels) {
        minLayer = std::min(minLayer, p.layer);
        maxLayer = std::max(maxLayer, p.layer);
    }

    for (const FoldPanel& panel : state.panels) {
        std::vector<Vec2> stay, move;
        splitPanel(panel, step, stay, move);

        if (!stay.empty()) {
            FoldPanel kept = panel;
            kept.outline = stay;
            next.panels.push_back(kept);
        }
        if (!move.empty()) {
            FoldPanel moved = panel;
            moved.outline = transformAll(reflect, move);
            moved.sheetToFolded = reflect * panel.sheetToFolded;
            moved.faceUp = !panel.faceUp;
            // Folding reverses the order of the moving layers: the bottom one
            // ends up outermost. Valley -> on top of the stack, mountain -> under it.
            moved.layer = step.kind == FoldKind::Valley ? maxLayer + 1 + (maxLayer - panel.layer)
                                                        : minLayer - 1 - (panel.layer - minLayer);
            next.panels.push_back(moved);
        }

        // A crease exists only where paper actually bends: the panel was cut
        // into two real pieces.
        Vec2 c0, c1;
        if (!stay.empty() && !move.empty() && creaseSegment(move, step, c0, c1)) {
            const Mat3 toSheet = panel.sheetToFolded.inverse();
            FoldCrease crease;
            crease.a = toSheet.apply(c0);
            crease.b = toSheet.apply(c1);
            // The fold is seen from the viewer's side. If this panel shows its
            // back, the same crease reads the opposite way on the front.
            crease.kind = panel.faceUp ? step.kind : opposite(step.kind);
            crease.step = state.stepsApplied;
            next.creases.push_back(crease);
        }
    }

    normaliseLayers(next.panels);
    return next;
}

std::vector<FoldState> FoldEngine::run(const std::vector<Vec2>& sheet, const std::vector<FoldStep>& steps) {
    std::vector<FoldState> states{flat(sheet)};
    for (const FoldStep& step : steps) states.push_back(fold(states.back(), step));
    return states;
}

std::vector<FoldPanel> FoldEngine::inProgress(const FoldState& state, const FoldStep& step, double t) {
    t = std::clamp(t, 0.0, 1.0);
    const Mat3 m = Mat3::partialFold(step.a, step.b, t);
    int minLayer = 0, maxLayer = 0;
    for (const FoldPanel& p : state.panels) {
        minLayer = std::min(minLayer, p.layer);
        maxLayer = std::max(maxLayer, p.layer);
    }

    std::vector<FoldPanel> out;
    for (const FoldPanel& panel : state.panels) {
        std::vector<Vec2> stay, move;
        splitPanel(panel, step, stay, move);
        if (!stay.empty()) {
            FoldPanel kept = panel;
            kept.outline = stay;
            out.push_back(kept);
        }
        if (!move.empty()) {
            FoldPanel moving = panel;
            moving.outline = transformAll(m, move);
            moving.sheetToFolded = m * panel.sheetToFolded;   // singular at t = 0.5 (edge-on)
            if (t > 0.5) moving.faceUp = !panel.faceUp;
            // While swinging, a valley flap is in front of the stack and a
            // mountain flap is behind it.
            moving.layer = step.kind == FoldKind::Valley ? maxLayer + 1 + (maxLayer - panel.layer)
                                                         : minLayer - 1 - (panel.layer - minLayer);
            out.push_back(moving);
        }
    }
    normaliseLayers(out);
    return sortedBottomToTop(out);
}

} // namespace ori
