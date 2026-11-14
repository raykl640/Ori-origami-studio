// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : DEFLATE / zlib compressor and PNG decoding of our own files
//  Owner : Evans Mwendwa (Geometry, crease editor & file formats)
// ============================================================================
#include "TestFramework.h"
#include "../src/core/Deflate.h"
#include "../src/core/Image.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>

using namespace ori;

namespace {

// A minimal INFLATE for fixed-Huffman blocks, so the tests can decode what
// our compressor writes without any library.
struct BitReader {
    const std::vector<std::uint8_t>& d;
    std::size_t pos = 0;
    int bit = 0;
    explicit BitReader(const std::vector<std::uint8_t>& data) : d(data) {}
    int get() {
        if (pos >= d.size()) return 0;
        const int b = (d[pos] >> bit) & 1;
        if (++bit == 8) { bit = 0; ++pos; }
        return b;
    }
    int bits(int n) { int v = 0; for (int i = 0; i < n; ++i) v |= get() << i; return v; }
    int code(int n) { int v = 0; for (int i = 0; i < n; ++i) v = (v << 1) | get(); return v; }
};

bool inflateFixed(const std::vector<std::uint8_t>& in, std::vector<std::uint8_t>& out) {
    static const int lenBase[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static const int lenExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static const int distBase[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
    static const int distExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
    BitReader r(in);
    if (r.bits(1) != 1 || r.bits(2) != 1) return false;   // one final fixed block
    while (true) {
        int sym = r.code(7);
        if (sym <= 0x17) sym += 256;                        // 256..279: 7 bits
        else {
            sym = (sym << 1) | r.get();                      // 8 bits
            if (sym >= 0x30 && sym <= 0xBF) sym -= 0x30;     // literals 0..143
            else if (sym >= 0xC0 && sym <= 0xC7) sym = sym - 0xC0 + 280;
            else { sym = (sym << 1) | r.get(); sym = sym - 0x190 + 144; }   // 9 bits: 144..255
        }
        if (sym < 256) { out.push_back(std::uint8_t(sym)); continue; }
        if (sym == 256) return true;
        const int li = sym - 257;
        if (li < 0 || li >= 29) return false;
        const int len = lenBase[li] + r.bits(lenExtra[li]);
        const int di = r.code(5);
        if (di >= 30) return false;
        const int dist = distBase[di] + r.bits(distExtra[di]);
        if (dist > int(out.size())) return false;
        for (int k = 0; k < len; ++k) out.push_back(out[out.size() - std::size_t(dist)]);
    }
}

std::vector<std::uint8_t> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(f), {});
}

} // namespace

TEST(export_deflate_roundtrip) {
    std::vector<std::vector<std::uint8_t>> inputs;
    inputs.push_back({});
    inputs.push_back({42});
    std::vector<std::uint8_t> runs(70000, 7);                  // long runs, > one window
    inputs.push_back(runs);
    std::vector<std::uint8_t> text;
    const std::string s = "ORI folds paper with reflections. ORI folds paper with reflections! ";
    for (int i = 0; i < 300; ++i) text.insert(text.end(), s.begin(), s.end());
    inputs.push_back(text);
    std::vector<std::uint8_t> noise;
    unsigned x = 12345;
    for (int i = 0; i < 50000; ++i) { x = x * 1103515245u + 12345u; noise.push_back(std::uint8_t(x >> 16)); }
    inputs.push_back(noise);
    for (const auto& in : inputs) {
        const auto z = deflateFixed(in);
        std::vector<std::uint8_t> out;
        CHECK(inflateFixed(z, out));
        CHECK(out == in);
    }
    CHECK(deflateFixed(runs).size() < 1000);                   // runs compress hugely
    CHECK(deflateFixed(text).size() < text.size() / 20);
    // Adler-32 of "Wikipedia" (the classic example) is 0x11E60398.
    const std::string w = "Wikipedia";
    CHECK(adler32(std::vector<std::uint8_t>(w.begin(), w.end())) == 0x11E60398u);
    const auto z = zlibCompress(text);
    CHECK(z[0] == 0x78 && ((z[0] << 8) | z[1]) % 31 == 0);    // valid zlib header check bits
}

TEST(export_png_pixels_survive_filter_and_deflate) {
    Image img(37, 23, colors::White);
    for (int y = 0; y < 23; ++y)
        for (int x = 0; x < 37; ++x) img.setPixel(x, y, Color(std::uint8_t(x * 7), std::uint8_t(y * 11), 90));
    CHECK(img.savePNG("ori_test_export.png"));
    const auto png = readFile("ori_test_export.png");
    // Decode our own PNG IDAT and compare every pixel (filters undone by hand).
    std::size_t i = 8;
    std::vector<std::uint8_t> idat;
    while (i + 8 <= png.size()) {
        const std::size_t n = (std::size_t(png[i]) << 24) | (std::size_t(png[i + 1]) << 16) | (std::size_t(png[i + 2]) << 8) | png[i + 3];
        if (std::string(png.begin() + std::ptrdiff_t(i + 4), png.begin() + std::ptrdiff_t(i + 8)) == "IDAT")
            idat.insert(idat.end(), png.begin() + std::ptrdiff_t(i + 8), png.begin() + std::ptrdiff_t(i + 8 + n));
        i += 12 + n;
    }
    std::vector<std::uint8_t> raw;
    CHECK(inflateFixed(std::vector<std::uint8_t>(idat.begin() + 2, idat.end() - 4), raw));
    const int stride = 37 * 3;
    CHECK(raw.size() == std::size_t(23 * (stride + 1)));
    std::vector<std::uint8_t> prev(static_cast<std::size_t>(stride), 0), cur(static_cast<std::size_t>(stride));
    bool same = true;
    for (int y = 0; y < 23 && raw.size() == std::size_t(23 * (stride + 1)); ++y) {
        const int ft = raw[std::size_t(y * (stride + 1))];
        for (int k = 0; k < stride; ++k) {
            const int a = k >= 3 ? cur[std::size_t(k - 3)] : 0, b = prev[std::size_t(k)], c = k >= 3 ? prev[std::size_t(k - 3)] : 0;
            int pred = 0;
            if (ft == 1) pred = a;
            else if (ft == 2) pred = b;
            else if (ft == 3) pred = (a + b) / 2;
            else if (ft == 4) {
                const int p = a + b - c, pa = std::abs(p - a), pb = std::abs(p - b), pc = std::abs(p - c);
                pred = (pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c);
            }
            cur[std::size_t(k)] = std::uint8_t(raw[std::size_t(y * (stride + 1) + 1 + k)] + pred);
        }
        for (int x = 0; x < 37; ++x) {
            const Color p = img.getPixel(x, y);
            if (cur[std::size_t(3 * x)] != p.r || cur[std::size_t(3 * x + 1)] != p.g || cur[std::size_t(3 * x + 2)] != p.b) same = false;
        }
        prev = cur;
    }
    CHECK(same);
    std::remove("ori_test_export.png");
}
