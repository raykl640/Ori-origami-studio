// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Studio : immediate-mode UI toolkit          Owner : Mathew Nguithi (Rasterizer)
//  File   : Widgets.h
//  An immediate-mode GUI drawn entirely with the project rasterizer: every
//  frame the panels call ui.button(...), ui.slider(...) and so on; each call
//  both draws the widget and reports what the mouse / keyboard did to it.
//  No windowing code here - the SDL shell (app/studio_main.cpp) only fills
//  a UiInput and shows the finished Image - so the whole UI can be driven
//  by unit tests.
// ============================================================================
#pragma once
#include "../src/core/Font.h"
#include "../src/core/Image.h"
#include "../src/core/Layout.h"
#include <map>
#include <string>
#include <vector>

namespace ori {

enum class Key { None, Escape, Enter, Backspace, Delete, Left, Right, Up, Down, Home, End,
                 PageUp, PageDown, Space, Tab, F1, F2, Char };

struct KeyPress {
    Key key = Key::None;
    char ch = 0;          // for Key::Char: lower-case letter, digit or symbol
    bool ctrl = false, shift = false, alt = false;
    bool is(char c) const { return key == Key::Char && ch == c; }
};

// Everything that happened since the previous frame.
struct UiInput {
    int mouseX = -1, mouseY = -1;
    bool mouseDown = false;      // left button held
    bool pressed = false;        // left button went down this frame
    bool released = false;       // left button went up this frame
    bool doubleClick = false;
    bool rightPressed = false;
    int wheel = 0;               // + = away from the user (scroll up)
    bool shift = false, ctrl = false, alt = false;
    std::vector<KeyPress> keys;
    std::string text;            // typed characters (text fields)
    std::vector<std::string> droppedFiles;

    void clearEvents();          // keep mouse position / held state, drop one-shot events
};

namespace ui {
    constexpr Color Bg        = Color(236, 232, 224);
    constexpr Color Bar       = Color(40, 38, 48);
    constexpr Color Panel     = Color(255, 255, 255);
    constexpr Color Border    = Color(210, 204, 192);
    constexpr Color Text      = Color(40, 38, 48);
    constexpr Color Muted     = Color(120, 114, 106);
    constexpr Color Accent    = Color(228, 87, 46);
    constexpr Color AccentBg  = Color(252, 232, 222);
    constexpr Color Button    = Color(246, 243, 236);
    constexpr Color ButtonHot = Color(234, 228, 216);
    constexpr Color Disabled  = Color(190, 185, 178);
}

bool contains(const Rect& r, int x, int y);
Rect inset(const Rect& r, int d);

class Ui {
public:
    // Start a frame: draw into `target`, read `input`. Ids of active drags
    // survive between frames because the Ui object lives in the Studio.
    void begin(Image& target, const UiInput& input);
    Image& img() { return *img_; }
    const UiInput& input() const { return in_; }

    bool hover(const Rect& r) const;
    // Where each enabled button was drawn in the last frame (by label) - lets
    // tests and tools click buttons by name.
    const std::map<std::string, Rect>& buttons() const { return buttons_; }
    // A left press inside r that no widget has used yet; marks it used.
    bool clickIn(const Rect& r);
    bool rightClickIn(const Rect& r);
    bool pressAvailable() const { return in_.pressed && !pressUsed_; }
    void usePress() { pressUsed_ = true; }

    // Drag tracking: startDrag(id) when a press is taken; dragging(id) is
    // true while the button stays down; dragEnded(id) on the release frame.
    void startDrag(const std::string& id) { active_ = id; }
    bool dragging(const std::string& id) const { return active_ == id && in_.mouseDown; }
    bool dragEnded(const std::string& id);
    bool anyDrag() const { return !active_.empty(); }
    void cancelDrag() { active_.clear(); }

    // ---- widgets ----------------------------------------------------------
    bool button(const Rect& r, const std::string& label, bool selected = false, bool enabled = true);
    bool toggle(const Rect& r, const std::string& label, bool& value);
    // Horizontal slider; returns true while the value changes.
    bool slider(const std::string& id, const Rect& r, double& value, double lo, double hi);
    // Single-line text field that always has focus while shown; true on Enter.
    bool textField(const Rect& r, std::string& value);
    // Vertical list scrolling: adjusts `scroll` (pixels) with the wheel over r.
    void scrollArea(const Rect& r, int contentHeight, int& scroll);

    // ---- drawing helpers ----------------------------------------------------
    Rect panel(const Rect& r, const std::string& title, int number = 0);
    void label(int x, int y, const std::string& text, Color c = ui::Text, int scale = 1);
    void labelCentered(const Rect& r, const std::string& text, Color c = ui::Text, int scale = 1);
    void badge(const Rect& r, const std::string& text, Color bg, Color fg = colors::White);
    // Clip helper: copy-on-write region so content drawn inside stays inside.
    void beginClip(const Rect& r);
    void endClip();

private:
    Image* img_ = nullptr;
    UiInput in_;
    bool pressUsed_ = false;
    std::map<std::string, Rect> buttons_;
    std::string active_;
    // clipping
    Rect clip_;
    Image saved_;
    bool clipping_ = false;
};

} // namespace ori
