# ORI — A Procedural Origami Folding Studio

![ORI studio mock-up, rendered by our own C++ rasterizer](docs/screenshots/14_studio_ui.png)

**ORI** is a design tool that folds a flat origami **crease pattern** into its folded state using nothing but **composed reflection transformations**, then renders the stacked paper as a layered raster image.

1. **Sketch**: draw a flat crease pattern of straight mountain and valley creases on a paper outline.
2. **Fold**: each crease is a reflection. Folds compose in sequence to bend the paper.
3. **Layer**: folded panels are clipped to their boundaries and z-ordered into a stack.
4. **Export**: the folded result is rasterised and saved as an image or a fold-sequence animation.

> **Checkpoint build.** Everything in this repository is **pure C++17 with no graphics library**. We wrote every pixel-producing algorithm ourselves: line drawing, polygon filling, clipping, circles, ellipses, curves and image encoding. The program writes PNG, BMP, PPM and GIF files directly.

---

## Group members and responsibilities (Group 1)

| Member | Area | Folders they own | Checkpoint work |
|---|---|---|---|
| **Evans Mwendwa** | Crease pattern editor | `src/geometry/`, `src/editor/`, `patterns/` | Polygon representation and operations, line intersection, crease-pattern model, grid and vertex snapping, `.cp` file format, SVG export |
| **Mohamed Bile** | Fold transformation engine | `src/math/`, `src/fold/` | `Vec2` and `Mat3` homogeneous matrices, reflection by composition, fold engine (split, reflect, restack, crease generation) |
| **Mathew Nguithi** | Rasterizer | `src/raster/` | DDA, Bresenham, Wu lines, styled strokes, midpoint circle and ellipse, scanline ET/AET fill, seed fills, Cohen–Sutherland, Liang–Barsky, Cyrus–Beck, Sutherland–Hodgman |
| **Matthew Wachira** | Colour, depth shading and curves | `src/shading/`, `src/curves/` | Palettes, depth shading, lighting gradient, Bézier (de Casteljau, Bernstein, adaptive), Catmull–Rom, B-spline, fold arrows, easing |
| **Edgar Kareithi** | Layering and panel stacking | `src/layering/`, `src/models/` | Painter's algorithm, translucent preview, exploded view, layer inspector, origami model library (dog, kite, napkin, crane) |
| **Ray Lukorito** | App shell, animation and export | `src/core/`, `app/`, `demos/` (shared), `tests/`, `docs/` | Framebuffer, PNG/BMP/PPM/GIF encoders, bitmap font, CLI, fold animation, studio mock-up, test framework, build system, CI |

Each demo file in `demos/` names its owner in its header comment. Run `ori list` to see who owns which demo.

## Technologies and tools

* **Language:** C++17 (standard library only)
* **Build:** GNU Make, CMake (3.14+), or the one-line scripts `build.sh` and `build.bat`
* **Compilers tested:** g++ 13 and clang++ (zero warnings with `-Wall -Wextra -Wpedantic -Wshadow`). MSVC is supported through CMake.
* **Output formats:** PNG, BMP, PPM, animated GIF and SVG, all written by our own encoders
* **Version control and CI:** Git and GitHub, issue and PR templates, and a GitHub Actions workflow that builds on Linux, macOS and Windows, runs the tests and uploads the renders
* **Documentation helper:** `scripts/make_evidence_pdf.py` (Python, ReportLab) builds the screenshot PDF. It is not part of the graphics code.

## Graphics techniques and algorithms implemented so far

| Topic (course) | Algorithms | Where | Project use |
|---|---|---|---|
| **Line drawing** | DDA, Bresenham (all octants), Xiaolin Wu anti-aliasing, brush width, dash patterns | `src/raster/Lines.*` | Creases in origami notation (valley: dashed; mountain: dash-dot) |
| **Line operations** | Segment and line intersection, side-of-line test, projection, distance, reflection of a point or line, grid and vertex snapping | `src/geometry/Line.*`, `src/editor/CreasePattern.*` | Crease vertices, snapping in the editor |
| **Line clipping** | Cohen–Sutherland, Liang–Barsky, Cyrus–Beck (any convex window) | `src/raster/Clipping.*` | Trimming creases to the sheet (including a diamond sheet) |
| **Polygon representation** | Ordered vertex list; shoelace area, centroid, perimeter, convexity, orientation, even-odd point-in-polygon, ear-clipping triangulation, convex hull | `src/geometry/Polygon.*` | Every sheet and folded panel |
| **Polygon clipping** | Sutherland–Hodgman, single half-plane clipping and polygon split | `src/raster/Clipping.*` | A fold splits each panel along the crease |
| **Polygon filling** | Scanline with Edge Table and Active Edge Table (even-odd, holes, top-left rule), per-pixel shaded fill, alpha fill, 4- and 8-connected flood fill, boundary fill | `src/raster/Fill.*` | Painting panels, region-filling crease-pattern faces |
| **Circles and ellipses** | Midpoint circle (8-way symmetry), midpoint ellipse (two regions), filled, translucent, arcs | `src/raster/Circle.*` | Vertex markers, handles, the dog's eyes and nose |
| **Curves** | Bézier by de Casteljau (any degree), Bernstein cubic, adaptive subdivision, Catmull–Rom, uniform cubic B-spline, Hermite, Bézier easing | `src/curves/Curves.*` | Fold arrows, mouth and brows, animation timing |
| **2D transformations** (started early) | Homogeneous 3×3 matrices: translate, rotate, scale, reflect across any line (by composition and closed form), inverse, determinant, partial fold `S(1, cos πt)` | `src/math/Mat3.*` | The fold engine |
| **Layering and colour** | Painter's algorithm, alpha blending, depth shading, lighting gradient | `src/layering/`, `src/shading/` | The folded stack |

## Repository structure

```
ori-origami-studio/
├── src/                     the library: one folder per module and owner
│   ├── core/       Ray      framebuffer, PNG/BMP/PPM/GIF encoders, bitmap font, layout
│   ├── math/       Mohamed  Vec2, Mat3 (homogeneous transforms, reflections)
│   ├── geometry/   Evans    Segment, Line, Polygon and their operations
│   ├── raster/     Mathew   lines, circles/ellipses, fills, clipping
│   ├── curves/     Matthew  Bézier, splines, fold arrows, easing
│   ├── shading/    Matthew  palettes, depth shading, lighting
│   ├── editor/     Evans    crease pattern model, snapping, .cp/.svg, renderer
│   ├── fold/       Mohamed  fold engine
│   ├── layering/   Edgar    painter's algorithm, inspector, exploded view
│   └── models/     Edgar    dog face, kite cone, napkin triangle, crane
├── demos/                   one evidence sheet per technique (owner in each header)
├── app/main.cpp             the `ori` command-line program
├── tests/                   dependency-free unit tests, one file per owner
├── patterns/                sample crease patterns (.cp text files)
├── docs/                    screenshots, evidence PDF, algorithm notes, improvement notes
├── scripts/                 GitHub issue setup and evidence-PDF helper
├── .github/                 CI workflow, issue and PR templates, CODEOWNERS example
├── Makefile  CMakeLists.txt  build.sh  build.bat
```

Dependency order (each layer only uses the ones above it): `core → math → geometry → raster → curves, shading → editor → fold → layering, models → demos, app`.

## How to compile and run

**Option A: Make (Linux, macOS, Windows with MSYS2)**
```bash
make            # builds build/ori
make test       # builds and runs the unit tests
./build/ori     # renders every evidence sheet into output/
```

**Option B: CMake (any platform, including Visual Studio)**
```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release        # unit tests
./build/ori                              # Windows: build\Release\ori.exe
```

**Option C: no build tool**
```bash
./build.sh          # Linux / macOS
build.bat           # Windows with MinGW-w64 g++ on PATH
```

**Using the program**
```bash
./build/ori                     # render all 14 sheets (+ .cp, .svg, .gif) into output/
./build/ori list                # list demos and their owners
./build/ori lines curves dog    # render selected demos only
./build/ori --format bmp        # BMP instead of PNG (opens natively on Windows)
./build/ori --out renders       # choose the output folder
./build/ori render patterns/preliminary_base.cp out.png   # render a crease-pattern file
```

Rendering all sheets takes under a second.

## Current progress

**Done (this checkpoint):**
* Every technique covered so far, implemented from scratch and shown on its own labelled evidence sheet. Each sheet ends with a **project application** panel.
* A working **fold-engine prototype**, brought forward from Phase 3. It folds three models (dog face, kite cone, napkin triangle) through valley and mountain folds. It also produces the crease pattern those folds create (mountain/valley assigned per layer) and exports it as `.cp` and print-ready `.svg`.
* **Objects and characters:** an origami dog face with circle eyes, an ellipse nose and a Bézier mouth; the kite cone; the napkin triangle; and a faceted paper-crane mascot.
* **Fold animation** using eased partial-fold matrices, exported as frames and as an **animated GIF** with our own LZW encoder.
* A **studio mock-up** showing the 5 planned interface regions, drawn entirely by our rasterizer.
* **32 unit tests, 2,255 checks**. These include cross-checks such as Cohen–Sutherland agreeing with Liang–Barsky on 500 random segments, composed and closed-form reflections being equal, and folded panels tiling the original sheet.

**Next (Phases 3–4):** an interactive window for the crease editor (mouse input), partial-layer folds, fold reordering in the UI, real-time preview, and PNG compression.

## Screenshots

| | |
|---|---|
| ![](docs/screenshots/12_characters.png) | ![](docs/screenshots/08_fold_dog_face.png) |
| ![](docs/screenshots/01_lines.png) | ![](docs/screenshots/04_clipping_filling.png) |
| ![](docs/screenshots/05_circles_ellipses.png) | ![](docs/screenshots/06_curves.png) |

The animated fold is in [`docs/screenshots/13_fold_dog_face.gif`](docs/screenshots/13_fold_dog_face.gif). All the sheets are collected with captions in **[`docs/ORI_Checkpoint_Evidence.pdf`](docs/ORI_Checkpoint_Evidence.pdf)**.

## Known limitations and issues

* **No interactive window yet.** The program renders to image files. Mouse-driven editing is planned for Phase 4, possibly through a thin windowing layer such as SDL2 that only displays our framebuffer.
* **Simple folds only.** A fold moves *every* layer on the moving side of the crease. Folding only the top flap is not supported yet. Reverse folds, squash folds and turning up just one layer of the dog's snout all need it.
* **No collision or flat-foldability checks.** Self-intersection, Maekawa and Kawasaki checks are out of scope, as the proposal states.
* **Fold steps are written in code.** Fold sequences are defined in `src/models/Models.cpp`. A file format for fold sequences is planned. Crease patterns already load from `.cp` files.
* **PNG files are uncompressed.** Our PNG writer uses *stored* deflate blocks, so a 1200×800 sheet is about 2.9 MB. The copies in `docs/screenshots/` were recompressed for the repository.
* **GIF colour.** GIF export uses a fixed 252-colour cube with ordered dithering, so gradients show a fine dither pattern.
* **Exploded view.** Layer labels in the exploded view can overlap when many layers sit at similar positions.
* **Text.** The built-in 5×7 bitmap font is upper-case only, with no accents.

## More documentation

* [`docs/ALGORITHMS.md`](docs/ALGORITHMS.md): how each algorithm works and where to find it. Useful for the viva.
* [`docs/IMPROVEMENTS.md`](docs/IMPROVEMENTS.md): improvement suggestions and which ones are already implemented.
