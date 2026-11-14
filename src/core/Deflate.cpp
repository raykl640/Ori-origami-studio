// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : core            Owner : Evans Mwendwa (Geometry, crease editor & file formats)
//  File   : Deflate.cpp
// ============================================================================
#include "Deflate.h"
#include <algorithm>

namespace ori {

namespace {

// Bits are packed least-significant first, as DEFLATE requires.
class BitWriter {
public:
    std::vector<std::uint8_t> out;
    void bits(std::uint32_t value, int count) {
        for (int i = 0; i < count; ++i) {
            if (fill_ == 0) out.push_back(0);
            if ((value >> i) & 1u) out.back() |= std::uint8_t(1u << fill_);
            fill_ = (fill_ + 1) & 7;
        }
    }
    // Huffman codes are defined most-significant bit first.
    void code(std::uint32_t c, int len) {
        for (int i = len - 1; i >= 0; --i) bits((c >> i) & 1u, 1);
    }
private:
    int fill_ = 0;
};

// Fixed literal / length code (RFC 1951 section 3.2.6).
void writeLitLen(BitWriter& w, int sym) {
    if (sym < 144)      w.code(0x30u + std::uint32_t(sym), 8);
    else if (sym < 256) w.code(0x190u + std::uint32_t(sym - 144), 9);
    else if (sym < 280) w.code(std::uint32_t(sym - 256), 7);
    else                w.code(0xC0u + std::uint32_t(sym - 280), 8);
}

const int kLenBase[29]  = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
                           67, 83, 99, 115, 131, 163, 195, 227, 258};
const int kLenExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
const int kDistBase[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769,
                           1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
const int kDistExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

void writeMatch(BitWriter& w, int length, int dist) {
    int li = 28;
    while (kLenBase[li] > length) --li;
    writeLitLen(w, 257 + li);
    w.bits(std::uint32_t(length - kLenBase[li]), kLenExtra[li]);
    int di = 29;
    while (kDistBase[di] > dist) --di;
    w.code(std::uint32_t(di), 5);                          // fixed distance codes are 5 bits
    w.bits(std::uint32_t(dist - kDistBase[di]), kDistExtra[di]);
}

constexpr int kWindow = 32768;
constexpr int kHashBits = 15;
constexpr int kMaxChain = 48;
constexpr int kMinMatch = 3, kMaxMatch = 258;

inline std::uint32_t hash3(const std::uint8_t* p) {
    const std::uint32_t v = (std::uint32_t(p[0]) << 16) | (std::uint32_t(p[1]) << 8) | p[2];
    return (v * 2654435761u) >> (32 - kHashBits);
}

} // namespace

std::vector<std::uint8_t> deflateFixed(const std::vector<std::uint8_t>& data) {
    BitWriter w;
    w.bits(1, 1);   // BFINAL
    w.bits(1, 2);   // BTYPE = 01, fixed Huffman
    const int n = int(data.size());
    std::vector<int> head(1u << kHashBits, -1), prev(kWindow, -1);
    auto insert = [&](int i) {
        if (i + kMinMatch > n) return;
        const std::uint32_t h = hash3(&data[std::size_t(i)]);
        prev[std::size_t(i % kWindow)] = head[h];
        head[h] = i;
    };
    int i = 0;
    while (i < n) {
        int bestLen = 0, bestDist = 0;
        if (i + kMinMatch <= n) {
            int cand = head[hash3(&data[std::size_t(i)])];
            const int maxLen = std::min(kMaxMatch, n - i);
            for (int chain = 0; cand >= 0 && i - cand <= kWindow - 1 && chain < kMaxChain; ++chain) {
                if (data[std::size_t(cand + bestLen)] == data[std::size_t(i + bestLen)]) {   // quick reject
                    int len = 0;
                    while (len < maxLen && data[std::size_t(cand + len)] == data[std::size_t(i + len)]) ++len;
                    if (len > bestLen) { bestLen = len; bestDist = i - cand; if (len == maxLen) break; }
                }
                const int p = prev[std::size_t(cand % kWindow)];
                if (p >= cand) break;   // stale entry from an older window
                cand = p;
            }
        }
        if (bestLen >= kMinMatch) {
            writeMatch(w, bestLen, bestDist);
            for (int k = 0; k < bestLen; ++k) insert(i + k);
            i += bestLen;
        } else {
            writeLitLen(w, data[std::size_t(i)]);
            insert(i);
            ++i;
        }
    }
    writeLitLen(w, 256);   // end of block
    return w.out;
}

std::uint32_t adler32(const std::vector<std::uint8_t>& data) {
    std::uint32_t a = 1, b = 0;
    for (std::uint8_t byte : data) { a = (a + byte) % 65521; b = (b + a) % 65521; }
    return (b << 16) | a;
}

std::vector<std::uint8_t> zlibCompress(const std::vector<std::uint8_t>& data) {
    std::vector<std::uint8_t> z = {0x78, 0x9C};   // deflate, 32 KB window, default level
    const auto body = deflateFixed(data);
    z.insert(z.end(), body.begin(), body.end());
    const std::uint32_t ad = adler32(data);
    z.push_back(std::uint8_t(ad >> 24)); z.push_back(std::uint8_t(ad >> 16));
    z.push_back(std::uint8_t(ad >> 8));  z.push_back(std::uint8_t(ad));
    return z;
}

} // namespace ori
