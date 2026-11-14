// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : framebuffer and the BMP / PPM / PNG file writers
//  Owner : Mathew Nguithi (Rasterizer & test framework)
// ============================================================================
#include "TestFramework.h"
#include "../src/core/Image.h"
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

TEST(core_image_pixels_and_blending) {
    Image img(10, 8, colors::White);
    CHECK(img.width() == 10 && img.height() == 8 && img.pixels().size() == 80);
    img.setPixel(3, 4, colors::Red);
    CHECK(img.getPixel(3, 4) == colors::Red);
    img.setPixel(-1, 100, colors::Red);                       // ignored, no crash
    CHECK(img.getPixel(-1, 0).a == 0);
    img.blendPixel(0, 0, colors::Black, 0.25);
    CHECK(img.getPixel(0, 0).r == 191);
    img.fillRect(-5, -5, 8, 8, colors::Blue);
    CHECK(img.getPixel(2, 2) == colors::Blue && img.getPixel(3, 3) == colors::White);
    CHECK(lerp(colors::Black, colors::White, 0.5).r == 128);
    CHECK(Color::hex(0xE4572E) == Color(0xE4, 0x57, 0x2E));
}

TEST(core_bmp_ppm_png_files) {
    Image img(37, 23, colors::White);
    for (int y = 0; y < 23; ++y)
        for (int x = 0; x < 37; ++x) img.setPixel(x, y, Color(std::uint8_t(x * 7), std::uint8_t(y * 11), 90));
    CHECK(img.save("ori_test_tmp.png") && img.savePNG("ori_test_tmp_raw.png", false));
    CHECK(img.save("ori_test_tmp.bmp") && img.save("ori_test_tmp.ppm"));
    const auto png = readFile("ori_test_tmp.png");
    CHECK(png.size() > 8 && png[0] == 137 && png[1] == 'P' && png[2] == 'N' && png[3] == 'G');
    CHECK(png.size() < readFile("ori_test_tmp_raw.png").size());
    const auto bmp = readFile("ori_test_tmp.bmp");
    CHECK(bmp.size() == 54 + std::size_t((37 * 3 + 3) & ~3) * 23);   // rows padded to 4 bytes
    const auto ppm = readFile("ori_test_tmp.ppm");
    CHECK(ppm.size() == std::string("P6\n37 23\n255\n").size() + 37 * 23 * 3);
    for (const char* f : {"ori_test_tmp.png", "ori_test_tmp_raw.png", "ori_test_tmp.bmp", "ori_test_tmp.ppm"}) std::remove(f);
}
