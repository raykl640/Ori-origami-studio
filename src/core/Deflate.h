// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : core            Owner : Evans Mwendwa (Geometry, crease editor & file formats)
//  File   : Deflate.h
//  A small DEFLATE / zlib compressor (RFC 1950 / 1951) for the PNG writer:
//    * LZ77 with hash chains over a 32 KB window (matches of 3..258 bytes)
//    * the fixed Huffman code table from the RFC (no tables to transmit)
//  Flat-coloured diagrams compress 20-100x with this alone.
// ============================================================================
#pragma once
#include <cstdint>
#include <vector>

namespace ori {

// Raw DEFLATE stream (one fixed-Huffman block).
std::vector<std::uint8_t> deflateFixed(const std::vector<std::uint8_t>& data);

// zlib wrapper: 2-byte header + deflate stream + Adler-32 trailer.
std::vector<std::uint8_t> zlibCompress(const std::vector<std::uint8_t>& data);

std::uint32_t adler32(const std::vector<std::uint8_t>& data);

} // namespace ori
