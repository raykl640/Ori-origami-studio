// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Demos : shared declarations                Owner : Edgar Kareithi (Studio layout)
//  Every demo renders one labelled evidence sheet (1200 x 800) for one
//  technique, ending with a "project" panel that shows where ORI uses it.
//  Each demo returns the paths of the files it wrote.
// ============================================================================
#pragma once
#include "../src/core/Font.h"
#include "../src/core/Image.h"
#include "../src/core/Layout.h"
#include "../src/models/Models.h"
#include "../src/shading/Palette.h"
#include <string>
#include <vector>

namespace ori {

struct DemoContext {
    std::string outDir = "output";
    std::string format = "png";      // png | bmp | ppm
    // Full path for an image called `name` in the chosen format.
    std::string path(const std::string& name) const { return outDir + "/" + name + "." + format; }
    // Full path for a file with its own extension (.cp, .svg, .gif ...).
    std::string file(const std::string& nameWithExt) const { return outDir + "/" + nameWithExt; }
};

constexpr int kSheetW = 1200;
constexpr int kSheetH = 800;

// A blank sheet with the ORI header; `top` receives the header height.
Image makeSheet(const std::string& title, const std::string& subtitle, int& top,
                int width = kSheetW, int height = kSheetH);

using DemoFn = std::vector<std::string> (*)(const DemoContext&);

struct DemoInfo {
    const char* key;      // command-line name, e.g. "lines"
    const char* file;     // output base name, e.g. "01_lines"
    const char* title;
    const char* owner;
    DemoFn run;
};

const std::vector<DemoInfo>& demoRegistry();

// ---- shared by the fold demos and the `ori fold` command (FoldSheets.cpp) --
// Step-by-step diagram sheet: every state with its next crease and arrow, the
// result, the generated crease pattern and the fold-sequence list.
Image foldSequenceSheet(const OrigamiModel& m, const std::string& title, const std::string& subtitle,
                        const std::vector<std::string>& filesWritten);
// The finished model on its own (painter's order + depth shading + decoration).
Image foldResultImage(const OrigamiModel& m, int size = 600);
// (demos/13_animation.cpp) Animation frames: every fold swings through Mat3::partialFold with Bezier
// easing; `hold` extra frames rest on each completed state.
std::vector<Image> foldAnimationFrames(const OrigamiModel& m, int size, int framesPerFold, int hold = 3);
bool writeFoldGif(const OrigamiModel& m, const std::string& path, int size = 420, int framesPerFold = 12);

// ---- the demos (one file each in demos/) ---------------------------------
std::vector<std::string> demoLines(const DemoContext& ctx);            // 01  Mathew Nguithi
std::vector<std::string> demoLineOperations(const DemoContext& ctx);   // 02  Mathew Nguithi
std::vector<std::string> demoPolygons(const DemoContext& ctx);         // 03  Evans Mwendwa
std::vector<std::string> demoClippingFilling(const DemoContext& ctx);  // 04  Mathew Nguithi
std::vector<std::string> demoCirclesEllipses(const DemoContext& ctx);  // 05  Matthew Wachira
std::vector<std::string> demoCurves(const DemoContext& ctx);           // 06  Matthew Wachira
std::vector<std::string> demoTransformations(const DemoContext& ctx);  // 07  Mohamed Bile
std::vector<std::string> demoFoldDog(const DemoContext& ctx);          // 08  Mohamed Bile
std::vector<std::string> demoFoldKite(const DemoContext& ctx);         // 09  Mohamed Bile
std::vector<std::string> demoFoldNapkin(const DemoContext& ctx);       // 10  Mohamed Bile
std::vector<std::string> demoLayersShading(const DemoContext& ctx);    // 11  Edgar Kareithi
std::vector<std::string> demoCharacters(const DemoContext& ctx);       // 12  Edgar Kareithi
std::vector<std::string> demoAnimation(const DemoContext& ctx);        // 13  Matthew Wachira
std::vector<std::string> demoStudioUI(const DemoContext& ctx);         // 14  Edgar Kareithi

} // namespace ori
