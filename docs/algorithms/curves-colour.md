# Curves, circles and colour

**Owner:** Matthew Wachira · **Code:** `src/curves/*`, `src/shading/*`, `src/raster/Circle.*`, `src/core/Color.h`, `src/core/GifWriter.*`
**Evidence sheets:** `05_circles_ellipses`, `06_curves`, `13_animation_frames` · **Tests:** `tests/test_curves.cpp`, `tests/test_circles.cpp`, `tests/test_colour.cpp`

## Circles and ellipses (`Circle.cpp`)

**Midpoint circle.** Compute one octant, from `(0, r)` to `x = y`, and mirror it 8 ways. The decision value starts at `d = 1 − r`. If `d < 0` the midpoint is inside, so step east: `d += 2x + 3`. Otherwise step south-east: `d += 2(x − y) + 5`. Integers only.

**Midpoint ellipse.** The ellipse has two regions:

* **Region 1**, where the slope is shallower than −1: step x every time and decide whether y drops. Starts from `d1 = ry² − rx²·ry + ¼rx²`.
* **Region 2**: step y every time and decide whether x grows.

Switch regions when `2ry²x ≥ 2rx²y`. Very flat ellipses can reach `y = 0` before `x = rx`, so the run is finished along the major axis. The test `ellipse_midpoint_regions_cover_the_quadrant` found this case.

**Fills.** For every row, take the widest midpoint point and fill one horizontal span. The fill matches the outline exactly, and with alpha each pixel is blended once. `drawArc` keeps the midpoint points within an angle range. `drawRing` fills the spans of the outer circle minus the inner one.

## Bézier curves and splines (`Curves.cpp`)

| Curve | Notes |
|---|---|
| **de Casteljau** | Repeated linear interpolation, `n → n−1 → … → 1` points. Works for any degree, and `deCasteljauLevels` returns every level (sheet 06, panel 1). |
| **Bernstein cubic** | `B(t) = (1−t)³P0 + 3t(1−t)²P1 + 3t²(1−t)P2 + t³P3`. Checked against de Casteljau to about 1e−14. |
| **Adaptive subdivision** | Split at `t = ½`: the left half's control points are the first point of each de Casteljau level, the right half's are the last. Stop when the inner control points are within the tolerance of the chord. Points go where the curve bends. |
| **Catmull–Rom** | Interpolates every point. Tangent `m_i = (P_{i+1} − P_{i−1}) / 2`, fed to a cubic **Hermite** segment. C1 continuous. |
| **Uniform cubic B-spline** | Basis `(1−t)³/6, (3t³−6t²+4)/6, (−3t³+3t²+3t+1)/6, t³/6`. C2 continuous, stays inside the control points' convex hull and approximates rather than passing through them. |

**Fold arrows.** A cubic Bézier is bowed off the chord by `bulge × |chord|`. A valley fold gets a filled (scanline) triangle head and a mountain fold a hollow one, as in printed origami diagrams.

**Easing.** `cubicBezierEase` is the CSS timing curve through `(0,0), (x1,y1), (x2,y2), (1,1)`. It solves `x(s) = t` for `s` by bisection, which works because x is monotonic, and returns `y(s)`. `easeInOut` (`.42, 0, .58, 1`) times every fold in the animation (sheet 13).

## Colour and shading

* **`Color.h`.** RGBA with `lerp`, `scale` and Porter–Duff source-over: `C = α·src + (1−α)·dst`.
* **Palettes** (`Palette.cpp`). Each palette has a coloured front and white back, mountain red and valley blue, and the outline, background and grid colours. There are three: classic, indigo and blueprint.
* **Depth shading.** `depthShade(base, rank)` moves a fixed `0.12` per level towards a warm shadow, capped at `0.60`. A 2-layer fold does not look as dark as a 10-layer one.
* **Lighting.** `lightingFactor` gives `1 ± 0.07` across the model's bounding box towards the light direction. It is applied per pixel through `fillPolygonShaded`.

## Animated GIF (`GifWriter.cpp`)

GIF allows only 256 colours per frame, so frames are quantised to a fixed **6×7×6 colour cube** (252 colours). The last 4 slots hold the UI's flat background colours exactly, so large backgrounds are not dithered. **4×4 Bayer ordered dithering** adds a threshold `(B[y&3][x&3] + .5) / 16` before rounding, which spreads gradient error into a regular pattern. The index stream is **LZW-compressed**: the code table starts with the colours plus the clear and end codes, grows the code size up to 12 bits, and emits a clear code when the table is full. Each frame carries a Graphic Control Extension (delay) and the file a NETSCAPE loop block.

## In the studio (`studio/StudioPalette.cpp`)

The timeline maps a position `p ∈ [0, n]` to fold `k = ⌊p⌋` at progress `easeInOut(p − k)`, so scrubbing or playing (1.2 s per fold) shows every fold in progress with the same Bézier timing as the GIF. The depth palette edits the live `RenderOptions`: depth shading and its step per layer, lighting (the dial sets the light direction vector), outlines, shadow, X-ray alpha and the palette.
