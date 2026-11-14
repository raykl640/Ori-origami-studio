// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : shading         Owner : Matthew Wachira (Colour, shading & curves)
//  File   : Palette.cpp
// ============================================================================
#include "Palette.h"
#include "../core/Font.h"
#include <algorithm>
#include <cmath>

namespace ori {

Palette Palette::classic() {
    return {"classic", Color::hex(0xE4572E), Color::hex(0xFBF7EE), Color::hex(0xD62828),
            Color::hex(0x1F6FD1), Color::hex(0x2B2B33), Color::hex(0x3A2E2A),
            Color::hex(0xF6F3EC), Color::hex(0xE4DFD3)};
}

Palette Palette::indigo() {
    return {"indigo", Color::hex(0x3D4E9E), Color::hex(0xF8F6F0), Color::hex(0xD62828),
            Color::hex(0x1F6FD1), Color::hex(0x23232B), Color::hex(0x1B1F3A),
            Color::hex(0xF4F2EE), Color::hex(0xE2DED6)};
}

Palette Palette::blueprint() {
    return {"blueprint", Color::hex(0xF2C14E), Color::hex(0xFFFFFF), Color::hex(0xFF5A5F),
            Color::hex(0x7FDBFF), Color::hex(0xFFFFFF), Color::hex(0x0B2545),
            Color::hex(0x13315C), Color::hex(0x1F4E85)};
}

Color depthShade(Color base, int rank, int layerCount, double perLevel, double maxDarken) {
    if (layerCount <= 1 || rank <= 0) return base;
    const double k = std::min(maxDarken, perLevel * rank);
    // Darken towards a warm shadow instead of pure black so light paper
    // reads as "paper in shadow" rather than grey.
    const Color shadow = Color(clampByte(base.r * 0.55 + 20), clampByte(base.g * 0.50 + 12), clampByte(base.b * 0.48 + 8));
    return lerp(base, shadow, k);
}

double lightingFactor(const Vec2& p, const BBox& box, const Vec2& lightDir, double amount) {
    const Vec2 c = (box.min + box.max) * 0.5;
    const double half = std::max(1.0, 0.5 * std::max(box.width(), box.height()));
    const double s = std::clamp(dot(p - c, lightDir.normalized()) / half, -1.0, 1.0);
    return 1.0 + amount * s;
}

void drawDepthPalette(Image& img, int x, int y, int w, int h, const Palette& pal, int layers) {
    layers = std::max(1, layers);
    const int sw = w / layers;
    for (int i = 0; i < layers; ++i) {
        const Color front = depthShade(pal.paperFront, i, layers);
        const Color back = depthShade(pal.paperBack, i, layers);
        img.fillRect(x + i * sw, y, sw - 2, h / 2, front);
        img.fillRect(x + i * sw, y + h / 2, sw - 2, h / 2, back);
        img.strokeRect(x + i * sw, y, sw - 2, h, Color(200, 195, 185));
        drawText(img, x + i * sw + 3, y + h + 4, "L" + std::to_string(layers - 1 - i), Color(110, 104, 96));
    }
}

} // namespace ori
