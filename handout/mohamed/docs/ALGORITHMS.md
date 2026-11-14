# ORI: algorithm notes

Each owner documents the algorithms in their own area. Use these notes for the viva: they say how each algorithm works, where the code is, which evidence sheet shows it and which tests check it.

| Area | Owner | Notes |
|---|---|---|
| Lines, fills, clipping; test framework | Mathew Nguithi | [algorithms/raster.md](algorithms/raster.md) |
| Geometry, crease-pattern editor, file formats (.cp, .svg, DEFLATE) | Evans Mwendwa | [algorithms/geometry-editor.md](algorithms/geometry-editor.md) |
| 2D transformations, fold engine, fold diagrams | Mohamed Bile | [algorithms/fold-engine.md](algorithms/fold-engine.md) |
| Curves, circles, colour, GIF | Matthew Wachira | [algorithms/curves-colour.md](algorithms/curves-colour.md) |
| Layering, models, studio layout, demo registry | Edgar Kareithi | [algorithms/layering-models.md](algorithms/layering-models.md) |
| Framebuffer, PNG/BMP/PPM writers, the `ori` program, build and CI | Ray Lukorito | this page (below) |

## The pipeline

```
 .cp / .ori file ──► CreasePattern / FoldSequence          (editor, fold)
                         │
                         ▼
 flat sheet ──► FoldEngine: split ─► reflect (Mat3) ─► restack ─► crease
                         │                                          │
                         ▼                                          ▼
               panels (bottom → top)                    generated crease pattern
                         │                                    (.cp, .svg)
                         ▼
 LayerRenderer: painter's order, depth shading, lighting, outlines (layering)
                         │
                         ▼
 scanline fill · Bresenham / Wu · midpoint circles · Bézier     (raster, curves)
                         │
                         ▼
 Image framebuffer ──► PNG (filters + DEFLATE) · BMP · PPM · GIF (LZW)   (core, export)
```

## Framebuffer and image files (`src/core/Image.*`)

**Framebuffer.** `Image` is a `vector<Color>`. `setPixel` ignores pixels outside the image, and `blendPixel` does source-over blending with coverage α. Every algorithm in the project draws through these two calls.

**PNG writer.** The file is the signature, then `IHDR` (8-bit RGB), `IDAT` and `IEND`. Each chunk ends with a CRC-32 from a 256-entry table. Before compression, each scanline is filtered five ways (None, Sub, Up, Average, Paeth), and the filter with the smallest `Σ|signed byte|` is kept. Flat-coloured rows become almost all zeros, and the DEFLATE compressor ([geometry-editor.md](algorithms/geometry-editor.md#file-export-deflate)) then shrinks a 1200×800 sheet from 2.9 MB to 40–90 KB. `savePNG(path, false)` still writes stored (uncompressed) blocks for comparison.

**BMP** is 24-bit, bottom-up, with rows padded to 4 bytes. **PPM** is `P6` followed by raw RGB.

## The `ori` program (`app/main.cpp`)

| Command | What it does |
|---|---|
| `ori [demo …] [--out DIR] [--format png\|bmp\|ppm]` | Render the evidence sheets |
| `ori list` | List demos and owners (from `demos/Registry.cpp`) |
| `ori render FILE.cp OUT.png` | Render a crease pattern |
| `ori svg FILE.cp OUT.svg` | Export a crease pattern as SVG |
| `ori fold FILE.ori [--out DIR] [--no-gif]` | Fold a sequence file. Writes the step sheet, the result, a GIF, `.cp` and `.svg` |
| `ori model dog\|kite\|napkin OUT.ori` | Save a built-in model as a sequence file |

## Build and CI

* `make`, `cmake` and `build.sh`/`build.bat` all build the same three things: the library (`src/*/*.cpp`), the `ori` program (`app/` + `demos/`) and `ori_tests`.
* CI (`.github/workflows/build.yml`) works in two stages. The first runs `scripts/ci_check.sh` on every push: it reports which member packages are present and compiles every file whose includes exist, listing the rest as *pending*. Once all packages are in, the second stage builds on Linux, macOS and Windows, runs the tests, renders every sheet, folds every `patterns/*.ori` and uploads the images. See [PUSH_PLAN.md](PUSH_PLAN.md).
