// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demo 12 : Objects and characters            Owner : Edgar Kareithi
//  The project's models, each built only from the project's own algorithms:
//  the dog face character, the kite cone, the napkin triangle and the
//  paper-crane mascot.
// ============================================================================
#include "Demos.h"
#include "../src/layering/LayerRenderer.h"

namespace ori {

std::vector<std::string> demoCharacters(const DemoContext& ctx) {
    int top = 0;
    Image img = makeSheet("OBJECTS AND CHARACTERS", "CURRENT PROJECT MODELS - EACH BUILT ONLY FROM THE ALGORITHMS IMPLEMENTED SO FAR", top);
    const Palette pal = Palette::classic();

    struct Entry { OrigamiModel model; const char* title; const char* caption; };
    const Entry entries[] = {
        {dogFaceModel(), "DOG FACE (CHARACTER)",
         "4 REFLECTION FOLDS, 8 PANELS.\nSCANLINE FILL + DEPTH SHADING.\nMIDPOINT CIRCLES (EYES), ELLIPSE\n(NOSE), BEZIER (MOUTH, BROWS)"},
        {kiteConeModel(), "KITE CONE",
         "KITE BASE: 2 VALLEY FOLDS ON\nANGLE BISECTORS + 1 MOUNTAIN\nFOLD BEHIND. WU-AA OUTLINES,\nPAINTER'S ORDER"},
        {napkinTriangleModel(), "NAPKIN TRIANGLE",
         "VALLEY + MOUNTAIN + DIAGONAL\nVALLEY: 8 LAYERS DEEP.\nHALF-PLANE CLIPPING SPLITS\nEVERY LAYER AT ONCE"},
    };

    int i = 0;
    for (const Entry& e : entries) {
        const Rect in = drawPanel(img, gridCell(i++, 4, 1, img.width(), img.height(), top), e.title);
        RenderOptions opt;
        opt.whiteSideUp = e.model.whiteSideUp;
        const Mat3 view = drawFoldStateFitted(img, e.model.run().back(), Rect{in.x, in.y + 40, in.w, in.h - 200}, pal, opt, 18);
        drawDecoration(img, e.model.decoration, view);
        drawCaption(img, in.x + 10, in.bottom() - 80, e.caption);
    }
    {
        const Rect in = drawPanel(img, gridCell(3, 4, 1, img.width(), img.height(), top), "PAPER CRANE (MASCOT)");
        drawCraneMascot(img, Rect{in.x, in.y + 120, in.w, in.h - 330}, pal);
        drawCaption(img, in.x + 10, in.bottom() - 80, "CONCAVE POLYGON, EAR-CLIP\nTRIANGULATION -> FACETS,\nCIRCLE EYE, ELLIPSE SHADOW,\nCATMULL-ROM WIND LINES");
    }

    const std::string out = ctx.path("12_characters");
    img.save(out);
    return {out};
}

} // namespace ori
