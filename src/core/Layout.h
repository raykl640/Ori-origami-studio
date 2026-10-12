// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : core            Owner : Ray Lukorito (App shell & export)
//  File   : Layout.h
//  Small helpers for composing labelled demo sheets (titled panels, a header
//  bar and captions) so every exported image explains itself.
// ============================================================================
#pragma once
#include "Font.h"
#include "Image.h"
#include <string>

namespace ori {

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    int cx() const { return x + w / 2; }
    int cy() const { return y + h / 2; }
    int right() const { return x + w; }
    int bottom() const { return y + h; }
};

namespace theme {
    constexpr Color Background = Color(246, 243, 236);   // warm paper-studio grey
    constexpr Color Panel      = Color(255, 255, 255);
    constexpr Color Border     = Color(210, 204, 192);
    constexpr Color Title      = Color(40, 38, 48);
    constexpr Color Caption    = Color(110, 104, 96);
    constexpr Color Accent     = Color(228, 87, 46);      // ORI coral
    constexpr Color HeaderBg   = Color(40, 38, 48);
}

// Full-width dark header bar with the sheet title; returns its height.
int drawHeader(Image& img, const std::string& title, const std::string& subtitle);

// White panel with a border and a title; returns the inner content rectangle.
Rect drawPanel(Image& img, const Rect& r, const std::string& title);

// Caption text under/inside a panel (wraps on '\n').
void drawCaption(Image& img, int x, int y, const std::string& text, Color c = theme::Caption);

// Grid of n panels inside the area below the header.
Rect gridCell(int index, int cols, int rows, int imgW, int imgH, int top, int margin = 16);

} // namespace ori
