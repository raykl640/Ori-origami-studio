// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Studio : dialogs (open, save as, rename, new, help, quit)   Owner : Edgar Kareithi (Studio layout)
//  File   : StudioDialogs.cpp
// ============================================================================
#include "Studio.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <set>

namespace ori {

namespace fs = std::filesystem;

namespace {

std::string upper(std::string s) {
    for (char& c : s) c = char(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

std::string extensionOf(const std::string& path) {
    std::string e = fs::path(path).extension().string();
    for (char& c : e) c = char(std::tolower(static_cast<unsigned char>(c)));
    return e;
}

// Text that fits in `width` pixels of the 5x7 font.
std::string fit(const std::string& s, int width) {
    const std::size_t n = std::size_t(std::max(1, (width + 1) / 6));
    return s.size() <= n ? s : s.substr(0, n > 2 ? n - 2 : n) + "..";
}

} // namespace

// --------------------------------------------------------------- dialogs --
void Studio::drawOverlay() {
    Image& img = ui_.img();
    // dim the studio behind the dialog
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x) img.blendPixel(x, y, Color(30, 28, 36), 0.45);
    for (const KeyPress& k : ui_.input().keys)
        if (k.key == Key::Escape) { overlay = Overlay::None; return; }

    auto dialog = [&](int w, int h, const std::string& title) {
        const Rect r{(img.width() - w) / 2, (img.height() - h) / 2, w, h};
        return ui_.panel(r, title);
    };

    switch (overlay) {
    case Overlay::Open: {
        const Rect in = dialog(620, 460, "OPEN A DESIGN (.ORI FOLD SEQUENCE OR .CP CREASE PATTERN)");
        const auto files = listDesignFiles();
        const Rect list{in.x + 10, in.y + 10, in.w - 20, in.h - 60};
        img.fillRect(list.x, list.y, list.w, list.h, Color(250, 248, 244));
        img.strokeRect(list.x, list.y, list.w, list.h, ui::Border);
        const int rowH = 26;
        ui_.scrollArea(list, int(files.size()) * rowH, openScroll_);
        ui_.beginClip(list);
        for (std::size_t i = 0; i < files.size(); ++i) {
            const Rect row{list.x + 1, list.y + 1 + int(i) * rowH - openScroll_, list.w - 8, rowH - 2};
            if (row.bottom() < list.y || row.y > list.bottom()) continue;
            const bool isFold = extensionOf(files[i]) == ".ori";
            if (ui_.hover(row) && ui_.hover(list)) img.fillRect(row.x, row.y, row.w, row.h, ui::AccentBg);
            ui_.badge(Rect{row.x + 6, row.y + 6, 44, 14}, isFold ? "FOLD" : "CP", isFold ? ui::Accent : palette().valley);
            ui_.label(row.x + 60, row.y + 9, fit(files[i], row.w - 70));
            if (ui_.hover(list) && ui_.clickIn(row)) {
                overlay = Overlay::None;
                open(files[i]);
            }
        }
        ui_.endClip();
        if (files.empty()) ui_.labelCentered(list, "NO .ORI OR .CP FILES IN PATTERNS/, " + upper(designsDir) + "/ OR HERE");
        ui_.label(in.x + 12, in.bottom() - 30, "TIP: YOU CAN ALSO DRAG A FILE ONTO THE WINDOW", ui::Muted);
        if (ui_.button(Rect{in.right() - 100, in.bottom() - 38, 90, 28}, "CANCEL")) overlay = Overlay::None;
        break;
    }
    case Overlay::SaveAs:
    case Overlay::Rename: {
        const bool rename = overlay == Overlay::Rename;
        const Rect in = dialog(520, 170, rename ? "RENAME FOLD STEP" : (tab == Tab::Fold ? "SAVE FOLD SEQUENCE AS" : "SAVE CREASE PATTERN AS"));
        const bool enter = ui_.textField(Rect{in.x + 14, in.y + 20, in.w - 28, 30}, textBuffer_);
        ui_.label(in.x + 14, in.y + 62,
                  rename ? "THE LABEL SHOWN IN THE SEQUENCE AND ON EXPORTED DIAGRAMS"
                         : "SAVED IN " + upper(designsDir) + "/ UNLESS YOU TYPE A FOLDER, E.G. PATTERNS/MY_MODEL", ui::Muted);
        const bool ok = ui_.button(Rect{in.right() - 200, in.bottom() - 40, 90, 28}, rename ? "RENAME" : "SAVE", true);
        if (ui_.button(Rect{in.right() - 100, in.bottom() - 40, 90, 28}, "CANCEL")) { overlay = Overlay::None; break; }
        if (ok || enter) {
            overlay = Overlay::None;
            if (rename) {
                if (selectedStep >= 0 && selectedStep < stepCount() && !textBuffer_.empty()) {
                    checkpoint();
                    doc.seq.steps[std::size_t(selectedStep)].label = textBuffer_;
                    setStatus("RENAMED STEP " + std::to_string(selectedStep + 1));
                }
            } else {
                saveAs(textBuffer_);
            }
        }
        break;
    }
    case Overlay::New: {
        const Rect in = dialog(560, tab == Tab::Fold ? 230 : 170, tab == Tab::Fold ? "NEW FOLD SEQUENCE" : "NEW CREASE PATTERN");
        ui_.label(in.x + 14, in.y + 14, "BLANK PAPER:");
        const char* papers[] = {"SQUARE", "DIAMOND", "RECTANGLE 2:1"};
        for (int i = 0; i < 3; ++i)
            if (ui_.button(Rect{in.x + 14 + i * 170, in.y + 32, 160, 34}, papers[i])) { overlay = Overlay::None; newDesign(i); }
        if (tab == Tab::Fold) {
            ui_.label(in.x + 14, in.y + 84, "OR START FROM A BUILT-IN MODEL:");
            const char* models[] = {"DOG FACE", "KITE CONE", "NAPKIN TRIANGLE"};
            for (int i = 0; i < 3; ++i)
                if (ui_.button(Rect{in.x + 14 + i * 170, in.y + 102, 160, 34}, models[i])) { overlay = Overlay::None; newDesign(10 + i); }
        }
        if (ui_.button(Rect{in.right() - 100, in.bottom() - 38, 90, 28}, "CANCEL")) overlay = Overlay::None;
        break;
    }
    case Overlay::ConfirmQuit: {
        const Rect in = dialog(520, 160, "UNSAVED CHANGES");
        std::string what = foldDirty() && cpDirty() ? "THE FOLD SEQUENCE AND THE CREASE PATTERN HAVE"
                         : foldDirty() ? "THE FOLD SEQUENCE HAS" : "THE CREASE PATTERN HAS";
        ui_.label(in.x + 14, in.y + 18, what + " UNSAVED CHANGES.");
        ui_.label(in.x + 14, in.y + 32, "UNNAMED DESIGNS ARE SAVED AS " + upper(designsDir) + "/UNTITLED...", ui::Muted);
        if (ui_.button(Rect{in.x + 14, in.bottom() - 42, 150, 30}, "SAVE AND QUIT", true)) {
            const Tab keep = tab;
            bool ok = true;
            for (Tab t : {Tab::Fold, Tab::Crease}) {
                tab = t;
                if (!(t == Tab::Fold ? foldDirty() : cpDirty())) continue;
                const std::string& p = t == Tab::Fold ? foldPath : cpPath;
                ok = (p.empty() ? saveAs(baseName()) : save()) && ok;
            }
            tab = keep;
            overlay = Overlay::None;
            exit_ = ok;
        }
        if (ui_.button(Rect{in.x + 174, in.bottom() - 42, 190, 30}, "QUIT WITHOUT SAVING")) exit_ = true;
        if (ui_.button(Rect{in.right() - 100, in.bottom() - 42, 90, 30}, "CANCEL")) overlay = Overlay::None;
        break;
    }
    case Overlay::Help: {
        const Rect in = dialog(760, 520, "ORI STUDIO - HELP");
        const char* lines[] = {
            "FOLD SEQUENCE TAB",
            "  DRAG ACROSS THE PREVIEW     ADD A FOLD AT THE CURRENT STEP (PAPER RIGHT OF THE DRAG MOVES)",
            "  SHIFT WHILE DRAGGING        LOCK THE LINE TO 22.5 DEGREE ANGLES",
            "  M / V                       NEXT FOLD (AND THE SELECTED STEP) MOUNTAIN / VALLEY",
            "  F                           FLIP WHICH SIDE OF THE SELECTED FOLD MOVES",
            "  DEL / BACKSPACE             DELETE THE SELECTED STEP     [ / ]  MOVE IT UP / DOWN",
            "  F2 OR DOUBLE-CLICK          RENAME THE SELECTED STEP",
            "  SPACE  LEFT  RIGHT  HOME  END   PLAY, STEP BACK / FORWARD, FIRST / LAST",
            "  G  D  X  E                  GRID SNAP, DIAGRAM ARROWS, X-RAY, EXPLODED VIEW",
            "",
            "CREASE PATTERN TAB",
            "  D / S / E                   DRAW, SELECT, ERASE TOOL",
            "  M / V / A                   DRAW MOUNTAIN, VALLEY OR AUXILIARY CREASES",
            "  T, DEL                      TOGGLE / DELETE THE SELECTED CREASE",
            "  L  G                        FULL-LENGTH LINES, GRID SNAP     + / -  GRID SIZE",
            "",
            "EVERYWHERE",
            "  CTRL+N  CTRL+O  CTRL+S  CTRL+SHIFT+S   NEW, OPEN, SAVE, SAVE AS",
            "  CTRL+E                      EXPORT (PNG, GIF, CP, SVG INTO DESIGNS/)",
            "  CTRL+Z  CTRL+Y              UNDO, REDO          TAB  SWITCH TABS",
            "  CTRL+ =  CTRL+ -            BIGGER / SMALLER UI          ESC  CLOSE A DIALOG",
            "  DRAG A .ORI OR .CP FILE ONTO THE WINDOW TO OPEN IT",
        };
        int y = in.y + 14;
        for (const char* l : lines) {
            const bool head = l[0] != ' ' && l[0] != '\0';
            ui_.label(in.x + 16, y, l, head ? ui::Accent : ui::Text);
            y += head ? 16 : 14;
        }
        ui_.label(in.x + 16, in.bottom() - 30, "UI SCALE " + std::to_string(uiScale) + "X", ui::Muted);
        if (ui_.button(Rect{in.right() - 100, in.bottom() - 40, 90, 28}, "CLOSE")) overlay = Overlay::None;
        break;
    }
    case Overlay::None:
        break;
    }
}

} // namespace ori
