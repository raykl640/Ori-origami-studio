// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demos : fold-sequence sheets               Owner : Mohamed Bile (Fold engine)
//  Shared by demos 08-10 and the `ori fold` command. Each step is drawn as
//  an origami diagram: the state so far, the next crease (valley dashed /
//  mountain dash-dot) and a Bezier fold arrow, followed by the result, the
//  generated crease pattern and the fold-sequence list.
// ============================================================================
#include "Demos.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/layering/LayerRenderer.h"
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

// One scale for every panel so the model visibly shrinks as it folds.
BBox unionBounds(const std::vector<FoldState>& states) {
    BBox b = panelBounds(states.front().panels);
    for (const FoldState& s : states) {
        const BBox o = panelBounds(s.panels);
        b.min.x = std::min(b.min.x, o.min.x); b.min.y = std::min(b.min.y, o.min.y);
        b.max.x = std::max(b.max.x, o.max.x); b.max.y = std::max(b.max.y, o.max.y);
    }
    return b;
}

std::string stats(const FoldState& s) {
    const int n = int(s.panels.size()), l = s.layerCount();
    return std::to_string(n) + (n == 1 ? " PANEL, " : " PANELS, ") + std::to_string(l) + (l == 1 ? " LAYER" : " LAYERS");
}

} // namespace

Image foldSequenceSheet(const OrigamiModel& m, const std::string& title, const std::string& subtitle,
                        const std::vector<std::string>& filesWritten) {
    const auto states = m.run();
    const Palette pal = Palette::classic();
    const int panels = int(states.size()) + 3;           // every state + result + crease pattern + list
    const int cols = 4, rows = (panels + cols - 1) / cols;
    int top = 0;
    Image img = makeSheet(title, subtitle, top, kSheetW, std::max(kSheetH, 58 + 16 + rows * 356));
    const BBox world = unionBounds(states);
    RenderOptions opt;
    opt.whiteSideUp = m.whiteSideUp;

    int cell = 0;
    for (std::size_t i = 0; i < states.size(); ++i, ++cell) {
        const std::string name = i == 0 ? "FLAT SHEET" : upper(m.steps[i - 1].label);
        const Rect in = drawPanel(img, gridCell(cell, cols, rows, img.width(), img.height(), top),
                                  std::to_string(i) + ". " + name);
        const Rect area{in.x, in.y, in.w, in.h - 30};
        const Mat3 view = fitView(world, area, 22);
        drawFoldState(img, states[i], view, pal, opt);
        std::string caption = stats(states[i]);
        if (i < m.steps.size()) {
            drawFoldDiagram(img, states[i], m.steps[i], view, pal);
            caption += "   NEXT: " + upper(foldKindName(m.steps[i].kind));
        }
        drawCaption(img, in.x + 10, in.bottom() - 18, caption);
    }

    // Result: painter's order + depth shading + lighting + decoration.
    {
        const Rect in = drawPanel(img, gridCell(cell++, cols, rows, img.width(), img.height(), top), "RESULT");
        const Mat3 view = fitView(world, Rect{in.x, in.y, in.w, in.h - 30}, 22);
        drawFoldState(img, states.back(), view, pal, opt);
        drawDecoration(img, m.decoration, view);
        drawCaption(img, in.x + 10, in.bottom() - 18, "PAINTER'S ORDER + DEPTH SHADING");
    }

    // The crease pattern the engine produced on the flat sheet.
    {
        const Rect in = drawPanel(img, gridCell(cell++, cols, rows, img.width(), img.height(), top), "GENERATED CREASE PATTERN");
        const CreasePattern cp = creasePatternOf(states.back(), m.name);
        const Mat3 view = fitView(cp.paper.bounds(), Rect{in.x, in.y, in.w, in.h - 30}, 22);
        CreaseViewOptions o;
        o.showGrid = false;
        drawCreasePattern(img, cp, view, pal, o);
        drawCaption(img, in.x + 10, in.bottom() - 18,
                    std::to_string(cp.count(CreaseType::Mountain)) + " MOUNTAIN, " +
                    std::to_string(cp.count(CreaseType::Valley)) + " VALLEY SEGMENTS");
    }

    // Fold sequence panel (region 2 of the studio UI).
    {
        const Rect in = drawPanel(img, gridCell(cell++, cols, rows, img.width(), img.height(), top), "FOLD SEQUENCE PANEL");
        int y = in.y + 12;
        for (std::size_t i = 0; i < m.steps.size(); ++i) {
            const FoldStep& st = m.steps[i];
            img.fillRect(in.x + 10, y, 18, 18, pal.valley);
            drawText(img, in.x + 16, y + 6, std::to_string(i + 1), colors::White);
            drawText(img, in.x + 36, y + 2, upper(st.label), theme::Title);
            drawText(img, in.x + 36, y + 11,
                     st.kind == FoldKind::Valley ? "VALLEY - FLAP COMES UP, ON TOP" : "MOUNTAIN - FLAP GOES BEHIND",
                     theme::Caption);
            y += 30;
            if (y > in.bottom() - 60) break;
        }
        if (!filesWritten.empty()) {
            int fy = in.bottom() - 16 - 11 * int(filesWritten.size());
            drawText(img, in.x + 10, fy - 12, "FILES WRITTEN:", theme::Caption);
            for (const std::string& f : filesWritten) {
                drawText(img, in.x + 10, fy, upper(f), theme::Caption);
                fy += 11;
            }
        }
    }
    return img;
}

Image foldResultImage(const OrigamiModel& m, int size) {
    Image img(size, size, theme::Background);
    const auto states = m.run();
    RenderOptions opt;
    opt.whiteSideUp = m.whiteSideUp;
    const Mat3 view = drawFoldStateFitted(img, states.back(), Rect{0, 0, size, size}, Palette::classic(), opt, size / 10);
    drawDecoration(img, m.decoration, view);
    return img;
}

} // namespace ori
