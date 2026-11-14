// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demo 11 : Layering and colour               Owner : Edgar Kareithi
//  Painter's algorithm (progressive), what happens without z-ordering,
//  translucent X-ray preview (alpha blending), isometric exploded stack,
//  layer inspector, depth shading, lighting and three colour palettes.
// ============================================================================
#include "Demos.h"
#include "../src/layering/LayerRenderer.h"
#include "../src/raster/Lines.h"
#include <cstdio>

namespace ori {

std::vector<std::string> demoLayersShading(const DemoContext& ctx) {
    int top = 0;
    Image img = makeSheet("LAYERING AND COLOUR", "PAINTER'S ALGORITHM, ALPHA BLENDING, LAYER INSPECTION, DEPTH SHADING AND PALETTES", top);
    const Palette pal = Palette::classic();
    const OrigamiModel dog = dogFaceModel(), kite = kiteConeModel();
    const FoldState dogDone = dog.run().back(), kiteDone = kite.run().back();
    RenderOptions dogOpt;
    dogOpt.whiteSideUp = dog.whiteSideUp;
    const auto stack = dogDone.bottomToTop();
    const int n = int(stack.size());

    // ---- 1: painter's algorithm, progressive -------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(0, 3, 2, img.width(), img.height(), top), "PAINTER'S ALGORITHM: BACK TO FRONT");
        const int counts[4] = {n / 4, n / 2, 3 * n / 4, n};
        for (int k = 0; k < 4; ++k) {
            const Rect cell{in.x + 8 + (k % 2) * (in.w / 2), in.y + 6 + (k / 2) * 112, in.w / 2 - 16, 110};
            RenderOptions o = dogOpt;
            o.maxPanels = counts[k];
            o.shadow = false;
            drawText(img, cell.x + 6, cell.y + 2, std::to_string(counts[k]) + "/" + std::to_string(n) + " PANELS", Color::hex(0x2B2B33));
            drawPanels(img, stack, fitView(panelBounds(dogDone.panels), Rect{cell.x, cell.y + 10, cell.w, cell.h - 10}, 8), pal, o);
        }
        drawCaption(img, in.x + 12, in.bottom() - 26, "SORT BY LAYER, LOWEST FIRST: LATER FILLS OVERWRITE");
    }

    // ---- 2: no z-order ----------------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(1, 3, 2, img.width(), img.height(), top), "WITHOUT Z-ORDER (REVERSED) - WRONG");
        RenderOptions o = dogOpt;
        o.reverseOrder = true;
        o.depthShading = false;
        drawFoldStateFitted(img, dogDone, Rect{in.x, in.y, in.w, in.h - 40}, pal, o, 18);
        drawCaption(img, in.x + 12, in.bottom() - 34, "EARS AND SNOUT VANISH BEHIND THE BACK LAYERS:\nOCCLUSION ONLY WORKS IF WE PAINT IN LAYER ORDER");
    }

    // ---- 3: translucent X-ray --------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(2, 3, 2, img.width(), img.height(), top), "TRANSLUCENT X-RAY PREVIEW (ALPHA 0.45)");
        RenderOptions o;
        o.whiteSideUp = kite.whiteSideUp;
        o.alpha = 0.45;
        drawFoldStateFitted(img, kiteDone, Rect{in.x, in.y, in.w, in.h - 40}, pal, o, 18);
        drawCaption(img, in.x + 12, in.bottom() - 34, "KITE CONE: THE TIP FOLDED BEHIND SHOWS THROUGH.\nC = A*PANEL + (1-A)*BELOW, LAYER BY LAYER");
    }

    // ---- 4: exploded view ---------------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(3, 3, 2, img.width(), img.height(), top), "EXPLODED STACK (PEEL THE LAYERS APART)");
        drawExplodedView(img, dogDone, Rect{in.x + 10, in.y + 4, in.w - 20, in.h - 44}, pal, 17, dogOpt);
        drawCaption(img, in.x + 12, in.bottom() - 34, "TILT = S(1,0.5) R(45 DEG); EACH LAYER LIFTED 17 PX\nSO HIDDEN PANELS CAN BE COUNTED");
    }

    // ---- 5: layer inspector -----------------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(4, 3, 2, img.width(), img.height(), top), "LAYER INSPECTOR (DOG FACE)");
        drawLayerInspector(img, dogDone, Rect{in.x + 8, in.y + 6, in.w - 16, in.h - 46}, pal, 4, dogOpt);
        drawCaption(img, in.x + 12, in.bottom() - 34, "ONE THUMBNAIL PER Z-LEVEL, TOP FIRST; THE GHOST\nIS THE WHOLE STACK. CORAL = FRONT, CREAM = BACK");
    }

    // ---- 6: depth shading and palettes -----------------------------------------------------------------
    {
        const Rect in = drawPanel(img, gridCell(5, 3, 2, img.width(), img.height(), top), "DEPTH SHADING AND PALETTES");
        const char* names[3] = {"FLAT", "DEPTH", "DEPTH+LIGHT"};
        for (int k = 0; k < 3; ++k) {
            RenderOptions o = dogOpt;
            o.shadow = false;
            o.depthShading = k >= 1;
            o.lighting = k == 2;
            const Rect cell{in.x + 6 + k * (in.w - 12) / 3, in.y + 4, (in.w - 12) / 3, 92};
            drawFoldStateFitted(img, dogDone, cell, pal, o, 10);
            drawText(img, cell.cx() - textWidth(names[k]) / 2, cell.bottom() + 2, names[k], Color::hex(0x2B2B33));
        }
        const Palette pals[3] = {Palette::classic(), Palette::indigo(), Palette::blueprint()};
        for (int k = 0; k < 3; ++k) {
            const Rect cell{in.x + 6 + k * (in.w - 12) / 3, in.y + 112, (in.w - 12) / 3 - 6, 98};
            img.fillRect(cell.x, cell.y, cell.w, cell.h, pals[k].background);
            RenderOptions o = dogOpt;
            o.shadow = false;
            drawFoldStateFitted(img, dogDone, Rect{cell.x, cell.y, cell.w, cell.h - 22}, pals[k], o, 8);
            drawDepthPalette(img, cell.x + 6, cell.bottom() - 26, cell.w - 12, 10, pals[k], 5);
        }
        char buf[120];
        std::snprintf(buf, sizeof buf, "SHADE = LERP(BASE, SHADOW, MIN(%.2F, %.2F*DEPTH)) * LIGHT", kDepthMax, kDepthPerLevel);
        drawCaption(img, in.x + 12, in.bottom() - 20, buf);
    }

    const std::string out = ctx.path("11_layers_shading");
    img.save(out);
    return {out};
}

} // namespace ori
