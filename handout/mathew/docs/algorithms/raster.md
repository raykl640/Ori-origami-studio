# Rasterizer: lines, fills and clipping

**Owner:** Mathew Nguithi · **Code:** `src/raster/Lines.*`, `src/raster/Fill.*`, `src/raster/Clipping.*`
**Evidence sheets:** `01_lines`, `02_line_operations`, `04_clipping_filling` · **Tests:** `tests/test_raster.cpp` (plus the framework in `tests/TestFramework.h`, `tests/test_main.cpp`)

Every pixel ORI produces goes through `Image::setPixel` or `Image::blendPixel`. Nothing below calls a graphics library.

## Line drawing (`Lines.cpp`)

| Algorithm | Function | How it works |
|---|---|---|
| DDA | `lineDDA`, `drawLineDDA` | `steps = max(|dx|, |dy|)`. Add `dx/steps` and `dy/steps` each step and round. Simple, but needs floating point. |
| Bresenham | `lineBresenham`, `drawLine` | One integer error term `err = dx + dy` (with `dy` negative). Each step: `e2 = 2*err`; if `e2 >= dy` step in x, if `e2 <= dx` step in y. `sx`/`sy = ±1` cover all 8 octants with no special cases. |
| Xiaolin Wu | `drawLineWu` | Walk the major axis; the exact minor coordinate `y` falls between two pixels, which get coverage `1 - frac(y)` and `frac(y)`. Endpoints are weighted by how much of their pixel the line covers. |

**Strokes.** A `Stroke` is a colour, a brush width and a dash pattern of on/off run lengths. The pattern is counted per Bresenham step, and `drawPolyline` carries the phase across joints, so a dashed Bézier made of 60 tiny segments still dashes evenly. A width above 1 stamps a round brush at each step (`stampBrush`). These encode origami notation:

* paper edge: solid
* valley: dashed `[dash + w − 1, gap + w − 1]`
* mountain: dash-dot `[9 + w, 3 + w, w, 3 + w]`
* reference line: dotted

The test `lines_dda_and_bresenham_agree_on_shape` checks on 400 random lines that both algorithms plot `max(|dx|,|dy|) + 1` pixels, are 8-connected, stay within half a pixel of the ideal line, and differ by at most one pixel where DDA's rounding hits a tie.

## Polygon filling (`Fill.cpp`)

**Scanline fill with ET/AET.** `scanlineSpans` is the core and every fill uses it.

1. **Edge Table.** Every non-horizontal edge is stored in a bucket keyed by its first scanline, with its x at that scanline's pixel centre and `1/m = dx/dy`.
2. **Active Edge Table.** For each scanline `y`, add the edges that start there, sort the crossings by x and fill between pairs. That is the even-odd rule, so holes and self-intersections work. Then remove the finished edges and add `1/m` to every x.
3. **Top-left rule.** Pixel centres are sampled at `(x + .5, y + .5)` and spans are half-open `[ceil(xa − .5), ceil(xb − .5))`. Two panels that share an edge never paint the same pixel and never leave a gap. This matters for translucent fills. The test `scanline_fill_exact_rectangles_and_shared_edges` blends two triangles at 50 % alpha and checks that no pixel was blended twice.

Variants: `fillPolygon` (solid), `fillPolygonAlpha` (blend once per pixel), and `fillPolygonShaded` (a per-pixel shader, used for depth and lighting gradients).

**Seed fills.** `floodFill` replaces the connected region of the seed's colour. `boundaryFill` spreads until it reaches a boundary colour. Both take 4- or 8-connectivity and use an **explicit stack** plus a visited bitmap, so filling a 1000×1000 region cannot overflow the call stack. A Bresenham diagonal touches only at corners, so an 8-connected fill leaks through it and a 4-connected fill does not. Sheet 04 shows this.

## Clipping (`Clipping.cpp`)

| Algorithm | Function | Idea |
|---|---|---|
| Cohen–Sutherland | `clipCohenSutherland` | 4-bit region codes (`TOP=4, BOTTOM=8, RIGHT=2, LEFT=1`). If both codes are 0, accept. If `code_a & code_b ≠ 0`, reject. Otherwise cut the outside endpoint at one boundary and repeat. |
| Liang–Barsky | `clipLiangBarsky` | Write the segment as `P(t) = A + t·d`. For each boundary, `p_k = (−dx, dx, −dy, dy)` and `q_k` is the distance to it. Entering boundaries raise `t0`, leaving ones lower `t1`. Reject if `t0 > t1`. |
| Cyrus–Beck | `clipCyrusBeck`, `clipLineToConvex` | The same parametric idea against **any convex polygon**, using each edge's inward normal. Both vertex orders are accepted. ORI uses it to trim creases to a diamond sheet and to find where a fold line crosses the folded model. |
| Sutherland–Hodgman | `clipSutherlandHodgman`, `sutherlandHodgmanStages` | Clip the subject against one window edge at a time with four cases: in→in keep the next vertex, in→out keep the intersection, out→in keep the intersection and the next vertex, out→out keep nothing. |

The test `clipping_cohen_sutherland_matches_liang_barsky` clips 500 random segments with all three line clippers and checks that they agree.

## The test framework (`tests/TestFramework.h`, `tests/test_main.cpp`)

The tests have no dependencies. `TEST(name) { … }` defines a function and a static `Registrar` object whose constructor adds it to a global list before `main` runs. `CHECK(cond)` and `CHECK_NEAR(a, b, eps)` count every check and print `file:line` and the expression on failure, at most 8 per test. The runner sorts tests by file and runs them all, or only the tests whose names contain a filter (`ori_tests fold`). It prints `PASS`/`FAIL` with a check count for each test, and its exit code tells `make test` and `ctest` whether everything passed.

## In the studio (`studio/Widgets.*`)

The studio's UI toolkit is built on these primitives. Buttons and panels are `fillRect`/`strokeRect`, toggles draw their tick with two `drawStroke` segments, slider knobs and the timeline are midpoint circles and rings, and text is the bitmap font. Scrolling lists draw into the frame and then restore the pixels outside their rectangle, which is a simple rectangular clip.
