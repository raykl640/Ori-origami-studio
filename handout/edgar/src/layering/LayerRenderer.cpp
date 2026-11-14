// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : layering        Owner : Edgar Kareithi (Layering, panels & studio UI)
//  File   : LayerRenderer.cpp
// ============================================================================
#include "LayerRenderer.h"
#include "../core/Font.h"
#include "../curves/Curves.h"
#include "../editor/CreaseRenderer.h"
#include "../raster/Clipping.h"
#include "../raster/Fill.h"
#include "../raster/Lines.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace ori {

namespace {

std::vector<Vec2> transformAll(const Mat3& m, const std::vector<Vec2>& pts) {
    std::vector<Vec2> out;
    out.reserve(pts.size());
    for (const Vec2& p : pts) out.push_back(m.apply(p));
    return out;
}

int maxLayerOf(const std::vector<FoldPanel>& panels) {
    int m = 0;
    for (const FoldPanel& p : panels) m = std::max(m, p.layer);
    return m;
}

// Union silhouette of all panels as a soft drop shadow (each pixel blended once).
void drawShadow(Image& img, const std::vector<std::vector<Vec2>>& outlines, Vec2 offset, double alpha) {
    std::vector<char> mask(std::size_t(img.width()) * img.height(), 0);
    for (const auto& o : outlines) {
        std::vector<Vec2> moved;
        for (const Vec2& p : o) moved.push_back(p + offset);
        scanlineSpans({moved}, [&](int y, int x0, int x1) {
            if (y < 0 || y >= img.height()) return;
            for (int x = std::max(0, x0); x < std::min(img.width(), x1); ++x) mask[std::size_t(y) * img.width() + x] = 1;
        }, img.height());
    }
    const Color shadow(60, 45, 35);
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (mask[std::size_t(y) * img.width() + x]) img.blendPixel(x, y, shadow, alpha);
}

} // namespace

BBox panelBounds(const std::vector<FoldPanel>& panels) {
    const double inf = std::numeric_limits<double>::infinity();
    BBox b{{inf, inf}, {-inf, -inf}};
    for (const FoldPanel& p : panels)
        for (const Vec2& v : p.outline) {
            b.min.x = std::min(b.min.x, v.x); b.min.y = std::min(b.min.y, v.y);
            b.max.x = std::max(b.max.x, v.x); b.max.y = std::max(b.max.y, v.y);
        }
    if (panels.empty()) b = BBox{{0, 0}, {1, 1}};
    return b;
}

Color panelColor(const FoldPanel& p, int rankFromTop, int layerCount, const Palette& pal, const RenderOptions& opt) {
    const bool coloured = p.faceUp != opt.whiteSideUp;
    const Color base = coloured ? pal.paperFront : pal.paperBack;
    return opt.depthShading ? depthShade(base, rankFromTop, layerCount, opt.depthPerLevel) : base;
}

void drawPanels(Image& img, const std::vector<FoldPanel>& bottomToTop, const Mat3& view,
                const Palette& pal, const RenderOptions& opt) {
    if (bottomToTop.empty()) return;
    const int top = maxLayerOf(bottomToTop);
    const int layers = top + 1;

    // Screen-space outlines and the model's screen bounds (for the light gradient).
    std::vector<std::vector<Vec2>> outlines;
    for (const FoldPanel& p : bottomToTop) outlines.push_back(transformAll(view, p.outline));
    const double inf = std::numeric_limits<double>::infinity();
    BBox sb{{inf, inf}, {-inf, -inf}};
    for (const auto& o : outlines)
        for (const Vec2& v : o) {
            sb.min.x = std::min(sb.min.x, v.x); sb.min.y = std::min(sb.min.y, v.y);
            sb.max.x = std::max(sb.max.x, v.x); sb.max.y = std::max(sb.max.y, v.y);
        }

    if (opt.shadow && opt.alpha >= 1.0) drawShadow(img, outlines, {4, 5}, 0.18);

    // Painter's algorithm: draw order = stacking order (or reversed, to show the bug).
    std::vector<std::size_t> order(bottomToTop.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
    if (opt.reverseOrder) std::reverse(order.begin(), order.end());
    const std::size_t limit = opt.maxPanels >= 0 ? std::min(order.size(), std::size_t(opt.maxPanels)) : order.size();

    for (std::size_t k = 0; k < limit; ++k) {
        const std::size_t i = order[k];
        const FoldPanel& p = bottomToTop[i];
        const Color c = panelColor(p, top - p.layer, layers, pal, opt);
        const auto& o = outlines[i];
        if (opt.alpha < 1.0) {
            fillPolygonAlpha(img, o, c, opt.alpha);
        } else if (opt.lighting) {
            fillPolygonShaded(img, {o}, [&](int x, int y) {
                return scale(c, lightingFactor({x + 0.5, y + 0.5}, sb, opt.lightDir));
            });
        } else {
            fillPolygon(img, o, c);
        }
        if (opt.outlines) drawPolyline(img, o, Stroke::smooth(pal.outline.withAlpha(opt.alpha < 1.0 ? 140 : 200)), true);
    }
}

void drawFoldState(Image& img, const FoldState& s, const Mat3& view, const Palette& pal, const RenderOptions& opt) {
    drawPanels(img, s.bottomToTop(), view, pal, opt);
}

Mat3 drawFoldStateFitted(Image& img, const FoldState& s, const Rect& r, const Palette& pal,
                         const RenderOptions& opt, int margin) {
    const Mat3 view = fitView(panelBounds(s.panels), r, margin);
    drawFoldState(img, s, view, pal, opt);
    return view;
}

// ---------------------------------------------------------- exploded view --
void drawExplodedView(Image& img, const FoldState& s, const Rect& r, const Palette& pal, double lift,
                      const RenderOptions& opt) {
    const auto panels = s.bottomToTop();
    if (panels.empty()) return;
    const int layers = s.layerCount();
    // Isometric tilt: rotate 45 degrees, then squash y by half.
    const Mat3 iso = Mat3::scale(1.0, 0.5) * Mat3::rotate(degToRad(45.0));
    // Fit the tilted model plus the lift of the top layer into the rectangle.
    const double inf = std::numeric_limits<double>::infinity();
    BBox b{{inf, inf}, {-inf, -inf}};
    for (const FoldPanel& p : panels)
        for (const Vec2& v : p.outline) {
            const Vec2 q = iso.apply(v);
            b.min.x = std::min(b.min.x, q.x); b.min.y = std::min(b.min.y, q.y);
            b.max.x = std::max(b.max.x, q.x); b.max.y = std::max(b.max.y, q.y);
        }
    const Rect area{r.x, int(r.y + lift * (layers - 1)), r.w - 40, int(r.h - lift * (layers - 1))};
    const Mat3 view = fitView(b, area, 12) * iso;

    RenderOptions o = opt;
    o.shadow = false;
    o.lighting = false;
    int lastLabelY = std::numeric_limits<int>::max();
    for (const FoldPanel& p : panels) {
        const Mat3 lifted = Mat3::translate(0, -lift * p.layer) * view;
        // Shade by depth using the full stack's layer count.
        const Color c = panelColor(p, layers - 1 - p.layer, layers, pal, o);
        const auto out = [&] { std::vector<Vec2> v; for (const Vec2& q : p.outline) v.push_back(lifted.apply(q)); return v; }();
        fillPolygonAlpha(img, out, c, 0.92);
        drawPolyline(img, out, Stroke::smooth(pal.outline), true);
        // Label at the panel's right-most point, nudged so labels do not overlap.
        Vec2 right = out[0];
        for (const Vec2& q : out) if (q.x > right.x) right = q;
        int ly = int(right.y) - 3;
        if (ly > lastLabelY - 10) ly = lastLabelY - 10;
        lastLabelY = ly;
        drawText(img, int(right.x) + 6, ly, "L" + std::to_string(p.layer), theme::Caption);
    }
}

// ---------------------------------------------------------- layer inspector --
void drawLayerThumbnail(Image& img, const FoldState& s, int layer, const Rect& r, const Palette& pal,
                        const RenderOptions& opt) {
    const Mat3 view = fitView(panelBounds(s.panels), r, 8);
    // Ghost of the whole stack.
    for (const FoldPanel& p : s.bottomToTop()) {
        std::vector<Vec2> o;
        for (const Vec2& v : p.outline) o.push_back(view.apply(v));
        fillPolygonAlpha(img, o, Color(200, 195, 188), 0.18);
        drawPolyline(img, o, Stroke::solid(Color(205, 200, 192)), true);
    }
    // The chosen layer in its true colour (no depth shading - it is on its own).
    RenderOptions o = opt;
    o.depthShading = false;
    o.shadow = false;
    o.lighting = false;
    for (const FoldPanel& p : s.panels) {
        if (p.layer != layer) continue;
        std::vector<Vec2> out;
        for (const Vec2& v : p.outline) out.push_back(view.apply(v));
        fillPolygon(img, out, panelColor(p, 0, 1, pal, o));
        drawPolyline(img, out, Stroke::smooth(pal.outline), true);
    }
}

int drawLayerInspector(Image& img, const FoldState& s, const Rect& r, const Palette& pal, int cols,
                       const RenderOptions& opt) {
    const int layers = s.layerCount();
    if (layers == 0 || cols <= 0) return 0;
    const int rows = (layers + cols - 1) / cols;
    const int gap = 6;
    const int cw = (r.w - gap * (cols - 1)) / cols, ch = (r.h - gap * (rows - 1)) / rows;
    for (int i = 0; i < layers; ++i) {
        const int layer = layers - 1 - i;          // top layer first
        const Rect cell{r.x + (i % cols) * (cw + gap), r.y + (i / cols) * (ch + gap), cw, ch};
        img.fillRect(cell.x, cell.y, cell.w, cell.h, Color(250, 248, 244));
        img.strokeRect(cell.x, cell.y, cell.w, cell.h, theme::Border);
        drawText(img, cell.x + 5, cell.y + 5, "LAYER " + std::to_string(layer), theme::Title);
        drawLayerThumbnail(img, s, layer, Rect{cell.x + 4, cell.y + 16, cell.w - 8, cell.h - 20}, pal, opt);
    }
    return layers;
}

// ------------------------------------------------------------ fold diagram --
Vec2 movingCentroid(const FoldState& s, const FoldStep& step) {
    double total = 0;
    Vec2 acc;
    for (const FoldPanel& p : s.panels) {
        const auto piece = FoldEngine::clipHalfPlane(p.outline, step.a, step.b, +1);
        if (piece.size() < 3) continue;
        const Polygon poly(piece);
        const double a = poly.area();
        if (a < 1e-9) continue;
        acc += poly.centroid() * a;
        total += a;
    }
    return total > 0 ? acc / total : (step.a + step.b) * 0.5;
}

void drawFoldDiagram(Image& img, const FoldState& s, const FoldStep& next, const Mat3& view, const Palette& pal) {
    // Crease: the fold line across the convex hull of the model (Cyrus-Beck).
    std::vector<Vec2> pts;
    for (const FoldPanel& p : s.panels) pts.insert(pts.end(), p.outline.begin(), p.outline.end());
    const Polygon hull = convexHull(pts);
    Vec2 p0, p1;
    if (hull.size() >= 3 && clipLineToConvex(next.a, next.b, hull.v, p0, p1)) {
        const Vec2 d = (p1 - p0).normalized() * 6.0;   // overshoot a little, like printed diagrams
        drawCrease(img, view.apply(p0) - d, view.apply(p1) + d,
                   next.kind == FoldKind::Valley ? CreaseType::Valley : CreaseType::Mountain, pal, 2);
    }
    // Arrow: from the flap's centre to its mirror image.
    const Vec2 from = movingCentroid(s, next);
    const Vec2 to = Mat3::reflectLine(next.a, next.b).apply(from);
    if (distance(from, to) < 1e-6) return;
    const Vec2 f = view.apply(from), t = view.apply(to);
    const Vec2 shrink = (t - f) * 0.08;
    drawFoldArrow(img, f + shrink, t - shrink, 0.32, Color::hex(0x2B2B33),
                  next.kind == FoldKind::Valley ? ArrowHead::Filled : ArrowHead::Hollow, 2);
}

} // namespace ori
