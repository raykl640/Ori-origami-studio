// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demos : registry + sheet helper            Owner : Edgar Kareithi (Studio layout)
// ============================================================================
#include "Demos.h"

namespace ori {

Image makeSheet(const std::string& title, const std::string& subtitle, int& top, int width, int height) {
    Image img(width, height, theme::Background);
    top = drawHeader(img, title, subtitle);
    return img;
}

const std::vector<DemoInfo>& demoRegistry() {
    static const std::vector<DemoInfo> demos = {
        {"lines",          "01_lines",                 "Line drawing",                    "Mathew Nguithi",  demoLines},
        {"lineops",        "02_line_operations",       "Line operations",                 "Mathew Nguithi",  demoLineOperations},
        {"polygons",       "03_polygons",              "Polygon representation",          "Evans Mwendwa",   demoPolygons},
        {"clipping",       "04_clipping_filling",      "Polygon clipping and filling",    "Mathew Nguithi",  demoClippingFilling},
        {"circles",        "05_circles_ellipses",      "Circles and ellipses",            "Matthew Wachira", demoCirclesEllipses},
        {"curves",         "06_curves",                "Curves: Bezier and splines",      "Matthew Wachira", demoCurves},
        {"transforms",     "07_transformations",       "2D transformations",              "Mohamed Bile",    demoTransformations},
        {"dog",            "08_fold_dog_face",         "Fold sequence: dog face",         "Mohamed Bile",    demoFoldDog},
        {"kite",           "09_fold_kite_cone",        "Fold sequence: kite cone",        "Mohamed Bile",    demoFoldKite},
        {"napkin",         "10_fold_napkin_triangle",  "Fold sequence: napkin triangle",  "Mohamed Bile",    demoFoldNapkin},
        {"layers",         "11_layers_shading",        "Layering and colour",             "Edgar Kareithi",  demoLayersShading},
        {"characters",     "12_characters",            "Objects and characters",          "Edgar Kareithi",  demoCharacters},
        {"animation",      "13_animation_frames",      "Fold animation (+ GIF)",          "Matthew Wachira", demoAnimation},
        {"studio",         "14_studio_ui",             "Studio interface",                "Edgar Kareithi",  demoStudioUI},
    };
    return demos;
}

} // namespace ori
