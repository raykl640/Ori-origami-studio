// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : bitmap font and sheet layout helpers
//  Owner : Edgar Kareithi (Layering, panels & studio layout)
// ============================================================================
#include "TestFramework.h"
#include "../src/core/Font.h"
#include "../src/core/Layout.h"

using namespace ori;

TEST(core_font_metrics) {
    CHECK(textWidth("ORI") == 3 * 6 - 1 || textWidth("ORI") == 3 * 6);
    CHECK(textWidth("AB", 2) == 2 * textWidth("AB"));
    Image img(60, 12, colors::White);
    const int end = drawText(img, 1, 1, "ori", colors::Black);
    CHECK(end > 1);
    long ink = 0;
    for (const Color& p : img.pixels()) ink += p == colors::Black;
    CHECK(ink > 10);
}

TEST(layout_panels_and_grid) {
    Image img(400, 300, theme::Background);
    const int top = drawHeader(img, "TITLE", "SUBTITLE");
    CHECK(top > 0 && img.getPixel(5, 5) == theme::HeaderBg);
    const Rect a = gridCell(0, 3, 2, 400, 300, top, 10), b = gridCell(1, 3, 2, 400, 300, top, 10);
    const Rect last = gridCell(5, 3, 2, 400, 300, top, 10);
    CHECK(a.w == b.w && a.h == b.h && b.x == a.right() + 10);
    CHECK(last.right() <= 400 - 10 + 2 && last.bottom() <= 300 - 10 + 2);    // fits inside the margins
    const Rect in = drawPanel(img, a, "PANEL");
    CHECK(in.x > a.x && in.y > a.y && in.right() <= a.right() && in.bottom() <= a.bottom());
    CHECK(img.getPixel(a.x, a.y) == theme::Border);
    CHECK(textWidth("") == 0);
}
