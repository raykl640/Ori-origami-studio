// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Studio : the interactive design studio      Owner : Ray Lukorito (App shell)
//  File   : Studio.h
//
//  The whole interactive application, independent of any windowing system:
//  each frame the shell passes the input (UiInput) and an Image to draw into,
//  and Studio::frame() redraws the studio with the project's rasterizer.
//
//  The five regions from the design, split by owner:
//    Studio.cpp         Ray     frame, layout, top bar, undo/redo, shortcuts
//    StudioFiles.cpp    Edgar   open / save / save as / export / new designs
//    StudioDialogs.cpp  Edgar   the dialogs (open list, save as, help, quit ...)
//    StudioFold.cpp     Mohamed fold-sequence panel, drawing folds, generated CP
//    StudioPreview.cpp  Edgar   folded preview and layer inspector
//    StudioPalette.cpp  Matthew depth palette and the animation timeline
//    StudioCrease.cpp   Evans   the crease-pattern editor tab
//    Widgets.cpp        Mathew  the immediate-mode UI toolkit
// ============================================================================
#pragma once
#include "../src/editor/CreasePattern.h"
#include "../src/fold/FoldSequence.h"
#include "../src/layering/LayerRenderer.h"
#include "../src/math/Mat3.h"
#include "Widgets.h"
#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace ori {

enum class Tab { Fold, Crease };
enum class Overlay { None, Open, SaveAs, Rename, Help, ConfirmQuit, New };
enum class CreaseTool { Draw, Select, Erase };

// Everything that is saved / undone.
struct StudioDoc {
    FoldSequence seq;
    CreasePattern cp;
};

class Studio {
public:
    Studio();

    // ---- used by the window shell ------------------------------------------
    void frame(Image& target, const UiInput& input, double dt);
    bool open(const std::string& path);          // .ori or .cp
    void requestQuit();                          // window close button
    bool shouldExit() const { return exit_; }
    bool needsAnimation() const;                 // keep redrawing (playback, status fade)
    std::string windowTitle() const;
    const Ui& ui() const { return ui_; }

    // ---- document -------------------------------------------------------------
    StudioDoc doc;
    std::string foldPath, cpPath;                // files the two tabs were loaded from / saved to
    bool foldDirty() const { return doc.seq.toText() != savedFold_; }
    bool cpDirty() const { return doc.cp.toText() != savedCp_; }
    std::string designsDir = "designs";          // where Save As and Export write

    // ---- shared state (public so tests can drive the studio) ---------------------
    Tab tab = Tab::Fold;
    Overlay overlay = Overlay::None;
    std::string status;                          // message in the status bar
    double statusAge = 0;
    int uiScale = 1;                             // set by the shell, shown in Help

    // Fold tab
    double timeline = 0;                         // 0 = flat sheet, n = all folds done
    bool playing = false;
    int selectedStep = -1;
    FoldKind newKind = FoldKind::Valley;         // kind of the next fold drawn
    bool foldSnapGrid = true;
    bool showDiagram = true;
    bool exploded = false;
    RenderOptions render;                        // depth palette settings
    int paletteIndex = 0;
    std::vector<bool> hiddenLayers;
    int highlightLayer = -1;
    int sequenceScroll = 0, layerScroll = 0;

    // Crease tab
    CreaseTool tool = CreaseTool::Draw;
    CreaseType drawType = CreaseType::Valley;
    int selectedCrease = -1;
    bool snapVertices = true, snapGrid = true, showGrid = true, fullLines = false, showFaces = false, showChecks = true;

    // Layout and view of the last frame (the tests use them to aim clicks).
    Rect previewRect, canvasRect, sequenceRect, layerRect, paletteRect, patternRect;
    Mat3 previewView, canvasView;

    // ---- operations (also bound to buttons and keys) -----------------------------
    void checkpoint();                           // call BEFORE every edit (undo snapshot)
    bool undo();
    bool redo();
    bool save();                                 // current tab; asks for a name if needed
    bool saveAs(const std::string& name);
    bool exportCurrent();
    void newDesign(int kind);                    // 0 square, 1 diamond, 2 rectangle, 10+ built-in model
    void setStatus(const std::string& s) { status = s; statusAge = 0; }

    std::vector<FoldState> states() const { return doc.seq.run(); }
    int stepCount() const { return int(doc.seq.steps.size()); }
    Palette palette() const;
    std::vector<std::string> listDesignFiles() const;
    std::string baseName() const;                // name used for exports

    // Fold-tab operations (StudioFold.cpp)
    bool addFold(const Vec2& a, const Vec2& b, FoldKind kind, int index);
    void deleteStep(int i);
    void moveStep(int i, int delta);
    void toggleStepKind(int i);
    void flipStep(int i);
    void setTimeline(double t);

    // Crease-tab operations (StudioCrease.cpp)
    bool addCreaseFromEditor(const Vec2& a, const Vec2& b);
    void setPaper(int kind);
    struct VertexCheck { Vec2 p; bool maekawa = true, kawasaki = true; };
    std::vector<VertexCheck> flatFoldabilityChecks() const;

private:
    // Studio.cpp
    void layout(int w, int h);
    void drawTopBar(const Rect& r);
    void drawStatusBar(const Rect& r);
    void handleGlobalKeys();
    // StudioDialogs.cpp
    void drawOverlay();
    // StudioFold.cpp
    void drawSequencePanel(const Rect& r);
    void drawGeneratedPattern(const Rect& r, const std::vector<FoldState>& st);
    void handleFoldTool(const std::vector<FoldState>& st);
    void foldKey(const KeyPress& k);
    Vec2 snapFoldPoint(const Vec2& screen, const FoldState& s, bool* snapped) const;
    // StudioPreview.cpp
    void drawPreview(const Rect& r, const std::vector<FoldState>& st);
    void drawLayerPanel(const Rect& r, const std::vector<FoldState>& st);
    // StudioPalette.cpp
    void drawPalettePanel(const Rect& r, int layers);
    void drawTimeline(const Rect& r);
    void advancePlayback(double dt);
    // StudioCrease.cpp
    void drawCreaseTab(const Rect& content);
    void creaseKey(const KeyPress& k);
    Vec2 snapCreasePoint(const Vec2& screen, bool* snapped) const;

    Ui ui_;
    bool exit_ = false;
    std::string savedFold_, savedCp_;            // text of the last saved / loaded documents
    std::vector<StudioDoc> undo_, redo_;
    std::string textBuffer_;                     // Save As / Rename field
    int openScroll_ = 0;
    // in-progress drags
    Vec2 dragStart_, dragEnd_;
    bool dragSnapped_ = false;
};

// The drawing area inside the folded-preview panel (below its toolbar,
// above the timeline). Shared by the preview and the fold tool.
inline Rect previewCanvas(const Rect& panel) {
    return Rect{panel.x + 1, panel.y + 50, panel.w - 2, std::max(10, panel.h - 50 - 58)};
}

// Constrain the direction a -> b to the nearest multiple of 22.5 degrees
// (the angles of classic origami bases), keeping its length.
Vec2 constrainAngle(const Vec2& a, const Vec2& b);

} // namespace ori
