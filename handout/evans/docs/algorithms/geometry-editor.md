# Geometry and the crease-pattern editor

**Owner:** Evans Mwendwa · **Code:** `src/geometry/*`, `src/editor/*`, `src/core/Deflate.*`, `patterns/*.cp`
**Evidence sheet:** `03_polygons` · **Tests:** `tests/test_geometry.cpp`, `tests/test_editor.cpp`, `tests/test_export.cpp`

## Lines and segments (`Line.*`)

* **Representation.** `Segment{a, b}` is a crease or an edge. `Line{p, d}` is an infinite fold axis, with the parametric form `p + t·d`.
* **Side of line.** `cross(d, q − p)`: positive on one side, negative on the other, 0 on the line. The fold engine uses the same test to decide which paper moves.
* **Implicit form.** `Ax + By + C = 0` with `(A, B)` the unit normal, so `|Ax + By + C|` is the distance.
* **Intersection.** For segments `p + t·r` and `q + u·s`: `t = cross(q − p, s) / cross(r, s)`, `u = cross(q − p, r) / cross(r, s)`. They meet when `0 ≤ t, u ≤ 1`. A denominator of 0 means parallel lines.
* **Projection and reflection.** `foot = p + d·((q − p)·d / |d|²)`, and the mirror image is `q' = 2·foot − q`. A test checks that this agrees with `Mat3::reflectLine`.

## Polygons (`Polygon.*`)

A polygon is an **ordered vertex list**, implicitly closed. Every paper sheet and folded panel is one.

| Operation | Method |
|---|---|
| Area | Shoelace: `½ Σ (x_i y_{i+1} − x_{i+1} y_i)`. The sign gives the orientation. |
| Centroid | `Σ (v_i + v_{i+1}) · cross(v_i, v_{i+1}) / (6A)` |
| Convexity | Every turn `cross(b − a, c − b)` has the same sign (collinear vertices are ignored). |
| Point in polygon | Cast a ray to +x and count edge crossings. An odd count means inside (even-odd rule). |
| Transform | Multiply every vertex by a `Mat3`. A reflection reverses the vertex order. |
| Triangulation | Ear clipping: repeatedly cut a convex corner that contains no other vertex. Produces `n − 2` triangles. |
| Convex hull | Andrew's monotone chain: sort by x, build the lower and upper chains, pop while not turning left. |

## The crease-pattern model (`CreasePattern.*`)

* `addCrease` trims a crease to a convex sheet with **Cyrus–Beck** and rejects creases that miss the paper. `addFullCrease` adds the whole chord along a line.
* `intersections()` tests every pair of creases with segment–segment intersection. `vertices()` returns the paper corners, crease ends and intersections, deduplicated.
* **Snapping.** `snap(p, radius, grid)` first looks for a vertex within `radius`, because creases must meet exactly. Otherwise it rounds to the grid, measured from the paper's corner.
* `split()` cuts every crease where another one crosses it, giving the edges of a planar graph.
* `toggle`, `removeCrease` and `creaseAt` (nearest crease within a distance) are the editor's operations.

### File formats

* **`.cp` (text).** `paper x y x y …`, then one crease per line: `M|V|A x1 y1 x2 y2`. Optional `name` and `grid` lines, and `#` comments. Lines in **ORIPA's numeric form** `type x1 y1 x2 y2` (1 = edge, 2 = mountain, 3 = valley) are also read, so patterns can move between ORI and ORIPA. Errors report a line number.
* **`.svg`.** A print-ready export: the paper polygon, mountain creases as red dash-dot, valley creases as blue dashed.

## Rendering (`CreaseRenderer.*`)

`fitView` builds `T(screen centre) · S(k) · T(−world centre)` to fit any pattern into a panel. `drawCreasePattern` scanline-fills the paper, draws the grid (each line trimmed to the sheet), the creases in notation and the vertex markers. `fillCreaseFaces` finds every face with a 4-connected flood fill on an off-screen mask and gives it one of two colours by the parity of creases crossed from a reference point. Neighbouring faces of a flat-foldable pattern always differ.

## File export: DEFLATE (`src/core/Deflate.*`)

The PNG writer needs a zlib stream, and ORI has no zlib, so this is a small DEFLATE compressor (RFC 1951) with the zlib wrapper (RFC 1950):

1. **LZ77.** A hash of the next 3 bytes selects a chain of earlier positions in a 32 KB window. Up to 48 candidates are compared. The longest match of 3–258 bytes is written as a *(length, distance)* pair; anything else is a literal byte. Each position is entered in the chain as it is passed.
2. **Fixed Huffman codes** from the RFC, so no code tables need to be sent:

   | Symbols | Code length |
   |---|---|
   | literals 0–143 | 8 bits |
   | literals 144–255 | 9 bits |
   | end of block 256, lengths 257–279 | 7 bits |
   | lengths 280–287 | 8 bits |
   | distances | 5 bits |

   Lengths and distances then add their *extra bits*. Huffman codes are written most-significant bit first and everything else least-significant first.
3. **zlib wrapper**: header `78 9C`, the block, then the **Adler-32** checksum (`a = 1 + Σ bytes`, `b = Σ a`, both mod 65521).

`tests/test_export.cpp` contains a small fixed-Huffman *inflater*. It round-trips empty input, a single byte, 70 000-byte runs (longer than the window), repeated text and random noise. It also decodes a PNG written by `Image::savePNG`, undoes the row filters by hand and checks every pixel.

## In the studio (`studio/StudioCrease.cpp`)

The crease-pattern tab is the interactive editor on top of `CreasePattern`. Drawing uses `snap` (vertices first, then the grid), and Shift rounds the angle to 22.5°. Full-length lines use Cyrus–Beck chords, selection uses `creaseAt`, and faces are 2-coloured by flood fill. The **flat-foldability checks** split the pattern at every crossing and then, at each interior vertex, count mountain and valley creases (Maekawa: |M − V| = 2) and sum alternate sector angles (Kawasaki: 180°).
