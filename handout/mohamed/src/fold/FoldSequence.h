// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : fold            Owner : Mohamed Bile (Fold transformation engine)
//  File   : FoldSequence.h
//  A fold sequence on disk (.ori text file): the paper outline plus the
//  ordered fold steps, so models can be designed without recompiling.
//
//    # comment
//    name Dog face
//    paper 200 0  400 200  200 400  0 200     outline, in order
//    start white                               white | colour side up
//    valley   400 200  0 200  Fold top corner down
//    mountain 0 110  400 110  Fold the tip behind
//    decorate dog                              optional named decoration
//
//  A fold line is given in the CURRENT folded coordinates. The paper to the
//  right of a -> b on screen (y down) is the part that moves.
// ============================================================================
#pragma once
#include "FoldEngine.h"
#include <string>
#include <vector>

namespace ori {

struct FoldSequence {
    std::string name;
    std::vector<Vec2> paper;
    std::vector<FoldStep> steps;
    bool whiteSideUp = true;       // origami convention: start with the white side up
    std::string decoration;        // e.g. "dog" (drawn by the models module)

    std::vector<FoldState> run() const { return FoldEngine::run(paper, steps); }

    std::string toText() const;
    static bool fromText(const std::string& text, FoldSequence& out, std::string* error = nullptr);
    bool save(const std::string& path) const;
    static bool load(const std::string& path, FoldSequence& out, std::string* error = nullptr);
};

} // namespace ori
