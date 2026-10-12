// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : core            Owner : Ray Lukorito (App shell & export)
//  File   : GifWriter.h
//  Animated GIF export for fold-sequence animations, written from scratch:
//  fixed 6x7x6 colour cube + 4x4 ordered (Bayer) dithering + LZW compression.
// ============================================================================
#pragma once
#include "Image.h"
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace ori {

class GifWriter {
public:
    // delayCs = frame delay in hundredths of a second; loops forever.
    bool open(const std::string& path, int width, int height, int delayCs = 6);
    bool addFrame(const Image& frame, int delayCs = -1);
    bool close();
    ~GifWriter() { if (f_.is_open()) close(); }

private:
    std::ofstream f_;
    int w_ = 0, h_ = 0, delay_ = 6;
    void writeLZW(const std::vector<std::uint8_t>& indices);
};

// Palette index for a colour at pixel (x, y), with ordered dithering.
std::uint8_t gifPaletteIndex(Color c, int x, int y);
Color gifPaletteColor(std::uint8_t index);

} // namespace ori
