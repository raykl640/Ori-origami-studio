// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : editor          Owner : Evans Mwendwa (Crease pattern editor)
//  File   : CreasePattern.cpp
// ============================================================================
#include "CreasePattern.h"
#include "../raster/Clipping.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace ori {

const char* creaseTypeName(CreaseType t) {
    switch (t) {
        case CreaseType::Mountain: return "mountain";
        case CreaseType::Valley:   return "valley";
        default:                   return "aux";
    }
}

// ------------------------------------------------------------- editing --
bool CreasePattern::addCrease(const Vec2& a, const Vec2& b, CreaseType t, bool clipToPaper, double minLength) {
    Vec2 p = a, q = b;
    if (clipToPaper && paper.size() >= 3) {
        if (paper.isConvex()) {
            if (!clipCyrusBeck(p, q, paper.v)) return false;
        } else if (!paper.contains((a + b) * 0.5)) {
            return false;   // non-convex paper: keep creases whose midpoint is on it
        }
    }
    if (distance(p, q) < minLength) return false;
    creases.push_back({p, q, t});
    return true;
}

bool CreasePattern::addFullCrease(const Vec2& a, const Vec2& b, CreaseType t) {
    Vec2 p, q;
    if (!clipLineToConvex(a, b, paper.v, p, q)) return false;
    creases.push_back({p, q, t});
    return true;
}

bool CreasePattern::removeCrease(std::size_t index) {
    if (index >= creases.size()) return false;
    creases.erase(creases.begin() + std::ptrdiff_t(index));
    return true;
}

void CreasePattern::toggle(std::size_t index) {
    if (index >= creases.size()) return;
    CreaseType& t = creases[index].type;
    if (t == CreaseType::Mountain) t = CreaseType::Valley;
    else if (t == CreaseType::Valley) t = CreaseType::Mountain;
}

int CreasePattern::creaseAt(const Vec2& p, double maxDist) const {
    int best = -1;
    double bestD = maxDist;
    for (std::size_t i = 0; i < creases.size(); ++i) {
        const double d = distanceToSegment(p, creases[i].segment());
        if (d <= bestD) { bestD = d; best = int(i); }
    }
    return best;
}

int CreasePattern::count(CreaseType t) const {
    return int(std::count_if(creases.begin(), creases.end(), [t](const Crease& c) { return c.type == t; }));
}

// ------------------------------------------------------------ vertices --
namespace {

void addUnique(std::vector<Vec2>& pts, const Vec2& p, double eps = 1e-6) {
    for (const Vec2& q : pts)
        if (nearlyEqual(p, q, eps)) return;
    pts.push_back(p);
}

} // namespace

std::vector<Vec2> CreasePattern::intersections() const {
    std::vector<Vec2> out;
    for (std::size_t i = 0; i < creases.size(); ++i)
        for (std::size_t j = i + 1; j < creases.size(); ++j)
            if (auto p = intersect(creases[i].segment(), creases[j].segment())) addUnique(out, *p);
    return out;
}

std::vector<Vec2> CreasePattern::vertices() const {
    std::vector<Vec2> out;
    for (const Vec2& p : paper.v) addUnique(out, p);
    for (const Crease& c : creases) { addUnique(out, c.a); addUnique(out, c.b); }
    for (const Vec2& p : intersections()) addUnique(out, p);
    return out;
}

Vec2 CreasePattern::snap(const Vec2& p, double vertexRadius, double grid) const {
    // 1. vertices win: creases should meet exactly.
    double best = vertexRadius;
    const Vec2* hit = nullptr;
    const auto verts = vertices();
    for (const Vec2& v : verts) {
        const double d = distance(p, v);
        if (d <= best) { best = d; hit = &v; }
    }
    if (hit) return *hit;
    // 2. otherwise the grid, measured from the paper's bounding-box corner.
    if (grid > 0) {
        const Vec2 o = paper.size() ? paper.bounds().min : Vec2{};
        return {o.x + std::round((p.x - o.x) / grid) * grid, o.y + std::round((p.y - o.y) / grid) * grid};
    }
    return p;
}

CreasePattern CreasePattern::split() const {
    CreasePattern out(paper, name);
    out.gridStep = gridStep;
    for (std::size_t i = 0; i < creases.size(); ++i) {
        const Crease& c = creases[i];
        const Vec2 d = c.b - c.a;
        const double len2 = d.lengthSq();
        std::vector<double> ts = {0.0, 1.0};
        for (std::size_t j = 0; j < creases.size(); ++j) {
            if (i == j) continue;
            if (auto p = intersect(c.segment(), creases[j].segment())) {
                const double t = dot(*p - c.a, d) / len2;
                if (t > 1e-6 && t < 1 - 1e-6) ts.push_back(t);
            }
        }
        std::sort(ts.begin(), ts.end());
        for (std::size_t k = 0; k + 1 < ts.size(); ++k)
            if (ts[k + 1] - ts[k] > 1e-9) out.creases.push_back({c.a + d * ts[k], c.a + d * ts[k + 1], c.type});
    }
    return out;
}

// ---------------------------------------------------------------- .cp --
std::string CreasePattern::toText() const {
    std::ostringstream s;
    s.precision(10);
    s << "# ORI crease pattern\n"
         "# paper x y x y ...      paper outline (in order)\n"
         "# M|V|A x1 y1 x2 y2      mountain / valley / auxiliary crease\n";
    if (!name.empty()) s << "name " << name << "\n";
    if (gridStep > 0) s << "grid " << gridStep << "\n";
    s << "paper";
    for (const Vec2& p : paper.v) s << " " << p.x << " " << p.y;
    s << "\n";
    for (const Crease& c : creases) {
        const char t = c.type == CreaseType::Mountain ? 'M' : c.type == CreaseType::Valley ? 'V' : 'A';
        s << t << " " << c.a.x << " " << c.a.y << " " << c.b.x << " " << c.b.y << "\n";
    }
    return s.str();
}

bool CreasePattern::fromText(const std::string& text, CreasePattern& out, std::string* error) {
    CreasePattern cp;
    cp.paper = Polygon();
    std::istringstream in(text);
    std::string line;
    int lineNo = 0;
    std::vector<Vec2> borderPts;   // ORIPA type-1 lines, if no explicit paper
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
            std::getline(ls, cp.name);
            cp.name.erase(0, cp.name.find_first_not_of(' '));
        } else if (key == "grid") {
            if (!(ls >> cp.gridStep) || cp.gridStep < 0) return fail("bad grid step");
        } else if (key == "paper") {
            double x, y;
            while (ls >> x >> y) cp.paper.v.push_back({x, y});
            if (cp.paper.size() < 3) return fail("paper needs at least 3 points");
        } else {
            CreaseType t;
            bool border = false;
            if (key == "M" || key == "m" || key == "2") t = CreaseType::Mountain;
            else if (key == "V" || key == "v" || key == "3") t = CreaseType::Valley;
            else if (key == "A" || key == "a" || key == "4") t = CreaseType::Aux;
            else if (key == "1") { t = CreaseType::Aux; border = true; }
            else return fail("unknown keyword '" + key + "'");
            double x1, y1, x2, y2;
            if (!(ls >> x1 >> y1 >> x2 >> y2)) return fail("a crease needs 4 numbers");
            if (border) { addUnique(borderPts, {x1, y1}); addUnique(borderPts, {x2, y2}); continue; }
            cp.creases.push_back({{x1, y1}, {x2, y2}, t});
        }
    }
    if (cp.paper.size() < 3) {
        if (borderPts.size() >= 3) cp.paper = convexHull(borderPts);
        else return fail("no paper outline");
    }
    out = cp;
    return true;
}

bool CreasePattern::save(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f << toText();
    return bool(f);
}

bool CreasePattern::load(const std::string& path, CreasePattern& out, std::string* error) {
    std::ifstream f(path);
    if (!f) {
        if (error) *error = "cannot open " + path;
        return false;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return fromText(ss.str(), out, error);
}

// --------------------------------------------------------------- .svg --
std::string CreasePattern::toSVG(double k, double margin) const {
    const BBox b = paper.bounds();
    const double w = b.width() * k + 2 * margin, h = b.height() * k + 2 * margin;
    auto X = [&](double x) { return (x - b.min.x) * k + margin; };
    auto Y = [&](double y) { return (y - b.min.y) * k + margin; };
    std::ostringstream s;
    s.precision(6);
    s << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << w << "\" height=\"" << h
      << "\" viewBox=\"0 0 " << w << " " << h << "\">\n"
      << "  <title>" << (name.empty() ? "ORI crease pattern" : name) << "</title>\n"
      << "  <desc>Mountain: red dash-dot. Valley: blue dashed. Generated by ORI.</desc>\n"
      << "  <rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>\n  <polygon points=\"";
    for (const Vec2& p : paper.v) s << X(p.x) << "," << Y(p.y) << " ";
    s << "\" fill=\"#fbf7ee\" stroke=\"#2b2b33\" stroke-width=\"2\"/>\n";
    for (const Crease& c : creases) {
        const char* colour = c.type == CreaseType::Mountain ? "#d62828" : c.type == CreaseType::Valley ? "#1f6fd1" : "#9a9a9a";
        const char* dash = c.type == CreaseType::Mountain ? "10 4 2 4" : c.type == CreaseType::Valley ? "8 5" : "2 4";
        s << "  <line class=\"" << creaseTypeName(c.type) << "\" x1=\"" << X(c.a.x) << "\" y1=\"" << Y(c.a.y)
          << "\" x2=\"" << X(c.b.x) << "\" y2=\"" << Y(c.b.y) << "\" stroke=\"" << colour
          << "\" stroke-width=\"1.6\" stroke-dasharray=\"" << dash << "\" stroke-linecap=\"round\"/>\n";
    }
    s << "</svg>\n";
    return s.str();
}

bool CreasePattern::saveSVG(const std::string& path, double pixelsPerUnit) const {
    std::ofstream f(path);
    if (!f) return false;
    f << toSVG(pixelsPerUnit);
    return bool(f);
}

} // namespace ori
