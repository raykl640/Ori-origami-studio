// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demo 06 : Curves - Bezier and splines    Owner : Matthew Wachira
//  de Casteljau construction, Bernstein basis, adaptive subdivision,
//  Catmull-Rom vs B-spline, and the project uses: fold arrows and the
//  easing curve that times every fold animation.
// ============================================================================
#include "Demos.h"
#include "../src/curves/Curves.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/layering/LayerRenderer.h"
#include "../src/models/Models.h"
#include "../src/raster/Circle.h"
#include "../src/raster/Fill.h"
#include "../src/raster/Lines.h"
#include <cmath>
#include <cstdio>

namespace ori {

std::vector<std::string> demoCurves(const DemoContext& ctx) {
    int top = 0;
    Image img = makeSheet("CURVES: BEZIER AND SPLINES", "DE CASTELJAU, BERNSTEIN, ADAPTIVE SUBDIVISION, CATMULL-ROM, B-SPLINE - FOLD ARROWS AND ANIMATION TIMING", top);
    const Palette pal = Palette::classic();
    const Color ink = Color::hex(0x2B2B33);
    const Color lv[] = {Color(150, 150, 150), Color::hex(0x2A9D8F), Color::hex(0x1F6FD1), Color::hex(0xE4572E)};

    // ---- 1: de Casteljau ---------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(0, 3, 2, img.width(), img.height(), top), "DE CASTELJAU CONSTRUCTION AT T = 0.4");
        const std::vector<Vec2> ctrl = {{in.x + 30.0, in.y + 230.0}, {in.x + 80.0, in.y + 30.0},
                                        {in.x + 290.0, in.y + 20.0}, {in.x + 340.0, in.y + 220.0}};
        drawCurve(img, bezierCurve(ctrl, 80), Stroke::solid(ink, 3));
        const auto levels = deCasteljauLevels(ctrl, 0.4);
        for (std::size_t L = 0; L < levels.size(); ++L) {
            if (levels[L].size() > 1) drawPolyline(img, levels[L], Stroke::solid(lv[L], L == 0 ? 1 : 2));
            for (const Vec2& p : levels[L]) fillCircle(img, int(p.x), int(p.y), L + 1 == levels.size() ? 6 : 4, lv[L]);
        }
        for (std::size_t i = 0; i < ctrl.size(); ++i)
            drawText(img, int(ctrl[i].x) + 8, int(ctrl[i].y) - 4, "P" + std::to_string(i), ink);
        drawCaption(img, in.x + 12, in.bottom() - 34, "REPEATED LERP: 4 -> 3 -> 2 -> 1 POINTS.\nTHE LAST POINT (ORANGE) LIES ON THE CURVE");
    }

    // ---- 2: Bernstein basis ----------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(1, 3, 2, img.width(), img.height(), top), "CUBIC BERNSTEIN BASIS FUNCTIONS");
        const int gx = in.x + 40, gy = in.y + 20, gw = in.w - 70, gh = 190;
        img.fillRect(gx, gy, gw, gh, Color(250, 248, 244));
        for (int i = 0; i <= 4; ++i) {
            drawStroke(img, {double(gx), gy + gh * i / 4.0}, {double(gx + gw), gy + gh * i / 4.0}, Stroke::solid(Color(230, 225, 215)));
            drawStroke(img, {gx + gw * i / 4.0, double(gy)}, {gx + gw * i / 4.0, double(gy + gh)}, Stroke::solid(Color(230, 225, 215)));
        }
        const char* names[] = {"(1-T)^3", "3T(1-T)^2", "3T^2(1-T)", "T^3"};
        for (int k = 0; k < 4; ++k) {
            std::vector<Vec2> pts;
            for (int i = 0; i <= 100; ++i) {
                const double t = i / 100.0, u = 1 - t;
                const double b[] = {u * u * u, 3 * u * u * t, 3 * u * t * t, t * t * t};
                pts.push_back({gx + gw * t, gy + gh * (1 - b[k])});
            }
            drawCurve(img, pts, Stroke::solid(lv[k], 2));
            drawText(img, in.x + 40 + k * 80, gy + gh + 10, names[k], lv[k]);
        }
        // check Bernstein form against de Casteljau
        double maxErr = 0;
        const std::vector<Vec2> c = {{0, 0}, {30, 90}, {80, -40}, {100, 10}};
        for (int i = 0; i <= 50; ++i) {
            const double t = i / 50.0;
            maxErr = std::max(maxErr, distance(deCasteljau(c, t), bezierCubicBernstein(c[0], c[1], c[2], c[3], t)));
        }
        char buf[96];
        std::snprintf(buf, sizeof buf, "WEIGHTS SUM TO 1 FOR EVERY T.\nMAX |BERNSTEIN - DE CASTELJAU| = %.1e", maxErr);
        drawCaption(img, in.x + 12, in.bottom() - 34, buf);
    }

    // ---- 3: adaptive vs uniform ----------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(2, 3, 2, img.width(), img.height(), top), "UNIFORM SAMPLING VS ADAPTIVE SUBDIVISION");
        // long gentle run then a tight hook: uniform wastes points on the run
        const std::vector<Vec2> ctrl = {{in.x + 20.0, in.y + 95.0}, {in.x + 360.0, in.y + 95.0},
                                        {in.x + 360.0, in.y + 10.0}, {in.x + 250.0, in.y + 60.0}};
        const auto uni = bezierCurve(ctrl, 24);
        std::vector<Vec2> ad;
        for (const Vec2& p : bezierAdaptive(ctrl, 0.5)) ad.push_back(p + Vec2{0, 120});
        drawControlPolygon(img, ctrl, Color(170, 170, 170));
        drawCurve(img, uni, Stroke::solid(Color::hex(0x1F6FD1), 2));
        for (const Vec2& p : uni) fillCircle(img, int(p.x), int(p.y), 3, Color::hex(0x1F6FD1));
        drawCurve(img, ad, Stroke::solid(Color::hex(0xE4572E), 2));
        for (const Vec2& p : ad) fillCircle(img, int(p.x), int(p.y), 3, Color::hex(0xE4572E));
        drawText(img, in.x + 230, in.y + 92, "UNIFORM: " + std::to_string(uni.size()) + " PTS", Color::hex(0x1F6FD1));
        drawText(img, in.x + 230, in.y + 212, "ADAPTIVE: " + std::to_string(ad.size()) + " PTS", Color::hex(0xE4572E));
        drawCaption(img, in.x + 12, in.bottom() - 34, "ADAPTIVE: SPLIT AT T=0.5 UNTIL THE CONTROL POLYGON\nIS WITHIN 0.5 PX OF ITS CHORD - POINTS GO WHERE IT BENDS");
    }

    // ---- 4: Catmull-Rom vs B-spline ----------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(3, 3, 2, img.width(), img.height(), top), "SPLINES: CATMULL-ROM VS UNIFORM B-SPLINE");
        const std::vector<Vec2> pts = {{in.x + 25.0, in.y + 180.0}, {in.x + 80.0, in.y + 50.0}, {in.x + 150.0, in.y + 150.0},
                                       {in.x + 215.0, in.y + 40.0}, {in.x + 280.0, in.y + 170.0}, {in.x + 345.0, in.y + 70.0}};
        drawControlPolygon(img, pts, Color(170, 170, 170));
        drawCurve(img, catmullRom(pts, 24), Stroke::solid(Color::hex(0xE4572E), 3));
        drawCurve(img, bSplineUniform(pts, 24), Stroke::solid(Color::hex(0x1F6FD1), 3));
        drawStroke(img, {in.x + 20.0, in.y + 228.0}, {in.x + 50.0, in.y + 228.0}, Stroke::solid(Color::hex(0xE4572E), 3));
        drawText(img, in.x + 58, in.y + 225, "CATMULL-ROM: INTERPOLATES (C1)", ink);
        drawStroke(img, {in.x + 20.0, in.y + 246.0}, {in.x + 50.0, in.y + 246.0}, Stroke::solid(Color::hex(0x1F6FD1), 3));
        drawText(img, in.x + 58, in.y + 243, "B-SPLINE: APPROXIMATES (C2)", ink);
        drawCaption(img, in.x + 12, in.bottom() - 34, "CR TANGENT M_I = (P_I+1 - P_I-1)/2 FED TO HERMITE.\nB-SPLINE BASIS (1-T)^3/6 ... T^3/6, SMOOTHER, OFF-POINT");
    }

    // ---- 5: project - origami diagram arrows ----------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(4, 3, 2, img.width(), img.height(), top), "PROJECT: FOLD ARROWS IN THE DIAGRAM VIEW");
        const OrigamiModel dog = dogFaceModel();
        CreasePattern cp(dog.paper);
        cp.addCrease({0, 200}, {400, 200}, CreaseType::Valley);
        const Mat3 view = fitView(cp.paper.bounds(), Rect{in.x, in.y, in.w - 110, in.h - 40}, 14);
        CreaseViewOptions o; o.showGrid = false; o.showVertices = false;
        Polygon front = cp.paper.transformed(view);
        fillPolygon(img, front.v, pal.paperFront);
        o.fillPaper = false;
        drawCreasePattern(img, cp, view, pal, o);
        drawFoldArrow(img, view.apply({200, 30}), view.apply({200, 370}), 0.35, ink, ArrowHead::Filled, 2);
        // a mountain-fold arrow (hollow head) to the side
        const Vec2 a{in.right() - 95.0, in.y + 60.0}, b{in.right() - 30.0, in.y + 170.0};
        drawFoldArrow(img, a, b, -0.4, ink, ArrowHead::Hollow, 2);
        drawText(img, in.right() - 105, in.y + 190, "MOUNTAIN", ink);
        drawText(img, in.right() - 105, in.y + 202, "(HOLLOW HEAD)", theme::Caption);
        drawCaption(img, in.x + 12, in.bottom() - 34, "STEP 1 OF THE DOG: VALLEY ARROW = CUBIC BEZIER BOWED\nOFF THE CHORD + FILLED (SCANLINE) TRIANGLE HEAD");
    }

    // ---- 6: project - easing curve ---------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(5, 3, 2, img.width(), img.height(), top), "PROJECT: BEZIER EASING TIMES EACH FOLD");
        const int gx = in.x + 30, gy = in.y + 16, gs = 190;
        img.fillRect(gx, gy, gs, gs, Color(250, 248, 244));
        img.strokeRect(gx, gy, gs, gs, Color(220, 215, 205));
        std::vector<Vec2> lin, ease;
        for (int i = 0; i <= 100; ++i) {
            const double t = i / 100.0;
            lin.push_back({gx + gs * t, gy + gs * (1 - t)});
            ease.push_back({gx + gs * t, gy + gs * (1 - easeInOut(t))});
        }
        drawCurve(img, lin, Stroke::dashed(Color(160, 160, 160)));
        drawCurve(img, ease, Stroke::solid(Color::hex(0xE4572E), 3));
        drawControlPolygon(img, {{double(gx), double(gy + gs)}, {gx + 0.42 * gs, double(gy + gs)}, {gx + 0.58 * gs, double(gy)}, {double(gx + gs), double(gy)}}, Color(150, 150, 150));
        drawText(img, gx, gy + gs + 6, "T (TIME)", theme::Caption);
        // the corner of the paper at evenly spaced times: bunched at the ends
        for (int i = 0; i <= 10; ++i) {
            const double p = easeInOut(i / 10.0);
            const int y = int(gy + 10 + p * (gs - 20));
            fillCircle(img, in.right() - 70, y, 5, lerp(pal.paperFront, Color::hex(0x1F6FD1), p));
        }
        drawText(img, in.right() - 120, gy + gs + 6, "CORNER AT 10 FPS", theme::Caption);
        drawCaption(img, in.x + 12, in.bottom() - 34, "CONTROL POINTS (0,0) (.42,0) (.58,1) (1,1): SOLVE X(U)=T\nBY BISECTION, USE Y(U) AS THE FOLD PROGRESS");
    }

    const std::string out = ctx.path("06_curves");
    img.save(out);
    return {out};
}

} // namespace ori
