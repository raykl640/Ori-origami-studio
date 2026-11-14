// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Studio : immediate-mode UI toolkit          Owner : Mathew Nguithi (Rasterizer)
//  File   : Widgets.cpp
// ============================================================================
#include "Widgets.h"
#include "../src/raster/Circle.h"
#include "../src/raster/Fill.h"
#include "../src/raster/Lines.h"
#include <algorithm>
#include <cmath>

namespace ori {

void UiInput::clearEvents() {
    pressed = released = doubleClick = rightPressed = false;
    wheel = 0;
    keys.clear();
    text.clear();
    droppedFiles.clear();
}

bool contains(const Rect& r, int x, int y) { return x >= r.x && y >= r.y && x < r.right() && y < r.bottom(); }
Rect inset(const Rect& r, int d) { return Rect{r.x + d, r.y + d, std::max(0, r.w - 2 * d), std::max(0, r.h - 2 * d)}; }

void Ui::begin(Image& target, const UiInput& input) {
    img_ = &target;
    in_ = input;
    pressUsed_ = false;
    clipping_ = false;
    buttons_.clear();
    // A drag whose button was released while we were not looking ends here.
    if (!in_.mouseDown && !in_.released) active_.clear();
}

bool Ui::hover(const Rect& r) const { return contains(r, in_.mouseX, in_.mouseY); }

bool Ui::clickIn(const Rect& r) {
    if (!pressAvailable() || !hover(r)) return false;
    pressUsed_ = true;
    return true;
}

bool Ui::rightClickIn(const Rect& r) { return in_.rightPressed && hover(r); }

bool Ui::dragEnded(const std::string& id) {
    if (active_ == id && (in_.released || !in_.mouseDown)) {
        active_.clear();
        return true;
    }
    return false;
}

// ------------------------------------------------------------------ widgets --
bool Ui::button(const Rect& r, const std::string& text, bool selected, bool enabled) {
    if (enabled) buttons_[text] = r;
    const bool hot = enabled && hover(r);
    Color bg = selected ? ui::Accent : hot ? ui::ButtonHot : ui::Button;
    if (!enabled) bg = ui::Button;
    img_->fillRect(r.x, r.y, r.w, r.h, bg);
    img_->strokeRect(r.x, r.y, r.w, r.h, selected ? ui::Accent : ui::Border);
    labelCentered(r, text, !enabled ? ui::Disabled : selected ? colors::White : ui::Text);
    return enabled && clickIn(r);
}

bool Ui::toggle(const Rect& r, const std::string& text, bool& value) {
    const Rect box{r.x, r.y + (r.h - 12) / 2, 12, 12};
    img_->fillRect(box.x, box.y, box.w, box.h, value ? ui::Accent : colors::White);
    img_->strokeRect(box.x, box.y, box.w, box.h, value ? ui::Accent : ui::Muted);
    if (value) {
        drawStroke(*img_, {box.x + 2.5, box.y + 6.0}, {box.x + 5.0, box.y + 9.0}, Stroke::solid(colors::White, 2));
        drawStroke(*img_, {box.x + 5.0, box.y + 9.0}, {box.x + 10.0, box.y + 3.0}, Stroke::solid(colors::White, 2));
    }
    label(r.x + 18, r.y + (r.h - 7) / 2, text, hover(r) ? ui::Accent : ui::Text);
    if (clickIn(r)) { value = !value; return true; }
    return false;
}

bool Ui::slider(const std::string& id, const Rect& r, double& value, double lo, double hi) {
    const int cy = r.y + r.h / 2;
    const double t = hi > lo ? std::clamp((value - lo) / (hi - lo), 0.0, 1.0) : 0.0;
    const int knob = r.x + int(std::lround(t * (r.w - 1)));
    img_->fillRect(r.x, cy - 2, r.w, 4, Color(225, 220, 210));
    img_->fillRect(r.x, cy - 2, knob - r.x, 4, ui::Accent);
    const bool active = active_ == id;
    fillCircle(*img_, knob, cy, 6, colors::White);
    drawRing(*img_, knob, cy, 6, 2, active || hover(r) ? ui::Accent : ui::Text);
    if (clickIn(r)) startDrag(id);
    if (dragging(id) || (active_ == id && in_.released)) {
        const double nt = std::clamp(double(in_.mouseX - r.x) / std::max(1, r.w - 1), 0.0, 1.0);
        const double nv = lo + nt * (hi - lo);
        if (in_.released) active_.clear();
        if (nv != value) { value = nv; return true; }
    }
    return false;
}

bool Ui::textField(const Rect& r, std::string& value) {
    for (char c : in_.text)
        if (c >= 32 && c < 127 && value.size() < 60) value += c;
    bool enter = false;
    for (const KeyPress& k : in_.keys) {
        if (k.key == Key::Backspace && !value.empty()) value.pop_back();
        if (k.key == Key::Enter) enter = true;
    }
    img_->fillRect(r.x, r.y, r.w, r.h, colors::White);
    img_->strokeRect(r.x, r.y, r.w, r.h, ui::Accent);
    const int maxChars = std::max(1, (r.w - 12) / 6);
    const std::string shown = value.size() > std::size_t(maxChars) ? value.substr(value.size() - std::size_t(maxChars)) : value;
    label(r.x + 6, r.y + (r.h - 7) / 2, shown);
    const int caret = r.x + 6 + int(shown.size()) * 6;
    img_->fillRect(caret, r.y + 5, 2, r.h - 10, ui::Accent);
    return enter;
}

void Ui::scrollArea(const Rect& r, int contentHeight, int& scroll) {
    const int maxScroll = std::max(0, contentHeight - r.h);
    if (hover(r) && in_.wheel != 0) scroll -= in_.wheel * 30;
    scroll = std::clamp(scroll, 0, maxScroll);
    if (maxScroll > 0) {   // thin scroll bar
        const int barH = std::max(20, r.h * r.h / std::max(1, contentHeight));
        const int barY = r.y + (r.h - barH) * scroll / maxScroll;
        img_->fillRect(r.right() - 4, r.y, 3, r.h, Color(240, 236, 228));
        img_->fillRect(r.right() - 4, barY, 3, barH, ui::Muted);
    }
}

// ------------------------------------------------------------------ drawing --
Rect Ui::panel(const Rect& r, const std::string& title, int number) {
    const Rect in = drawPanel(*img_, r, number > 0 ? "  " + title : title);
    if (number > 0) {
        fillCircle(*img_, r.x + 16, r.y + 11, 6, ui::Accent);
        drawText(*img_, r.x + 14, r.y + 8, std::to_string(number), colors::White);
    }
    return in;
}

void Ui::label(int x, int y, const std::string& text, Color c, int scale) { drawText(*img_, x, y, text, c, scale); }

void Ui::labelCentered(const Rect& r, const std::string& text, Color c, int scale) {
    drawText(*img_, r.x + (r.w - textWidth(text, scale)) / 2, r.y + (r.h - 7 * scale) / 2, text, c, scale);
}

void Ui::badge(const Rect& r, const std::string& text, Color bg, Color fg) {
    img_->fillRect(r.x, r.y, r.w, r.h, bg);
    labelCentered(r, text, fg);
}

// Content drawn between beginClip / endClip is kept only inside the rectangle:
// we remember the pixels around it and put them back afterwards.
void Ui::beginClip(const Rect& r) {
    clip_ = r;
    saved_ = *img_;
    clipping_ = true;
}

void Ui::endClip() {
    if (!clipping_) return;
    clipping_ = false;
    Image& im = *img_;
    // Lists only overflow vertically and a little sideways, so restoring a
    // band around the clip rectangle is enough (and cheap).
    const int x0 = std::max(0, clip_.x - 40), x1 = std::min(im.width(), clip_.right() + 40);
    for (int y = 0; y < im.height(); ++y) {
        const bool rowIn = y >= clip_.y && y < clip_.bottom();
        for (int x = x0; x < x1; ++x)
            if (!rowIn || x < clip_.x || x >= clip_.right()) im.setPixel(x, y, saved_.getPixel(x, y));
    }
}

} // namespace ori
