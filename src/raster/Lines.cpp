// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : raster          Owner : Mathew Nguithi (Rasterizer)
//  File   : Lines.cpp
// ============================================================================
#include "Lines.h"
#include <cmath>

namespace ori {

void drawLine(Image& img, int x0, int y0, int x1, int y1, Color c) {
    int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        img.setPixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void drawValleyCrease(Image& img, int x0, int y0, int x1, int y1, Color c, int dash, int gap) {
    int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int pixelCount = 0;
    int patternLength = dash + gap;

    while (true) {
        if ((pixelCount % patternLength) < dash) {
            img.setPixel(x0, y0, c);
        }
        pixelCount++;
        
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void drawMountainCrease(Image& img, int x0, int y0, int x1, int y1, Color c, int dash, int gap, int dot) {
    int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int pixelCount = 0;
    int patternLength = dash + gap + dot + gap;

    while (true) {
        int mod = pixelCount % patternLength;
        if (mod < dash || (mod >= dash + gap && mod < dash + gap + dot)) {
            img.setPixel(x0, y0, c);
        }
        pixelCount++;
        
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

}