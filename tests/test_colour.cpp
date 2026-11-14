// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : colour maths, palettes, depth shading, lighting, GIF export
//  Owner : Matthew Wachira (Colour, shading & curves)
// ============================================================================
#include "TestFramework.h"
#include "../src/core/Color.h"
#include "../src/core/GifWriter.h"
#include "../src/shading/Palette.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

using namespace ori;

namespace {

std::vector<std::uint8_t> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(f), {});
}

} // namespace

TEST(colour_maths) {
    CHECK(lerp(colors::Black, colors::White, 0.5).r == 128);
    CHECK(lerp(colors::Black, colors::White, 2.0) == colors::White);           // t is clamped
    CHECK(Color::hex(0xE4572E) == Color(0xE4, 0x57, 0x2E));
    CHECK(scale(Color(100, 200, 250), 1.2) == Color(120, 240, 255));            // clamped at 255
    CHECK(blendOver(colors::White, colors::Black, 0.25) == Color(191, 191, 191));
    CHECK(blendOver(colors::White, colors::Black.withAlpha(0), 1.0) == colors::White);
    CHECK(clampByte(-5) == 0 && clampByte(300) == 255 && clampByte(254.6) == 255);
}

TEST(shading_depth_and_light) {
    const Palette pal = Palette::classic();
    int prev = 1 << 30;
    for (int rank = 0; rank < 8; ++rank) {
        const Color c = depthShade(pal.paperFront, rank, 8);
        const int sum = c.r + c.g + c.b;
        CHECK(sum <= prev);                                  // never lighter going down
        prev = sum;
    }
    CHECK(depthShade(pal.paperFront, 0, 8) == pal.paperFront);
    CHECK(depthShade(pal.paperFront, 3, 1) == pal.paperFront);       // a single layer is not shaded
    CHECK(depthShade(pal.paperFront, 50, 60) == depthShade(pal.paperFront, 60, 60));   // capped
    const BBox box{{0, 0}, {100, 100}};
    const Vec2 light{-1, -1};
    CHECK(lightingFactor({0, 0}, box, light) > lightingFactor({100, 100}, box, light));
    CHECK_NEAR(lightingFactor({50, 50}, box, light), 1.0, 1e-12);
    CHECK(Palette::indigo().name == "indigo" && Palette::blueprint().paperFront != Palette::classic().paperFront);
}

TEST(colour_gif_writer_and_palette) {
    GifWriter gif;
    CHECK(gif.open("ori_test_tmp.gif", 16, 12, 5));
    for (int f = 0; f < 3; ++f) {
        Image img(16, 12, Color(std::uint8_t(f * 80), 40, 200));
        CHECK(gif.addFrame(img));
    }
    CHECK(gif.close());
    const auto g = readFile("ori_test_tmp.gif");
    CHECK(g.size() > 13 && std::string(g.begin(), g.begin() + 6) == "GIF89a");
    CHECK(g[6] == 16 && g[8] == 12);              // logical screen size, little endian
    CHECK(g.back() == 0x3B);                      // trailer
    std::remove("ori_test_tmp.gif");
    // Palette: pure colours map close to themselves.
    const Color c = gifPaletteColor(gifPaletteIndex(Color(255, 0, 0), 0, 0));
    CHECK(c.r > 200 && c.g < 60 && c.b < 60);
}
