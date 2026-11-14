# The fold engine: reflections by composition

**Owner:** Mohamed Bile · **Code:** `src/math/*`, `src/fold/*`, `patterns/*.ori`
**Evidence sheets:** `07_transformations`, `08`–`10` fold sequences (`demos/FoldSheets.cpp`, `demos/08_10_fold_sequences.cpp`) · **Tests:** `tests/test_math.cpp`, `tests/test_fold.cpp`

## Homogeneous 3×3 matrices (`Mat3.*`)

A point `(x, y)` is the column `(x, y, 1)` and a direction is `(x, y, 0)`, so translations do not move directions. `A * B` means "B first, then A".

**A fold is a reflection across the crease.** It is built by composing the basic transforms:

```
M = T(p) · R(θ) · S(1, −1) · R(−θ) · T(−p)
```

Read right to left: move the crease point to the origin, rotate the crease onto the x-axis, mirror in y, rotate back, move back. `reflectLineClosedForm` builds the same matrix directly as `[cos2θ sin2θ; sin2θ −cos2θ]` about `p`. The tests check the two forms agree on 300 random creases, and sheet 07 checks 1000 more at run time (maximum difference about 1e−13).

Properties the tests rely on:

* `det(M) = −1`: a reflection reverses orientation, so a panel is **face down exactly when its accumulated matrix has a negative determinant**.
* `M·M = I`: folding twice along the same crease undoes the fold.
* Two crossing creases make a **rotation** by twice the angle between them, and two parallel creases make a **translation** by twice their distance (a pleat). Sheet 07 shows both.

**Partial folds** for animation: `partialFold(a, b, t) = T R S(1, cos πt) R⁻¹ T⁻¹`. Seen from above, the flap's distance from the crease shrinks by `cos πt`. `t = 0` is the identity, `t = ½` is edge-on (det 0), and `t = 1` equals `reflectLine`.

## Folding a stack (`FoldEngine.*`)

The model is a list of **panels**. Each panel is a flat polygon plus the `sheetToFolded` matrix that carried it from the flat sheet, a layer number, and a face-up flag. One fold:

1. **Split.** Clip every panel against both half-planes of the fold line (Sutherland–Hodgman with a single clip edge). Slivers below 1e−6 area are dropped.
2. **Reflect.** Pieces on the moving side (right of `a → b` on screen) get `M · outline`, and their `sheetToFolded` becomes `M · sheetToFolded`.
3. **Restack.** Folding reverses the order of the moving layers. A valley fold puts them **on top** (`max + 1 + (max − layer)`) and a mountain fold tucks them **under** (`min − 1 − (layer − min)`). Layers are then renumbered 0..n−1.
4. **Crease.** Where a panel was really cut, the cut is mapped back with `sheetToFolded⁻¹` and recorded on the flat sheet. If the panel was face down, the crease reads the opposite way (M ↔ V). So **one fold through several layers produces both mountain and valley creases** (kite cone, napkin).

Invariants checked by the tests:

* The total paper area never changes.
* Every panel's `sheetOutline()` lies on the original sheet, and together they tile it.
* `faceUp == !flipsOrientation()`.

**In-progress folds.** `inProgress(state, step, t)` applies `partialFold(t)` to the moving pieces. Until the flap passes upright (`t = ½`), its layers keep their order, so the layer that was on top stays nearest the viewer. After that the order reverses and the flap shows its other side.

## Fold sequences on disk (`FoldSequence.*`)

`.ori` text files let anyone design a model without recompiling:

```
name Dog face
paper  200 0  400 200  200 400  0 200
start white                         # white | colour side up
valley 400 200  0 200  Fold top corner down
mountain 0 110 400 110 Fold the tip behind
decorate dog                        # optional
```

`ori fold FILE.ori` runs the engine and writes the diagram sheet, the finished model, a GIF, and the generated `.cp` and `.svg`. `ori model dog OUT.ori` exports a built-in model. Parse errors report a line number.

## Fold diagram sheets (`demos/FoldSheets.cpp`)

`foldSequenceSheet` draws every state of a sequence at one shared scale, taken from the union of all the states' bounds, so the model visibly shrinks as it folds. Each state carries its next crease and a Bézier arrow. The sheet ends with the result, the generated crease pattern with its M/V counts, and the fold-sequence list. The sheet grows to as many rows as the sequence needs, so `ori fold` works for user designs with any number of steps. `foldResultImage` renders just the finished model.

## In the studio (`studio/StudioFold.cpp`)

Drawing a fold on the preview inverts the view matrix to get model coordinates and snaps them to panel corners and edge midpoints (then the grid). While you drag, it runs `FoldEngine::fold` on a temporary step and shows the result as a translucent ghost. Before a step is added, the paper areas on both sides of the line are measured, so a line that misses the paper is rejected instead of adding an empty step. Steps can be inserted, reordered, flipped (swapping `a` and `b` changes which side moves) and toggled between valley and mountain. The whole sequence is refolded every frame, which is cheap.
