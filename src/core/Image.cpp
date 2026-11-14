// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : core            Owner : Ray Lukorito (App shell & export)
//  File   : Image.cpp
//  Framebuffer + hand-written image encoders:
//    * PPM (P6)  - simplest possible format, header + raw RGB
//    * BMP       - 24-bit, bottom-up rows padded to 4 bytes (opens on Windows)
//    * PNG       - adaptive row filters (None/Sub/Up/Average/Paeth) and our own
//                  DEFLATE compressor (Deflate.cpp), CRC-32 and Adler-32.
//                  savePNG(path, false) writes stored (uncompressed) blocks.
//                  No zlib / libpng needed.
// ============================================================================
#include "Image.h"
#include "Deflate.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>

namespace ori {

Image::Image(int width, int height, Color background)
    : w_(width), h_(height), px_(static_cast<std::size_t>(width) * height, background) {}

void Image::setPixel(int x, int y, Color c) {
    if (!inBounds(x, y)) return;
    if (c.a == 255) px_[static_cast<std::size_t>(y) * w_ + x] = c;
    else            blendPixel(x, y, c, 1.0);
}

void Image::blendPixel(int x, int y, Color c, double alpha) {
    if (!inBounds(x, y)) return;
    Color& dst = px_[static_cast<std::size_t>(y) * w_ + x];
    dst = blendOver(dst, c, alpha);
}

Color Image::getPixel(int x, int y) const {
    if (!inBounds(x, y)) return Color(0, 0, 0, 0);
    return px_[static_cast<std::size_t>(y) * w_ + x];
}

void Image::clear(Color c) { std::fill(px_.begin(), px_.end(), c); }

void Image::fillRect(int x, int y, int w, int h, Color c) {
    for (int j = std::max(0, y); j < std::min(h_, y + h); ++j)
        for (int i = std::max(0, x); i < std::min(w_, x + w); ++i)
            setPixel(i, j, c);
}

void Image::strokeRect(int x, int y, int w, int h, Color c) {
    for (int i = x; i < x + w; ++i) { setPixel(i, y, c); setPixel(i, y + h - 1, c); }
    for (int j = y; j < y + h; ++j) { setPixel(x, j, c); setPixel(x + w - 1, j, c); }
}

void Image::blit(const Image& src, int dx, int dy) {
    for (int y = 0; y < src.height(); ++y)
        for (int x = 0; x < src.width(); ++x)
            setPixel(dx + x, dy + y, src.getPixel(x, y));
}

// ---------------------------------------------------------------- dispatch --
static std::string lowerExt(const std::string& path) {
    auto dot = path.find_last_of('.');
    std::string e = dot == std::string::npos ? "" : path.substr(dot + 1);
    std::transform(e.begin(), e.end(), e.begin(), [](unsigned char ch) { return std::tolower(ch); });
    return e;
}

bool Image::save(const std::string& path) const {
    const std::string e = lowerExt(path);
    if (e == "bmp") return saveBMP(path);
    if (e == "ppm") return savePPM(path);
    return savePNG(path);
}

// --------------------------------------------------------------------- PPM --
bool Image::savePPM(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f << "P6\n" << w_ << " " << h_ << "\n255\n";
    for (const Color& c : px_) { f.put(char(c.r)); f.put(char(c.g)); f.put(char(c.b)); }
    return bool(f);
}

// --------------------------------------------------------------------- BMP --
static void putLE16(std::ofstream& f, std::uint16_t v) { f.put(char(v & 0xFF)); f.put(char(v >> 8)); }
static void putLE32(std::ofstream& f, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) f.put(char((v >> (8 * i)) & 0xFF));
}

bool Image::saveBMP(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    const int rowBytes = (w_ * 3 + 3) & ~3;              // rows padded to 4 bytes
    const std::uint32_t dataSize = std::uint32_t(rowBytes) * h_;
    // BITMAPFILEHEADER (14 bytes)
    f.put('B'); f.put('M');
    putLE32(f, 54 + dataSize); putLE16(f, 0); putLE16(f, 0); putLE32(f, 54);
    // BITMAPINFOHEADER (40 bytes)
    putLE32(f, 40); putLE32(f, std::uint32_t(w_)); putLE32(f, std::uint32_t(h_));
    putLE16(f, 1); putLE16(f, 24); putLE32(f, 0); putLE32(f, dataSize);
    putLE32(f, 2835); putLE32(f, 2835); putLE32(f, 0); putLE32(f, 0);
    // Pixel rows, bottom-up, stored as B,G,R
    for (int y = h_ - 1; y >= 0; --y) {
        int written = 0;
        for (int x = 0; x < w_; ++x) {
            const Color& c = px_[std::size_t(y) * w_ + x];
            f.put(char(c.b)); f.put(char(c.g)); f.put(char(c.r));
            written += 3;
        }
        while (written++ < rowBytes) f.put(0);
    }
    return bool(f);
}

// --------------------------------------------------------------------- PNG --
namespace {

std::array<std::uint32_t, 256> makeCrcTable() {
    std::array<std::uint32_t, 256> t{};
    for (std::uint32_t n = 0; n < 256; ++n) {
        std::uint32_t c = n;
        for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        t[n] = c;
    }
    return t;
}

std::uint32_t crc32(const std::vector<std::uint8_t>& bytes) {
    static const auto table = makeCrcTable();
    std::uint32_t c = 0xFFFFFFFFu;
    for (std::uint8_t b : bytes) c = table[(c ^ b) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

void pushBE32(std::vector<std::uint8_t>& v, std::uint32_t x) {
    v.push_back(std::uint8_t(x >> 24)); v.push_back(std::uint8_t(x >> 16));
    v.push_back(std::uint8_t(x >> 8));  v.push_back(std::uint8_t(x));
}

void writeChunk(std::ofstream& f, const char type[4], const std::vector<std::uint8_t>& data) {
    std::vector<std::uint8_t> buf;
    pushBE32(buf, std::uint32_t(data.size()));
    std::vector<std::uint8_t> crcPart(type, type + 4);
    crcPart.insert(crcPart.end(), data.begin(), data.end());
    buf.insert(buf.end(), crcPart.begin(), crcPart.end());
    pushBE32(buf, crc32(crcPart));
    f.write(reinterpret_cast<const char*>(buf.data()), std::streamsize(buf.size()));
}

} // namespace

namespace {

std::uint8_t paeth(int a, int b, int c) {
    const int p = a + b - c, pa = std::abs(p - a), pb = std::abs(p - b), pc = std::abs(p - c);
    if (pa <= pb && pa <= pc) return std::uint8_t(a);
    return std::uint8_t(pb <= pc ? b : c);
}

// Filter one scanline with each of the 5 PNG filters and keep the one with
// the smallest sum of |signed byte| - the usual heuristic, and it makes
// flat-coloured rows almost all zeros.
void filterRow(const std::uint8_t* cur, const std::uint8_t* prev, int len, std::vector<std::uint8_t>& out) {
    const int bpp = 3;
    std::vector<std::uint8_t> best, trial(static_cast<std::size_t>(len));
    long bestScore = -1;
    std::uint8_t bestType = 0;
    for (std::uint8_t type = 0; type < 5; ++type) {
        long score = 0;
        for (int i = 0; i < len; ++i) {
            const int a = i >= bpp ? cur[i - bpp] : 0, b = prev ? prev[i] : 0, c = (prev && i >= bpp) ? prev[i - bpp] : 0;
            int pred = 0;
            switch (type) {
                case 1: pred = a; break;
                case 2: pred = b; break;
                case 3: pred = (a + b) / 2; break;
                case 4: pred = paeth(a, b, c); break;
                default: break;
            }
            trial[std::size_t(i)] = std::uint8_t(cur[i] - pred);
            score += std::abs(int(std::int8_t(trial[std::size_t(i)])));
        }
        if (bestScore < 0 || score < bestScore) { bestScore = score; bestType = type; best = trial; }
    }
    out.push_back(bestType);
    out.insert(out.end(), best.begin(), best.end());
}

} // namespace

bool Image::savePNG(const std::string& path, bool compress) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    static const std::uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    f.write(reinterpret_cast<const char*>(sig), 8);

    std::vector<std::uint8_t> ihdr;
    pushBE32(ihdr, std::uint32_t(w_)); pushBE32(ihdr, std::uint32_t(h_));
    ihdr.insert(ihdr.end(), {8 /*bit depth*/, 2 /*RGB*/, 0, 0, 0});
    writeChunk(f, "IHDR", ihdr);

    // RGB scanlines.
    const int rowLen = w_ * 3;
    std::vector<std::uint8_t> rgb(std::size_t(h_) * rowLen);
    for (std::size_t i = 0; i < px_.size(); ++i) {
        rgb[3 * i] = px_[i].r; rgb[3 * i + 1] = px_[i].g; rgb[3 * i + 2] = px_[i].b;
    }

    std::vector<std::uint8_t> z;
    if (compress) {
        // Filtered scanlines (filter type byte + bytes), then DEFLATE.
        std::vector<std::uint8_t> filtered;
        filtered.reserve(std::size_t(h_) * (rowLen + 1));
        for (int y = 0; y < h_; ++y)
            filterRow(&rgb[std::size_t(y) * rowLen], y ? &rgb[std::size_t(y - 1) * rowLen] : nullptr, rowLen, filtered);
        z = zlibCompress(filtered);
    } else {
        // Filter "None" + stored (uncompressed) deflate blocks of <= 65535 bytes.
        std::vector<std::uint8_t> raw;
        raw.reserve(std::size_t(h_) * (rowLen + 1));
        for (int y = 0; y < h_; ++y) {
            raw.push_back(0);
            raw.insert(raw.end(), rgb.begin() + std::ptrdiff_t(y) * rowLen, rgb.begin() + std::ptrdiff_t(y + 1) * rowLen);
        }
        z = {0x78, 0x01};
        std::size_t pos = 0;
        do {
            const std::size_t len = std::min<std::size_t>(65535, raw.size() - pos);
            const bool last = pos + len == raw.size();
            z.push_back(last ? 1 : 0);
            z.push_back(std::uint8_t(len & 0xFF));  z.push_back(std::uint8_t(len >> 8));
            z.push_back(std::uint8_t(~len & 0xFF)); z.push_back(std::uint8_t((~len >> 8) & 0xFF));
            z.insert(z.end(), raw.begin() + std::ptrdiff_t(pos), raw.begin() + std::ptrdiff_t(pos + len));
            pos += len;
        } while (pos < raw.size());
        pushBE32(z, adler32(raw));
    }

    writeChunk(f, "IDAT", z);
    writeChunk(f, "IEND", {});
    return bool(f);
}

} // namespace ori
