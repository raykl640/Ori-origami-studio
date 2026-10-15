// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : core            Owner : Ray Lukorito (App shell & export)
//  File   : Layout.cpp
// ============================================================================
#include "Layout.h"

namespace ori {

int drawHeader(Image& img, const std::string& title, const std::string& subtitle) {
    const int h = 58;
    img.fillRect(0, 0, img.width(), h, theme::HeaderBg);
    img.fillRect(0, h - 4, img.width(), 4, theme::Accent);
    drawText(img, 18, 12, "ORI", theme::Accent, 3);
    drawText(img, 84, 12, title, colors::White, 2);
    drawText(img, 84, 34, subtitle, Color(190, 186, 178), 1);
    return h;
}

Rect drawPanel(Image& img, const Rect& r, const std::string& title) {
    img.fillRect(r.x, r.y, r.w, r.h, theme::Panel);
    img.strokeRect(r.x, r.y, r.w, r.h, theme::Border);
    img.fillRect(r.x + 1, r.y + 1, r.w - 2, 20, Color(250, 248, 244));
    img.fillRect(r.x + 1, r.y + 21, r.w - 2, 1, theme::Border);
    img.fillRect(r.x + 8, r.y + 7, 3, 9, theme::Accent);
    drawText(img, r.x + 16, r.y + 8, title, theme::Title, 1);
    return Rect{r.x + 1, r.y + 22, r.w - 2, r.h - 23};
}

void drawCaption(Image& img, int x, int y, const std::string& text, Color c) {
    std::size_t start = 0;
    int line = 0;
    while (start <= text.size()) {
        std::size_t nl = text.find('\n', start);
        std::string part = text.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
        drawText(img, x, y + line * 11, part, c, 1);
        ++line;
        if (nl == std::string::npos) break;
        start = nl + 1;
    }
}

Rect gridCell(int index, int cols, int rows, int imgW, int imgH, int top, int margin) {
    const int cellW = (imgW - margin * (cols + 1)) / cols;
    const int cellH = (imgH - top - margin * (rows + 1)) / rows;
    const int c = index % cols, r = index / cols;
    return Rect{margin + c * (cellW + margin), top + margin + r * (cellH + margin), cellW, cellH};
}

} // namespace ori
