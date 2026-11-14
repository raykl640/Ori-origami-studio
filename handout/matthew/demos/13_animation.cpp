// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demo 13 : Fold animation                   Owner : Matthew Wachira
//  Every fold swings through the partial-fold matrix
//      M(t) = T(a) R(theta) S(1, cos(pi t)) R(-theta) T(-a)
//  with t driven by the Bezier ease-in-out curve. Frames are composed by the
//  painter's algorithm and exported as one sheet and as an animated GIF
//  (our own LZW encoder).
// ============================================================================
#include "Demos.h"
#include "../src/core/GifWriter.h"
#include "../src/curves/Curves.h"
#include "../src/layering/LayerRenderer.h"
#include "../src/raster/Lines.h"
#include <algorithm>
#include <cstdio>

namespace ori {

namespace {

BBox animationBounds(const std::vector<FoldState>& states) {
    BBox b = panelBounds(states.front().panels);
    for (const FoldState& s : states) {
        const BBox o = panelBounds(s.panels);
        b.min.x = std::min(b.min.x, o.min.x); b.min.y = std::min(b.min.y, o.min.y);
        b.max.x = std::max(b.max.x, o.max.x); b.max.y = std::max(b.max.y, o.max.y);
    }
    return b;
}

// One frame of fold `step` at eased progress t (in [0, 1]) into `r`.
void drawFrame(Image& img, const OrigamiModel& m, const std::vector<FoldState>& states, std::size_t step,
               double t, const Mat3& view) {
    RenderOptions opt;
    opt.whiteSideUp = m.whiteSideUp;
    if (step >= m.steps.size()) {
        drawFoldState(img, states.back(), view, Palette::classic(), opt);
        drawDecoration(img, m.decoration, view);
        return;
    }
    drawPanels(img, FoldEngine::inProgress(states[step], m.steps[step], t), view, Palette::classic(), opt);
}

} // namespace

std::vector<Image> foldAnimationFrames(const OrigamiModel& m, int size, int framesPerFold, int hold) {
    const auto states = m.run();
    const Mat3 view = fitView(animationBounds(states), Rect{0, 0, size, size}, size / 10);
    std::vector<Image> frames;
    for (std::size_t s = 0; s < m.steps.size(); ++s) {
        for (int h = 0; h < hold; ++h) {                       // rest on the state before folding
            Image img(size, size, theme::Background);
            drawFrame(img, m, states, s, 0.0, view);
            frames.push_back(img);
        }
        for (int f = 1; f <= framesPerFold; ++f) {
            Image img(size, size, theme::Background);
            drawFrame(img, m, states, s, easeInOut(double(f) / framesPerFold), view);
            frames.push_back(img);
        }
    }
    for (int h = 0; h < hold * 4; ++h) {                       // linger on the finished model
        Image img(size, size, theme::Background);
        drawFrame(img, m, states, m.steps.size(), 1.0, view);
        frames.push_back(img);
    }
    return frames;
}

bool writeFoldGif(const OrigamiModel& m, const std::string& path, int size, int framesPerFold) {
    const auto frames = foldAnimationFrames(m, size, framesPerFold);
    GifWriter gif;
    if (!gif.open(path, size, size, 6)) return false;
    for (const Image& f : frames)
        if (!gif.addFrame(f)) return false;
    return gif.close();
}

std::vector<std::string> demoAnimation(const DemoContext& ctx) {
    const OrigamiModel dog = dogFaceModel();
    const auto states = dog.run();
    int top = 0;
    Image img = makeSheet("FOLD ANIMATION", "PARTIAL FOLD M(T) = T R S(1, COS(PI T)) R^-1 T^-1, T EASED BY A CUBIC BEZIER - EXPORTED AS AN ANIMATED GIF", top);

    // 4 folds x 5 frames: t = 0, .25, .5, .75, 1 (eased) for each fold.
    const int cols = 5, rows = 4, gap = 10;
    const Rect area{16, top + 16, img.width() - 32 - 260, img.height() - top - 32};
    const int cw = (area.w - gap * (cols - 1)) / cols, ch = (area.h - gap * (rows - 1)) / rows;
    const Mat3 world = fitView(animationBounds(states), Rect{0, 0, cw, ch - 16}, 10);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) {
            const double u = c / double(cols - 1);
            const double t = easeInOut(u);
            const Rect cell{area.x + c * (cw + gap), area.y + r * (ch + gap), cw, ch};
            img.fillRect(cell.x, cell.y, cell.w, cell.h, theme::Panel);
            img.strokeRect(cell.x, cell.y, cell.w, cell.h, theme::Border);
            Image frame(cw - 2, ch - 18, theme::Panel);
            RenderOptions opt;
            opt.whiteSideUp = dog.whiteSideUp;
            opt.shadow = false;
            drawPanels(frame, FoldEngine::inProgress(states[std::size_t(r)], dog.steps[std::size_t(r)], t), world,
                       Palette::classic(), opt);
            img.blit(frame, cell.x + 1, cell.y + 1);
            char buf[64];
            std::snprintf(buf, sizeof buf, "FOLD %d  U=%.2f  T=%.2f", r + 1, u, t);
            drawText(img, cell.x + 6, cell.bottom() - 13, buf, theme::Caption);
        }

    // Side panel: what a frame is.
    {
        const Rect in = drawPanel(img, Rect{img.width() - 16 - 250, top + 16, 250, img.height() - top - 32}, "HOW A FRAME IS MADE");
        drawCaption(img, in.x + 10, in.y + 12,
                    "1. U = FRAME / FRAMES\n"
                    "2. T = EASE(U): SOLVE THE CUBIC\n"
                    "   BEZIER X(S) = U, T = Y(S)\n"
                    "3. SPLIT EVERY PANEL ON THE\n"
                    "   FOLD LINE (HALF-PLANE CLIP)\n"
                    "4. MOVING PIECES USE M(T):\n"
                    "   DISTANCE TO THE CREASE\n"
                    "   SHRINKS BY COS(PI T)\n"
                    "5. T > 0.5: THE FLAP HAS PASSED\n"
                    "   UPRIGHT AND SHOWS ITS BACK\n"
                    "6. PAINTER'S ALGORITHM DRAWS\n"
                    "   THE STACK BOTTOM TO TOP");
        // the easing curve
        const int gx = in.x + 30, gy = in.y + 190, gs = 170;
        img.fillRect(gx, gy, gs, gs, Color(250, 248, 244));
        img.strokeRect(gx, gy, gs, gs, theme::Border);
        std::vector<Vec2> curve;
        for (int i = 0; i <= 60; ++i) curve.push_back({gx + gs * i / 60.0, gy + gs * (1 - easeInOut(i / 60.0))});
        drawCurve(img, curve, Stroke::solid(theme::Accent, 2));
        for (int c = 0; c < cols; ++c) {
            const double u = c / double(cols - 1);
            const Vec2 p{gx + gs * u, gy + gs * (1 - easeInOut(u))};
            img.fillRect(int(p.x) - 3, int(p.y) - 3, 7, 7, Palette::classic().valley);
        }
        drawText(img, gx, gy + gs + 6, "U (FRAME TIME) -> T", theme::Caption);
        const int frames = int(foldAnimationFrames(dog, 64, 12).size());
        drawCaption(img, in.x + 10, in.bottom() - 60,
                    "GIF: " + std::to_string(frames) + " FRAMES, 6/100 S EACH\n"
                    "252-COLOUR CUBE + 4X4 BAYER\n"
                    "DITHER + LZW (OUR ENCODER)\n"
                    "13_FOLD_DOG_FACE.GIF");
    }

    const std::string out = ctx.path("13_animation_frames");
    img.save(out);
    const std::string gif = ctx.file("13_fold_dog_face.gif");
    writeFoldGif(dog, gif, 420, 12);
    return {out, gif};
}

} // namespace ori
