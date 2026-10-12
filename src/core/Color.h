// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : core            Owner : Ray Lukorito (App shell & export)
//  File   : Color.h
//  RGBA colour type with the small amount of colour maths the rasterizer needs
//  (linear interpolation, scaling and "source-over" alpha blending).
// ============================================================================
#pragma once
#include <cstdint>
#include <algorithm>

namespace ori {

struct Color {
    std::uint8_t r = 0, g = 0, b = 0, a = 255;

    constexpr Color() = default;
    constexpr Color(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_, std::uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}

    // Build a colour from a 0xRRGGBB literal, e.g. Color::hex(0xE4572E).
    static constexpr Color hex(std::uint32_t rgb, std::uint8_t alpha = 255) {
        return Color(static_cast<std::uint8_t>((rgb >> 16) & 0xFF),
                     static_cast<std::uint8_t>((rgb >> 8) & 0xFF),
                     static_cast<std::uint8_t>(rgb & 0xFF), alpha);
    }

    bool operator==(const Color& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }
    bool operator!=(const Color& o) const { return !(*this == o); }

    // Same RGB, ignoring alpha (used by flood / boundary fill comparisons).
    bool sameRGB(const Color& o) const { return r == o.r && g == o.g && b == o.b; }

    Color withAlpha(std::uint8_t alpha) const { return Color(r, g, b, alpha); }
};

inline std::uint8_t clampByte(double v) {
    v = std::clamp(v, 0.0, 255.0);
    return static_cast<std::uint8_t>(v + 0.5);   // 255.0 + 0.5 truncates to 255
}

// Linear interpolation between two colours, t in [0,1].
inline Color lerp(const Color& c0, const Color& c1, double t) {
    t = std::clamp(t, 0.0, 1.0);
    return Color(clampByte(c0.r + (c1.r - c0.r) * t),
                 clampByte(c0.g + (c1.g - c0.g) * t),
                 clampByte(c0.b + (c1.b - c0.b) * t),
                 clampByte(c0.a + (c1.a - c0.a) * t));
}

// Multiply RGB by a brightness factor (k < 1 darkens, k > 1 lightens).
inline Color scale(const Color& c, double k) {
    return Color(clampByte(c.r * k), clampByte(c.g * k), clampByte(c.b * k), c.a);
}

// Porter-Duff "source over": draw src (with coverage alpha in [0,1]) onto dst.
inline Color blendOver(const Color& dst, const Color& src, double alpha) {
    alpha = std::clamp(alpha * (src.a / 255.0), 0.0, 1.0);
    return Color(clampByte(src.r * alpha + dst.r * (1.0 - alpha)),
                 clampByte(src.g * alpha + dst.g * (1.0 - alpha)),
                 clampByte(src.b * alpha + dst.b * (1.0 - alpha)),
                 255);
}

// A few named colours used across the demos.
namespace colors {
    constexpr Color White  = Color(255, 255, 255);
    constexpr Color Black  = Color(0, 0, 0);
    constexpr Color Ink    = Color(34, 34, 40);
    constexpr Color Grey   = Color(150, 150, 150);
    constexpr Color Red    = Color(220, 50, 47);
    constexpr Color Blue   = Color(38, 110, 200);
    constexpr Color Green  = Color(60, 160, 90);
    constexpr Color Orange = Color(240, 140, 30);
    constexpr Color Purple = Color(130, 80, 180);
}

} // namespace ori
