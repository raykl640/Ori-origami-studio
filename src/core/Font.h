// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : core            Owner : Ray Lukorito (App shell & export)
//  File   : Font.h
//  A tiny 5x7 bitmap font so rendered images can carry their own labels
//  (we have no text API - every glyph is plotted pixel by pixel).
// ============================================================================
#pragma once
#include "Image.h"
#include <string>

namespace ori {

// Draw text with its top-left corner at (x, y). Each glyph is 5x7 "font pixels",
// each font pixel is `scale` x `scale` screen pixels. Lower-case is drawn as
// upper-case. Returns the x coordinate just after the last glyph.
int drawText(Image& img, int x, int y, const std::string& text, Color c, int scale = 1);

// Width in pixels that drawText would use.
int textWidth(const std::string& text, int scale = 1);

// Text centred horizontally on cx.
void drawTextCentered(Image& img, int cx, int y, const std::string& text, Color c, int scale = 1);

} // namespace ori
