// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : core            Owner : Ray Lukorito (App shell & export)
//  File   : Image.h
//  The framebuffer. Every algorithm in the project writes pixels through this
//  class - no graphics library is used. Images are exported with our own
//  PNG / BMP / PPM encoders (see Image.cpp).
// ============================================================================
#pragma once
#include "Color.h"
#include <string>
#include <vector>

namespace ori {

class Image {
public:
    Image() = default;
    Image(int width, int height, Color background = colors::White);

    int width()  const { return w_; }
    int height() const { return h_; }

    bool inBounds(int x, int y) const { return x >= 0 && y >= 0 && x < w_ && y < h_; }

    // Plot a single pixel (silently ignored when outside the framebuffer).
    void setPixel(int x, int y, Color c);
    // Plot a pixel with partial coverage / transparency (alpha in [0,1]).
    void blendPixel(int x, int y, Color c, double alpha);
    Color getPixel(int x, int y) const;

    void clear(Color c);
    // Axis-aligned rectangle helpers (used for UI chrome, not for the algorithms).
    void fillRect(int x, int y, int w, int h, Color c);
    void strokeRect(int x, int y, int w, int h, Color c);
    // Copy another image into this one at (dx, dy).
    void blit(const Image& src, int dx, int dy);

    // Export - format picked from the extension (.png, .bmp, .ppm).
    bool save(const std::string& path) const;
    bool savePNG(const std::string& path) const;
    bool saveBMP(const std::string& path) const;
    bool savePPM(const std::string& path) const;

    const std::vector<Color>& pixels() const { return px_; }

private:
    int w_ = 0, h_ = 0;
    std::vector<Color> px_;
};

} // namespace ori
