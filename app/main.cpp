// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  App : the `ori` command-line program       Owner : Ray Lukorito (App shell)
//
//    ori                          render every evidence sheet into output/
//    ori list                     list the demos and their owners
//    ori lines curves dog         render selected demos only
//    ori --format bmp             BMP (or ppm) instead of PNG
//    ori --out renders            choose the output folder
//    ori render in.cp out.png     render a crease-pattern file
//    ori svg in.cp out.svg        crease pattern -> print-ready SVG
//    ori fold in.ori              fold a sequence file: diagram sheet, result,
//                                 animated GIF, generated .cp and .svg
//    ori model dog out.ori        save a built-in model as a sequence file
// ============================================================================
#include "../demos/Demos.h"
#include "../src/editor/CreaseRenderer.h"
#include "../src/fold/FoldSequence.h"
#include "../src/layering/LayerRenderer.h"
#include "../src/models/Models.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace ori;
namespace fs = std::filesystem;

namespace {

void usage() {
    std::cout <<
        "ORI - procedural origami folding studio (pure C++17, no graphics library)\n\n"
        "usage:\n"
        "  ori [demo ...] [--out DIR] [--format png|bmp|ppm]\n"
        "        render evidence sheets (all of them when no demo is named)\n"
        "  ori list                       list demos and owners\n"
        "  ori render FILE.cp OUT.png [--size N]\n"
        "                                 render a crease pattern\n"
        "  ori svg FILE.cp OUT.svg        export a crease pattern as SVG\n"
        "  ori fold FILE.ori [--out DIR] [--size N] [--no-gif]\n"
        "                                 fold a sequence file and export the diagram\n"
        "                                 sheet, the result, a GIF and the crease pattern\n"
        "  ori model dog|kite|napkin OUT.ori\n"
        "                                 save a built-in model as a sequence file\n"
        "  ori help\n";
}

bool ensureDir(const std::string& dir) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) std::cerr << "cannot create " << dir << ": " << ec.message() << "\n";
    return !ec;
}

std::string stem(const std::string& path) { return fs::path(path).stem().string(); }

// ---------------------------------------------------------------- commands --
int cmdList() {
    std::printf("%-12s %-26s %-34s %s\n", "NAME", "FILE", "TITLE", "OWNER");
    for (const DemoInfo& d : demoRegistry())
        std::printf("%-12s %-26s %-34s %s\n", d.key, d.file, d.title, d.owner);
    return 0;
}

int cmdRender(const std::string& in, const std::string& out, int size) {
    CreasePattern cp;
    std::string err;
    if (!CreasePattern::load(in, cp, &err)) { std::cerr << in << ": " << err << "\n"; return 1; }
    Image img(size, size, theme::Background);
    const Mat3 view = fitView(cp.paper.bounds(), Rect{0, 0, size, size - 24}, size / 16);
    drawCreasePattern(img, cp, view, Palette::classic());
    drawCreaseLegend(img, 12, size - 16, Palette::classic());
    if (!img.save(out)) { std::cerr << "cannot write " << out << "\n"; return 1; }
    std::cout << out << "  (" << cp.creases.size() << " creases: " << cp.count(CreaseType::Mountain)
              << " mountain, " << cp.count(CreaseType::Valley) << " valley)\n";
    return 0;
}

int cmdSvg(const std::string& in, const std::string& out) {
    CreasePattern cp;
    std::string err;
    if (!CreasePattern::load(in, cp, &err)) { std::cerr << in << ": " << err << "\n"; return 1; }
    if (!cp.saveSVG(out, 1.5)) { std::cerr << "cannot write " << out << "\n"; return 1; }
    std::cout << out << "\n";
    return 0;
}

int cmdFold(const std::string& in, const std::string& outDir, const std::string& format, int size, bool gif) {
    FoldSequence seq;
    std::string err;
    if (!FoldSequence::load(in, seq, &err)) { std::cerr << in << ": " << err << "\n"; return 1; }
    if (!ensureDir(outDir)) return 1;
    const std::string base = stem(in);
    const OrigamiModel m = OrigamiModel::fromSequence(seq, base);
    const auto states = m.run();

    const std::string cpPath = outDir + "/" + base + ".cp", svgPath = outDir + "/" + base + ".svg";
    const CreasePattern cp = creasePatternOf(states.back(), m.name);
    cp.save(cpPath);
    cp.saveSVG(svgPath, 1.5);

    std::vector<std::string> written = {cpPath, svgPath};
    const std::string sheet = outDir + "/" + base + "_steps." + format;
    foldSequenceSheet(m, "FOLD SEQUENCE: " + m.name, std::to_string(m.steps.size()) + " FOLDS FROM " + in,
                      {base + ".cp", base + ".svg"}).save(sheet);
    const std::string result = outDir + "/" + base + "." + format;
    foldResultImage(m, size).save(result);
    written.push_back(sheet);
    written.push_back(result);
    if (gif) {
        const std::string g = outDir + "/" + base + ".gif";
        if (writeFoldGif(m, g, std::min(size, 480), 12)) written.push_back(g);
    }
    const FoldState& s = states.back();
    std::cout << m.name << ": " << m.steps.size() << " folds -> " << s.panels.size() << " panels, "
              << s.layerCount() << " layers, " << cp.count(CreaseType::Mountain) << " mountain + "
              << cp.count(CreaseType::Valley) << " valley crease segments\n";
    for (const std::string& w : written) std::cout << "  " << w << "\n";
    return 0;
}

int cmdModel(const std::string& key, const std::string& out) {
    OrigamiModel m;
    if (!findModel(key, m)) { std::cerr << "unknown model '" << key << "' (dog, kite, napkin)\n"; return 1; }
    if (!m.toSequence().save(out)) { std::cerr << "cannot write " << out << "\n"; return 1; }
    std::cout << out << "\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    DemoContext ctx;
    int size = 600;
    bool gif = true;
    std::vector<std::string> positional;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        auto value = [&](const char* flag) -> std::string {
            if (i + 1 >= args.size()) { std::cerr << flag << " needs a value\n"; std::exit(2); }
            return args[++i];
        };
        if (a == "--out" || a == "-o") ctx.outDir = value("--out");
        else if (a == "--format" || a == "-f") ctx.format = value("--format");
        else if (a == "--size") size = std::max(64, std::atoi(value("--size").c_str()));
        else if (a == "--no-gif") gif = false;
        else if (a == "-h" || a == "--help" || a == "help") { usage(); return 0; }
        else if (!a.empty() && a[0] == '-') { std::cerr << "unknown option " << a << "\n"; usage(); return 2; }
        else positional.push_back(a);
    }
    if (ctx.format != "png" && ctx.format != "bmp" && ctx.format != "ppm") {
        std::cerr << "format must be png, bmp or ppm\n";
        return 2;
    }

    if (!positional.empty()) {
        const std::string& cmd = positional[0];
        const std::size_t n = positional.size();
        if (cmd == "list") return cmdList();
        if (cmd == "render") {
            if (n != 3) { usage(); return 2; }
            return cmdRender(positional[1], positional[2], size);
        }
        if (cmd == "svg") {
            if (n != 3) { usage(); return 2; }
            return cmdSvg(positional[1], positional[2]);
        }
        if (cmd == "fold") {
            if (n != 2) { usage(); return 2; }
            return cmdFold(positional[1], ctx.outDir, ctx.format, size, gif);
        }
        if (cmd == "model") {
            if (n != 3) { usage(); return 2; }
            return cmdModel(positional[1], positional[2]);
        }
    }

    // Demo mode: every named demo (or all of them).
    std::vector<const DemoInfo*> chosen;
    for (const std::string& p : positional) {
        const DemoInfo* found = nullptr;
        for (const DemoInfo& d : demoRegistry())
            if (p == d.key || p == d.file) found = &d;
        if (!found) { std::cerr << "unknown demo '" << p << "' - try `ori list`\n"; return 2; }
        chosen.push_back(found);
    }
    if (chosen.empty())
        for (const DemoInfo& d : demoRegistry()) chosen.push_back(&d);

    if (!ensureDir(ctx.outDir)) return 1;
    const auto t0 = std::chrono::steady_clock::now();
    int files = 0;
    for (const DemoInfo* d : chosen) {
        const auto written = d->run(ctx);
        for (const std::string& w : written) std::cout << "  " << w << "\n";
        files += int(written.size());
    }
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("%d sheet(s), %d file(s) written to %s/ in %.2f s\n", int(chosen.size()), files, ctx.outDir.c_str(), secs);
    return 0;
}
