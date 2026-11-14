// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Studio : open, save, save as, export, new designs   Owner : Edgar Kareithi (Studio layout)
//  File   : StudioFiles.cpp
// ============================================================================
#include "Studio.h"
#include "../demos/Demos.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/models/Models.h"
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

std::string lower(std::string s) {
    for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string extensionOf(const std::string& path) { return lower(fs::path(path).extension().string()); }

} // namespace

std::string Studio::baseName() const {
    const std::string& path = tab == Tab::Fold ? foldPath : cpPath;
    if (!path.empty()) return fs::path(path).stem().string();
    std::string n = tab == Tab::Fold ? doc.seq.name : doc.cp.name;
    if (n.empty()) n = "untitled";
    for (char& c : n) c = (std::isalnum(static_cast<unsigned char>(c)) ? char(std::tolower(static_cast<unsigned char>(c))) : '_');
    return n;
}

// ------------------------------------------------------------------ files --
bool Studio::open(const std::string& path) {
    const std::string ext = extensionOf(path);
    std::string err;
    if (ext == ".ori") {
        FoldSequence seq;
        if (!FoldSequence::load(path, seq, &err)) { setStatus("COULD NOT OPEN " + path + ": " + err); return false; }
        checkpoint();
        doc.seq = seq;
        foldPath = path;
        savedFold_ = doc.seq.toText();
        tab = Tab::Fold;
        selectedStep = -1;
        highlightLayer = -1;
        hiddenLayers.clear();
        playing = false;
        timeline = stepCount();
    } else if (ext == ".cp") {
        CreasePattern cp;
        if (!CreasePattern::load(path, cp, &err)) { setStatus("COULD NOT OPEN " + path + ": " + err); return false; }
        checkpoint();
        doc.cp = cp;
        if (doc.cp.gridStep <= 0) doc.cp.gridStep = std::max(doc.cp.paper.bounds().width(), doc.cp.paper.bounds().height()) / 8;
        cpPath = path;
        savedCp_ = doc.cp.toText();
        tab = Tab::Crease;
        selectedCrease = -1;
    } else {
        setStatus("ORI STUDIO OPENS .ORI FOLD SEQUENCES AND .CP CREASE PATTERNS");
        return false;
    }
    setStatus("OPENED " + path);
    return true;
}

bool Studio::save() {
    const std::string& path = tab == Tab::Fold ? foldPath : cpPath;
    if (path.empty()) {
        textBuffer_ = baseName();
        overlay = Overlay::SaveAs;
        return false;
    }
    const bool ok = tab == Tab::Fold ? doc.seq.save(path) : doc.cp.save(path);
    if (!ok) { setStatus("COULD NOT WRITE " + path); return false; }
    if (tab == Tab::Fold) savedFold_ = doc.seq.toText(); else savedCp_ = doc.cp.toText();
    setStatus("SAVED " + path);
    return true;
}

bool Studio::saveAs(const std::string& rawName) {
    std::string name = rawName;
    name.erase(0, name.find_first_not_of(' '));
    while (!name.empty() && name.back() == ' ') name.pop_back();
    if (name.empty()) { setStatus("TYPE A NAME FIRST"); return false; }
    const std::string ext = tab == Tab::Fold ? ".ori" : ".cp";
    fs::path p(name);
    if (!p.has_parent_path()) p = fs::path(designsDir) / p;
    if (lower(p.extension().string()) != ext) p += ext;
    std::error_code ec;
    if (p.has_parent_path()) fs::create_directories(p.parent_path(), ec);
    (tab == Tab::Fold ? foldPath : cpPath) = p.generic_string();
    if (tab == Tab::Fold && (doc.seq.name.empty() || doc.seq.name == "Untitled")) doc.seq.name = p.stem().string();
    if (tab == Tab::Crease && (doc.cp.name.empty() || doc.cp.name == "Untitled pattern")) doc.cp.name = p.stem().string();
    return save();
}

bool Studio::exportCurrent() {
    std::error_code ec;
    fs::create_directories(designsDir, ec);
    const std::string base = baseName();
    const std::string dir = designsDir + "/";
    if (tab == Tab::Fold) {
        const OrigamiModel m = OrigamiModel::fromSequence(doc.seq, base);
        const auto st = m.run();
        const CreasePattern cp = creasePatternOf(st.back(), m.name);
        bool ok = cp.save(dir + base + ".cp") && cp.saveSVG(dir + base + ".svg", 1.5);
        ok = foldSequenceSheet(m, "FOLD SEQUENCE: " + upper(m.name), std::to_string(m.steps.size()) + " FOLDS - EXPORTED FROM ORI STUDIO",
                               {base + ".cp", base + ".svg"}).save(dir + base + "_steps.png") && ok;
        ok = foldResultImage(m, 600).save(dir + base + ".png") && ok;
        if (!m.steps.empty()) ok = writeFoldGif(m, dir + base + ".gif", 420, 12) && ok;
        setStatus(ok ? "EXPORTED " + dir + base + "_STEPS.PNG, .PNG, .GIF, .CP AND .SVG" : "EXPORT FAILED - CHECK " + dir);
        return ok;
    }
    const int size = 900;
    Image img(size, size, theme::Background);
    const Mat3 view = fitView(doc.cp.paper.bounds(), Rect{0, 0, size, size - 30}, 40);
    CreaseViewOptions o;
    o.showGrid = false;
    drawCreasePattern(img, doc.cp, view, palette(), o);
    drawCreaseLegend(img, 20, size - 22, palette());
    const bool ok = img.save(dir + base + ".png") && doc.cp.saveSVG(dir + base + ".svg", 1.5);
    setStatus(ok ? "EXPORTED " + dir + base + ".PNG AND .SVG" : "EXPORT FAILED - CHECK " + dir);
    return ok;
}

void Studio::newDesign(int kind) {
    checkpoint();
    if (kind >= 10) {
        const std::vector<OrigamiModel> models = allModels();
        const OrigamiModel& m = models[std::size_t(std::clamp(kind - 10, 0, int(models.size()) - 1))];
        doc.seq = m.toSequence();
        foldPath.clear();
        savedFold_.clear();
        tab = Tab::Fold;
        timeline = stepCount();
        selectedStep = -1;
        hiddenLayers.clear();
        setStatus("STARTED FROM THE " + upper(m.name) + " - EDIT IT AND SAVE IT UNDER A NEW NAME");
        return;
    }
    if (tab == Tab::Fold) {
        FoldSequence s;
        s.name = "Untitled";
        s.paper = kind == 1 ? std::vector<Vec2>{{200, 0}, {400, 200}, {200, 400}, {0, 200}}
                : kind == 2 ? Polygon::rectangle(0, 0, 400, 200).v
                            : Polygon::rectangle(0, 0, 400, 400).v;
        doc.seq = s;
        foldPath.clear();
        savedFold_.clear();
        timeline = 0;
        selectedStep = -1;
        hiddenLayers.clear();
        setStatus("NEW SHEET - DRAG ACROSS IT TO MAKE THE FIRST FOLD");
    } else {
        doc.cp.creases.clear();
        doc.cp.name = "Untitled pattern";
        setPaper(kind);
        cpPath.clear();
        savedCp_.clear();
        selectedCrease = -1;
        setStatus("NEW CREASE PATTERN - DRAG TO DRAW CREASES");
    }
}

std::vector<std::string> Studio::listDesignFiles() const {
    std::set<std::string> dirs = {"patterns", designsDir, "."};
    for (const std::string& p : {foldPath, cpPath})
        if (!p.empty()) dirs.insert(fs::path(p).parent_path().empty() ? "." : fs::path(p).parent_path().string());
    std::set<std::string> seen;
    std::vector<std::string> out;
    for (const std::string& d : dirs) {
        std::error_code ec;
        if (!fs::is_directory(d, ec)) continue;
        for (const auto& e : fs::directory_iterator(d, ec)) {
            if (!e.is_regular_file()) continue;
            const std::string ext = extensionOf(e.path().string());
            if (ext != ".ori" && ext != ".cp") continue;
            const std::string canon = fs::weakly_canonical(e.path(), ec).string();
            if (!seen.insert(canon).second) continue;
            out.push_back(e.path().lexically_normal().generic_string());
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace ori
