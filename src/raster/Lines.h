#pragma once
#include "../core/Image.h"

namespace ori {

void drawLine(Image& img, int x0, int y0, int x1, int y1, Color c);

void drawValleyCrease(Image& img, int x0, int y0, int x1, int y1, Color c, int dash = 5, int gap = 5);

void drawMountainCrease(Image& img, int x0, int y0, int x1, int y1, Color c, int dash = 5, int gap = 3, int dot = 1);

}