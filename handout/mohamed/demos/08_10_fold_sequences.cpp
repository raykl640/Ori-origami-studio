// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demos 08-10 : fold sequences (dog face, kite cone, napkin triangle)
//  Owner : Mohamed Bile (Fold transformation engine)
//  The fold engine folds each model step by step; the engine also writes the
//  crease pattern it created (mountain / valley assigned per layer) as an
//  editable .cp file and a print-ready .svg.
// ============================================================================
#include "Demos.h"

namespace ori {

// ---------------------------------------------------------------- the demos --
namespace {

std::vector<std::string> foldDemo(const DemoContext& ctx, const OrigamiModel& m, const std::string& file,
                                  const std::string& title, const std::string& subtitle) {
    const auto states = m.run();
    const CreasePattern cp = creasePatternOf(states.back(), m.name);
    const std::string cpPath = ctx.file(file + ".cp"), svgPath = ctx.file(file + ".svg");
    cp.save(cpPath);
    cp.saveSVG(svgPath, 1.5);
    Image img = foldSequenceSheet(m, title, subtitle,
                                  {file + ".cp   (editable text)", file + ".svg  (print-ready)"});
    const std::string out = ctx.path(file);
    img.save(out);
    return {out, cpPath, svgPath};
}

} // namespace

std::vector<std::string> demoFoldDog(const DemoContext& ctx) {
    return foldDemo(ctx, dogFaceModel(), "08_fold_dog_face", "FOLD SEQUENCE: DOG FACE",
                    "EACH FOLD = SPLIT PANELS (HALF-PLANE CLIP) + REFLECT MOVING SIDE + RESTACK LAYERS");
}

std::vector<std::string> demoFoldKite(const DemoContext& ctx) {
    return foldDemo(ctx, kiteConeModel(), "09_fold_kite_cone", "FOLD SEQUENCE: KITE CONE",
                    "KITE BASE ON THE 22.5 DEGREE BISECTORS, THEN A MOUNTAIN FOLD TUCKS THE TIP UNDER THE STACK");
}

std::vector<std::string> demoFoldNapkin(const DemoContext& ctx) {
    return foldDemo(ctx, napkinTriangleModel(), "10_fold_napkin_triangle", "FOLD SEQUENCE: NAPKIN TRIANGLE",
                    "VALLEY, MOUNTAIN, DIAGONAL VALLEY - ONE HALF-PLANE PASS SPLITS EVERY LAYER THE CREASE CROSSES");
}

} // namespace ori
