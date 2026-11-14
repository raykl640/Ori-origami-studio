// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : the interactive studio, driven like a user would drive it
//  Owner : Ray Lukorito (App shell)
//  A Driver feeds mouse moves, drags, clicks on named buttons, key presses
//  and typing into Studio::frame() - the same path the SDL window uses.
// ============================================================================
#include "TestFramework.h"
#include "../studio/Studio.h"
#include <cmath>
#include <filesystem>
#include <fstream>

using namespace ori;
namespace fs = std::filesystem;

namespace {

struct Driver {
    Studio s;
    Image img{1280, 800};
    UiInput in;

    explicit Driver(const std::string& dir) {
        s.designsDir = dir;
        frame();
    }
    void frame(double dt = 0.016) {
        s.frame(img, in, dt);
        in.clearEvents();
    }
    void move(const Vec2& p) {
        in.mouseX = int(std::lround(p.x));
        in.mouseY = int(std::lround(p.y));
        frame();
    }
    void press(const Vec2& p) {
        in.mouseX = int(std::lround(p.x));
        in.mouseY = int(std::lround(p.y));
        in.mouseDown = true;
        in.pressed = true;
        frame();
    }
    void release(const Vec2& p) {
        in.mouseX = int(std::lround(p.x));
        in.mouseY = int(std::lround(p.y));
        in.mouseDown = false;
        in.released = true;
        frame();
    }
    void click(const Vec2& p) { press(p); release(p); }
    void drag(const Vec2& a, const Vec2& b, bool shift = false) {
        in.shift = shift;
        press(a);
        move(lerp(a, b, 0.5));
        move(b);
        release(b);
        in.shift = false;
    }
    bool clickButton(const std::string& label) {
        const auto& b = s.ui().buttons();
        const auto it = b.find(label);
        if (it == b.end()) return false;
        click({it->second.x + it->second.w / 2.0, it->second.y + it->second.h / 2.0});
        return true;
    }
    void key(Key k, char ch = 0, bool ctrl = false, bool shift = false) {
        KeyPress kp;
        kp.key = k; kp.ch = ch; kp.ctrl = ctrl; kp.shift = shift;
        in.keys.push_back(kp);
        frame();
    }
    void ch(char c, bool ctrl = false, bool shift = false) { key(Key::Char, c, ctrl, shift); }
    void type(const std::string& t) { in.text = t; frame(); }
    // screen position of a model point in the fold preview / crease canvas
    Vec2 fold(double x, double y) const { return s.previewView.apply(Vec2{x, y}); }
    Vec2 cp(double x, double y) const { return s.canvasView.apply(Vec2{x, y}); }
};

std::string tempDir(const std::string& name) {
    const fs::path p = fs::temp_directory_path() / ("ori_studio_test_" + name);
    std::error_code ec;
    fs::remove_all(p, ec);
    return p.string();
}

} // namespace

TEST(studio_starts_with_the_dog_and_draws_every_region) {
    Driver d(tempDir("start"));
    CHECK(d.s.tab == Tab::Fold && d.s.stepCount() == 4);
    CHECK_NEAR(d.s.timeline, 4.0, 1e-12);
    CHECK(!d.s.foldDirty() && !d.s.cpDirty());
    CHECK(d.s.windowTitle().find("dog_face") != std::string::npos);
    for (const char* b : {"NEW", "OPEN", "SAVE", "SAVE AS", "EXPORT", "HELP", "PLAY", "FOLD SEQUENCE", "CREASE PATTERN", "DIAGRAM (D)"})
        CHECK(d.s.ui().buttons().count(b) == 1);
    // the preview really shows paper: the model centre is not background
    const Vec2 c = d.fold(200, 260);
    CHECK(!d.img.getPixel(int(c.x), int(c.y)).sameRGB(ui::Panel));
}

TEST(studio_drawing_a_fold_adds_a_step_with_undo_and_redo) {
    Driver d(tempDir("draw"));
    d.s.newDesign(0);                                   // blank square
    d.frame();
    CHECK(d.s.stepCount() == 0);
    d.drag(d.fold(200, 400), d.fold(200, 0));          // vertical line, right half moves
    CHECK(d.s.stepCount() == 1);
    const FoldState s = d.s.states().back();
    CHECK(s.panels.size() == 2 && s.layerCount() == 2);
    CHECK(nearlyEqual(d.s.doc.seq.steps[0].a, {200, 400}, 1e-6));   // snapped to the edge midpoints
    CHECK(nearlyEqual(d.s.doc.seq.steps[0].b, {200, 0}, 1e-6));
    CHECK_NEAR(d.s.timeline, 1.0, 1e-12);
    CHECK(d.s.selectedStep == 0 && d.s.foldDirty());
    d.ch('z', true);                                   // Ctrl+Z
    CHECK(d.s.stepCount() == 0);
    d.ch('y', true);                                   // Ctrl+Y
    CHECK(d.s.stepCount() == 1);
    CHECK(d.clickButton("UNDO") && d.s.stepCount() == 0);
    CHECK(d.clickButton("REDO") && d.s.stepCount() == 1);
}

TEST(studio_fold_snapping_and_angle_lock) {
    Driver d(tempDir("snap"));
    d.s.newDesign(0);
    d.frame();
    // 4 px away from the corners: snaps exactly onto them
    d.drag(d.fold(0, 0) + Vec2{3, 4}, d.fold(400, 400) + Vec2{-4, 3});
    CHECK(d.s.stepCount() == 1);
    CHECK(nearlyEqual(d.s.doc.seq.steps[0].a, {0, 0}, 1e-6) && nearlyEqual(d.s.doc.seq.steps[0].b, {400, 400}, 1e-6));
    // Shift locks a wobbly drag to a 22.5-degree multiple
    d.s.foldSnapGrid = false;
    d.s.setTimeline(0);
    d.frame();
    const Vec2 a = d.fold(30, 210), b = d.fold(370, 223);
    d.drag(a, b, true);
    const FoldStep& st = d.s.doc.seq.steps[0];
    const double deg = radToDeg((st.b - st.a).angle());
    CHECK_NEAR(std::fmod(std::fabs(deg) + 1e-9, 22.5), 0.0, 1e-6);
}

TEST(studio_line_that_misses_the_paper_is_rejected) {
    Driver d(tempDir("miss"));
    d.s.newDesign(0);
    d.frame();
    d.s.foldSnapGrid = false;
    // a line far left of the sheet, drawn so nothing is on its moving side
    d.drag(d.fold(-30, 0) , d.fold(-30, 400));
    CHECK(d.s.stepCount() == 0);
    CHECK(d.s.status.find("MISSES") != std::string::npos);
}

TEST(studio_sequence_editing_keys_and_buttons) {
    Driver d(tempDir("edit"));
    CHECK(d.s.stepCount() == 4);
    d.key(Key::Down);                                  // select step 1
    CHECK(d.s.selectedStep == 0);
    d.ch('m');                                         // make it a mountain fold
    CHECK(d.s.doc.seq.steps[0].kind == FoldKind::Mountain);
    const Vec2 a = d.s.doc.seq.steps[0].a;
    d.ch('f');                                         // flip the moving side
    CHECK(nearlyEqual(d.s.doc.seq.steps[0].b, a));
    d.ch(']');                                         // move it down
    CHECK(d.s.selectedStep == 1 && d.s.doc.seq.steps[1].kind == FoldKind::Mountain);
    CHECK(d.clickButton("M/V") && d.s.doc.seq.steps[1].kind == FoldKind::Valley);
    CHECK(d.clickButton("DELETE") && d.s.stepCount() == 3);
    d.key(Key::Delete);
    CHECK(d.s.stepCount() == 2);
    // rename with F2 and typing
    d.key(Key::Down);
    d.key(Key::F2);
    CHECK(d.s.overlay == Overlay::Rename);
    d.key(Key::Backspace);
    d.type("Ears");
    d.key(Key::Enter);
    CHECK(d.s.overlay == Overlay::None);
    CHECK(d.s.doc.seq.steps[std::size_t(d.s.selectedStep)].label.find("Ears") != std::string::npos);
}

TEST(studio_timeline_playback_and_scrubbing) {
    Driver d(tempDir("play"));
    d.key(Key::Home);
    CHECK_NEAR(d.s.timeline, 0.0, 1e-12);
    d.key(Key::Space);
    CHECK(d.s.playing);
    for (int i = 0; i < 20; ++i) d.frame(0.1);         // 2 seconds: in the middle of fold 2
    CHECK(d.s.timeline > 1.0 && d.s.timeline < 3.0);
    for (int i = 0; i < 60; ++i) d.frame(0.1);
    CHECK(!d.s.playing);
    CHECK_NEAR(d.s.timeline, 4.0, 1e-12);
    d.key(Key::Left);
    CHECK_NEAR(d.s.timeline, 3.0, 1e-12);
    // Space at the end restarts from the flat sheet
    d.key(Key::End);
    d.key(Key::Space);
    CHECK(d.s.playing && d.s.timeline < 0.1);
}

TEST(studio_view_settings_and_layers) {
    Driver d(tempDir("view"));
    CHECK(d.clickButton("X-RAY (X)") && d.s.render.alpha < 1.0);
    d.ch('x');
    CHECK(d.s.render.alpha >= 1.0);
    d.ch('e');
    CHECK(d.s.exploded);
    d.ch('e');
    CHECK(!d.s.exploded);
    d.ch('d');
    CHECK(!d.s.showDiagram);
    d.s.hiddenLayers[3] = true;
    d.frame();
    CHECK(d.clickButton("SHOW ALL"));
    CHECK(!d.s.hiddenLayers[3]);
}

TEST(studio_crease_editor_draw_select_toggle_erase) {
    Driver d(tempDir("cp"));
    d.key(Key::Tab);
    CHECK(d.s.tab == Tab::Crease);
    d.s.newDesign(0);
    d.frame();
    d.drag(d.cp(0, 0) + Vec2{2, 3}, d.cp(400, 400) + Vec2{-3, 2});   // valley, snapped to corners
    CHECK(d.s.doc.cp.creases.size() == 1);
    CHECK(d.s.doc.cp.creases[0].type == CreaseType::Valley);
    CHECK(nearlyEqual(d.s.doc.cp.creases[0].a, {0, 0}, 1e-6) && nearlyEqual(d.s.doc.cp.creases[0].b, {400, 400}, 1e-6));
    d.ch('m');
    d.drag(d.cp(400, 0), d.cp(0, 400));
    CHECK(d.s.doc.cp.count(CreaseType::Mountain) == 1 && d.s.doc.cp.intersections().size() == 1);
    // full-length lines: a short drag becomes a whole chord of the sheet
    d.ch('l');
    d.drag(d.cp(150, 100), d.cp(250, 100));
    CHECK(d.s.doc.cp.creases.size() == 3);
    CHECK_NEAR(d.s.doc.cp.creases.back().segment().length(), 400.0, 1e-6);
    d.ch('l');
    // select + toggle + delete
    d.ch('s');
    d.click(d.cp(300, 300));                           // only the first diagonal passes here
    CHECK(d.s.selectedCrease == 0);
    d.ch('t');
    CHECK(d.s.doc.cp.creases[0].type == CreaseType::Mountain);
    d.key(Key::Delete);
    CHECK(d.s.doc.cp.creases.size() == 2);
    // erase tool
    d.ch('e');
    d.click(d.cp(50, 350));                            // only the other diagonal passes here
    CHECK(d.s.doc.cp.creases.size() == 1);
    // right-click toggles with any tool
    d.in.mouseX = int(d.cp(200, 100).x); d.in.mouseY = int(d.cp(200, 100).y);
    d.in.rightPressed = true;
    const CreaseType before = d.s.doc.cp.creases[0].type;
    d.frame();
    CHECK(d.s.doc.cp.creases[0].type != before);
    // grid size keys
    const double g = d.s.doc.cp.gridStep;
    d.ch('+');
    CHECK_NEAR(d.s.doc.cp.gridStep, g / 2, 1e-9);
}

TEST(studio_flat_foldability_checks) {
    Driver d(tempDir("checks"));
    d.s.tab = Tab::Crease;
    CreasePattern good(Polygon::rectangle(0, 0, 400, 400));
    good.addCrease({0, 0}, {400, 400}, CreaseType::Valley);
    good.addCrease({400, 0}, {0, 400}, CreaseType::Valley);
    good.addCrease({0, 200}, {400, 200}, CreaseType::Mountain);
    good.addCrease({200, 0}, {200, 200}, CreaseType::Mountain);
    good.addCrease({200, 200}, {200, 400}, CreaseType::Valley);    // square base: 5 V + 3 M
    d.s.doc.cp = good;
    auto checks = d.s.flatFoldabilityChecks();
    CHECK(checks.size() == 1 && checks[0].maekawa && checks[0].kawasaki);
    d.s.doc.cp.creases.back().type = CreaseType::Mountain;           // 4 + 4: Maekawa fails
    checks = d.s.flatFoldabilityChecks();
    CHECK(checks.size() == 1 && !checks[0].maekawa && checks[0].kawasaki);
    d.s.doc.cp.creases[0].b = {400, 300};                            // bend a diagonal: Kawasaki fails too
    d.s.doc.cp.creases[0].a = {0, 100};
    checks = d.s.flatFoldabilityChecks();
    bool anyKawasakiFail = false;
    for (const auto& c : checks) anyKawasakiFail = anyKawasakiFail || !c.kawasaki;
    CHECK(anyKawasakiFail);
}

TEST(studio_save_open_and_export_roundtrip) {
    const std::string dir = tempDir("files");
    Driver d(dir);
    d.s.newDesign(0);
    d.frame();
    d.drag(d.fold(0, 200), d.fold(400, 200));
    CHECK(d.s.stepCount() == 1);
    d.ch('s', true);                                   // Ctrl+S on an unnamed design asks for a name
    CHECK(d.s.overlay == Overlay::SaveAs);
    for (int i = 0; i < 20; ++i) d.in.keys.push_back(KeyPress{Key::Backspace});
    d.frame();
    d.type("book fold");
    d.key(Key::Enter);
    const std::string path = dir + "/book fold.ori";
    CHECK(fs::exists(path));
    CHECK(!d.s.foldDirty() && d.s.foldPath == fs::path(path).generic_string());
    // change it, then open the saved file again
    d.drag(d.fold(0, 0), d.fold(400, 200));
    CHECK(d.s.foldDirty());
    CHECK(d.s.open(path) && d.s.stepCount() == 1 && !d.s.foldDirty());
    // the open dialog lists it, and clicking the row opens it
    d.ch('o', true);
    CHECK(d.s.overlay == Overlay::Open);
    const auto files = d.s.listDesignFiles();
    bool listed = false;
    for (const auto& f : files) listed = listed || f.find("book fold.ori") != std::string::npos;
    CHECK(listed);
    d.key(Key::Escape);
    CHECK(d.s.overlay == Overlay::None);
    // export writes the diagram sheet, result, GIF and crease pattern
    CHECK(d.s.exportCurrent());
    for (const char* ext : {"_steps.png", ".png", ".gif", ".cp", ".svg"}) CHECK(fs::exists(dir + "/book fold" + ext));
    // crease tab save + export
    d.key(Key::Tab);
    d.drag(d.cp(0, 0), d.cp(400, 400));
    CHECK(d.s.saveAs("diag"));
    CHECK(fs::exists(dir + "/diag.cp") && !d.s.cpDirty());
    CHECK(d.s.exportCurrent() && fs::exists(dir + "/diag.png") && fs::exists(dir + "/diag.svg"));
    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST(studio_dialogs_are_modal_and_quit_asks_first) {
    Driver d(tempDir("modal"));
    d.key(Key::F1);
    CHECK(d.s.overlay == Overlay::Help);
    const int steps = d.s.stepCount();
    d.drag(d.fold(0, 200), d.fold(400, 200));          // ignored behind the dialog
    CHECK(d.s.stepCount() == steps);
    CHECK(d.clickButton("CLOSE") && d.s.overlay == Overlay::None);
    d.s.requestQuit();
    CHECK(d.s.shouldExit());                          // nothing unsaved: quits at once
    Driver e(tempDir("modal2"));
    e.key(Key::Home);
    e.drag(e.fold(0, 200), e.fold(400, 200));
    CHECK(e.s.foldDirty());
    e.s.requestQuit();
    CHECK(!e.s.shouldExit() && e.s.overlay == Overlay::ConfirmQuit);
    e.frame();
    CHECK(e.clickButton("CANCEL") && e.s.overlay == Overlay::None && !e.s.shouldExit());
    e.s.requestQuit();
    e.frame();
    CHECK(e.clickButton("QUIT WITHOUT SAVING") && e.s.shouldExit());
}

TEST(studio_new_dialog_and_built_in_models) {
    Driver d(tempDir("new"));
    CHECK(d.clickButton("NEW") && d.s.overlay == Overlay::New);
    CHECK(d.clickButton("KITE CONE"));
    CHECK(d.s.overlay == Overlay::None && d.s.stepCount() == 3 && !d.s.doc.seq.whiteSideUp);
    CHECK(d.clickButton("NEW") && d.clickButton("DIAMOND"));
    CHECK(d.s.stepCount() == 0 && d.s.doc.seq.paper.size() == 4);
    CHECK(d.s.undo() && d.s.stepCount() == 3);         // NEW is undoable too
    // generated crease pattern -> crease tab
    CHECK(d.clickButton("EDIT IN CP TAB"));
    CHECK(d.s.tab == Tab::Crease && d.s.doc.cp.creases.size() == 5);
}

TEST(studio_drop_a_file_onto_the_window) {
    Driver d(tempDir("drop"));
    d.in.droppedFiles.push_back("patterns/does_not_exist.cp");
    d.frame();
    CHECK(d.s.status.find("COULD NOT OPEN") != std::string::npos);
    CHECK(d.s.tab == Tab::Fold);
}
