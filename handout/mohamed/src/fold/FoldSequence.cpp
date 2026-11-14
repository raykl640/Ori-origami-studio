// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : fold            Owner : Mohamed Bile (Fold transformation engine)
//  File   : FoldSequence.cpp
// ============================================================================
#include "FoldSequence.h"
#include <fstream>
#include <sstream>

namespace ori {

namespace {

std::string trim(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos) return "";
    const auto b = s.find_last_not_of(" \t\r");
    return s.substr(a, b - a + 1);
}

} // namespace

std::string FoldSequence::toText() const {
    std::ostringstream s;
    s.precision(10);
    s << "# ORI fold sequence\n";
    if (!name.empty()) s << "name " << name << "\n";
    s << "paper";
    for (const Vec2& p : paper) s << "  " << p.x << " " << p.y;
    s << "\nstart " << (whiteSideUp ? "white" : "colour") << "\n";
    for (const FoldStep& st : steps) {
        s << foldKindName(st.kind) << "  " << st.a.x << " " << st.a.y << "  " << st.b.x << " " << st.b.y;
        if (!st.label.empty()) s << "  " << st.label;
        s << "\n";
    }
    if (!decoration.empty()) s << "decorate " << decoration << "\n";
    return s.str();
}

bool FoldSequence::fromText(const std::string& text, FoldSequence& out, std::string* error) {
    FoldSequence seq;
    std::istringstream in(text);
    std::string line;
    int lineNo = 0;
    auto fail = [&](const std::string& msg) {
        if (error) *error = "line " + std::to_string(lineNo) + ": " + msg;
        return false;
    };
    while (std::getline(in, line)) {
        ++lineNo;
        const auto hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);
        std::istringstream ls(line);
        std::string key;
        if (!(ls >> key)) continue;
        if (key == "name") {
            std::string rest;
            std::getline(ls, rest);
            seq.name = trim(rest);
        } else if (key == "paper") {
            double x, y;
            while (ls >> x >> y) seq.paper.push_back({x, y});
            if (seq.paper.size() < 3) return fail("paper needs at least 3 points");
        } else if (key == "start") {
            std::string side;
            ls >> side;
            if (side == "white") seq.whiteSideUp = true;
            else if (side == "colour" || side == "color") seq.whiteSideUp = false;
            else return fail("start must be 'white' or 'colour'");
        } else if (key == "valley" || key == "mountain" || key == "V" || key == "M") {
            FoldStep st;
            st.kind = (key == "valley" || key == "V") ? FoldKind::Valley : FoldKind::Mountain;
            if (!(ls >> st.a.x >> st.a.y >> st.b.x >> st.b.y)) return fail("a fold needs 4 numbers: ax ay bx by");
            if (distance(st.a, st.b) < 1e-9) return fail("fold line end points are the same");
            std::string rest;
            std::getline(ls, rest);
            st.label = trim(rest);
            if (st.label.empty()) st.label = std::string(foldKindName(st.kind)) + " fold " + std::to_string(seq.steps.size() + 1);
            seq.steps.push_back(st);
        } else if (key == "decorate") {
            ls >> seq.decoration;
        } else {
            return fail("unknown keyword '" + key + "'");
        }
    }
    if (seq.paper.size() < 3) return fail("missing 'paper' line");
    out = seq;
    return true;
}

bool FoldSequence::save(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f << toText();
    return bool(f);
}

bool FoldSequence::load(const std::string& path, FoldSequence& out, std::string* error) {
    std::ifstream f(path);
    if (!f) {
        if (error) *error = "cannot open " + path;
        return false;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return fromText(ss.str(), out, error);
}

} // namespace ori
