// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : core            Owner : Ray Lukorito (App shell & export)
//  File   : GifWriter.cpp
// ============================================================================
#include "GifWriter.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace ori {

namespace {
constexpr int RL = 6, GL = 7, BL = 6;               // levels per channel (252 colours)
const int kBayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

int quantise(std::uint8_t c, int levels, int x, int y) {
    const double v = c / 255.0 * (levels - 1);
    const double threshold = (kBayer[y & 3][x & 3] + 0.5) / 16.0;
    return std::clamp(int(std::floor(v + threshold)), 0, levels - 1);
}

void le16(std::ofstream& f, int v) { f.put(char(v & 0xFF)); f.put(char((v >> 8) & 0xFF)); }
}

// Indices 252-255 hold exact copies of the UI's flat colours so large flat
// areas are not dithered.
const Color kReserved[4] = {Color(246, 243, 236), Color(255, 255, 255), Color(40, 38, 48), Color(250, 248, 244)};

std::uint8_t gifPaletteIndex(Color c, int x, int y) {
    for (int i = 0; i < 4; ++i) if (c.sameRGB(kReserved[i])) return std::uint8_t(252 + i);
    return std::uint8_t(quantise(c.r, RL, x, y) * GL * BL + quantise(c.g, GL, x, y) * BL + quantise(c.b, BL, x, y));
}

Color gifPaletteColor(std::uint8_t i) {
    if (i >= RL * GL * BL) return kReserved[i - RL * GL * BL];
    const int r = i / (GL * BL), g = (i / BL) % GL, b = i % BL;
    return Color(std::uint8_t(r * 255 / (RL - 1)), std::uint8_t(g * 255 / (GL - 1)), std::uint8_t(b * 255 / (BL - 1)));
}

bool GifWriter::open(const std::string& path, int width, int height, int delayCs) {
    f_.open(path, std::ios::binary);
    if (!f_) return false;
    w_ = width; h_ = height; delay_ = delayCs;
    f_.write("GIF89a", 6);
    le16(f_, w_); le16(f_, h_);
    f_.put(char(0xF7));          // global colour table, 8 bits/channel, 256 entries
    f_.put(0); f_.put(0);        // background index, aspect ratio
    for (int i = 0; i < 256; ++i) {
        const Color c = gifPaletteColor(std::uint8_t(i));
        f_.put(char(c.r)); f_.put(char(c.g)); f_.put(char(c.b));
    }
    // NETSCAPE2.0 application extension: loop forever
    const unsigned char loop[] = {0x21, 0xFF, 0x0B, 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0',
                                  0x03, 0x01, 0x00, 0x00, 0x00};
    f_.write(reinterpret_cast<const char*>(loop), sizeof loop);
    return bool(f_);
}

bool GifWriter::addFrame(const Image& img, int delayCs) {
    if (!f_.is_open()) return false;
    // Graphic control extension
    f_.put(char(0x21)); f_.put(char(0xF9)); f_.put(4); f_.put(0);
    le16(f_, delayCs < 0 ? delay_ : delayCs);
    f_.put(0); f_.put(0);
    // Image descriptor
    f_.put(char(0x2C)); le16(f_, 0); le16(f_, 0); le16(f_, w_); le16(f_, h_); f_.put(0);

    std::vector<std::uint8_t> idx(std::size_t(w_) * h_);
    for (int y = 0; y < h_; ++y)
        for (int x = 0; x < w_; ++x) idx[std::size_t(y) * w_ + x] = gifPaletteIndex(img.getPixel(x, y), x, y);
    writeLZW(idx);
    return bool(f_);
}

// LZW with variable code width (9..12 bits), codes packed LSB-first and
// written as data sub-blocks of at most 255 bytes.
void GifWriter::writeLZW(const std::vector<std::uint8_t>& data) {
    const int minCodeSize = 8, clearCode = 256, eoiCode = 257;
    f_.put(char(minCodeSize));

    std::vector<std::uint8_t> block;
    std::uint32_t bitBuf = 0;
    int bitCount = 0;
    auto flushBlock = [&]() {
        if (block.empty()) return;
        f_.put(char(block.size()));
        f_.write(reinterpret_cast<const char*>(block.data()), std::streamsize(block.size()));
        block.clear();
    };
    auto emit = [&](int code, int size) {
        bitBuf |= std::uint32_t(code) << bitCount;
        bitCount += size;
        while (bitCount >= 8) {
            block.push_back(std::uint8_t(bitBuf & 0xFF));
            bitBuf >>= 8; bitCount -= 8;
            if (block.size() == 255) flushBlock();
        }
    };

    std::unordered_map<std::uint32_t, int> dict;
    int codeSize = minCodeSize + 1, maxCode = eoiCode;
    emit(clearCode, codeSize);
    int cur = -1;
    for (std::uint8_t px : data) {
        if (cur < 0) { cur = px; continue; }
        const std::uint32_t key = (std::uint32_t(cur) << 8) | px;
        auto it = dict.find(key);
        if (it != dict.end()) { cur = it->second; continue; }
        emit(cur, codeSize);
        dict[key] = ++maxCode;
        if (maxCode >= (1 << codeSize) && codeSize < 12) ++codeSize;
        if (maxCode == 4095) {                       // table full: reset
            emit(clearCode, codeSize);
            dict.clear();
            codeSize = minCodeSize + 1;
            maxCode = eoiCode;
        }
        cur = px;
    }
    if (cur >= 0) emit(cur, codeSize);
    emit(eoiCode, codeSize);
    if (bitCount > 0) { block.push_back(std::uint8_t(bitBuf & 0xFF)); bitBuf = 0; bitCount = 0; }
    flushBlock();
    f_.put(0);                                        // block terminator
}

bool GifWriter::close() {
    if (!f_.is_open()) return false;
    f_.put(char(0x3B));
    f_.close();
    return true;
}

} // namespace ori
