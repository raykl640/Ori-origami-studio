// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : editor          Owner : Evans Mwendwa (Crease pattern editor)
//  File   : CreasePattern.h
//  The crease-pattern model behind the editor: a paper outline plus straight
//  mountain / valley creases. It supports what an editor needs - adding a
//  crease (trimmed to the paper with Cyrus-Beck), toggling M/V, deleting,
//  finding the crease under the cursor, vertex-then-grid snapping, and the
//  vertices where creases meet (segment / segment intersection).
//
//  File formats:
//    .cp   plain text, one crease per line - see toText() for the grammar.
//          Lines in ORIPA's numeric form "type x1 y1 x2 y2" (2 = mountain,
//          3 = valley) are also read, so patterns can be swapped with it.
//    .svg  print-ready vector export (toSVG()).
// ============================================================================
#pragma once
#include "../geometry/Line.h"
#include "../geometry/Polygon.h"
#include "../math/Vec2.h"
#include <string>
#include <vector>

namespace ori {

enum class CreaseType { Mountain, Valley, Aux };

const char* creaseTypeName(CreaseType t);

struct Crease {
    Vec2 a, b;
    CreaseType type = CreaseType::Valley;
    Segment segment() const { return {a, b}; }
};

class CreasePattern {
public:
    Polygon paper;
    std::vector<Crease> creases;
    std::string name;
    double gridStep = 0.0;        // editor grid spacing (0 = none)

    CreasePattern() : paper(Polygon::rectangle(0, 0, 400, 400)) {}
    explicit CreasePattern(Polygon sheet, std::string title = "") : paper(std::move(sheet)), name(std::move(title)) {}

    // Add a crease. With clipToPaper it is trimmed to the (convex) paper and
    // rejected when it misses the paper or is shorter than minLength.
    bool addCrease(const Vec2& a, const Vec2& b, CreaseType t, bool clipToPaper = true, double minLength = 1e-6);
    // Add the chord of the paper along the infinite line through a and b.
    bool addFullCrease(const Vec2& a, const Vec2& b, CreaseType t);
    bool removeCrease(std::size_t index);
    // Mountain <-> valley (aux lines are left alone).
    void toggle(std::size_t index);

    // Index of the crease nearest to p within maxDist, or -1.
    int creaseAt(const Vec2& p, double maxDist) const;
    int count(CreaseType t) const;

    // Paper corners, crease end points and crease / crease intersections, deduplicated.
    std::vector<Vec2> vertices() const;
    std::vector<Vec2> intersections() const;

    // Snap a cursor position: to the nearest vertex within vertexRadius,
    // otherwise to the nearest grid point (gridStep > 0), otherwise unchanged.
    Vec2 snap(const Vec2& p, double vertexRadius, double grid) const;
    Vec2 snap(const Vec2& p, double vertexRadius) const { return snap(p, vertexRadius, gridStep); }

    // Split every crease where another crease crosses it (planar graph edges).
    CreasePattern split() const;

    // ---- file formats -----------------------------------------------------
    std::string toText() const;
    // Parse .cp text. Returns false and fills `error` (with a line number) on bad input.
    static bool fromText(const std::string& text, CreasePattern& out, std::string* error = nullptr);
    bool save(const std::string& path) const;
    static bool load(const std::string& path, CreasePattern& out, std::string* error = nullptr);

    // Print-ready SVG, `pixelsPerUnit` scales the coordinates.
    std::string toSVG(double pixelsPerUnit = 1.0, double margin = 20.0) const;
    bool saveSVG(const std::string& path, double pixelsPerUnit = 1.0) const;
};

} // namespace ori
