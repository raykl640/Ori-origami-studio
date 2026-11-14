# Layering, models and the studio layout

**Owner:** Edgar Kareithi · **Code:** `src/layering/*`, `src/models/*`, `src/core/Font.*`, `src/core/Layout.*`, `demos/Demos.h`, `demos/Registry.cpp`
**Evidence sheets:** `11_layers_shading`, `12_characters`, `14_studio_ui` · **Tests:** `tests/test_layering.cpp`, `tests/test_layout.cpp`

## Painter's algorithm (`LayerRenderer.cpp`)

A folded model is a stack of flat panels. There is no depth buffer, so occlusion comes from **drawing order**:

1. Sort the panels by layer, bottom first (`FoldState::bottomToTop`).
2. Scanline-fill each panel in turn, so later (higher) panels overwrite the ones under them.

Sheet 11 shows the stack being painted progressively (2/8, 4/8, 6/8, 8/8 panels) and what happens when the order is reversed: the ears and snout disappear behind the back layers. The test `layering_painters_algorithm_top_layer_wins` checks both cases pixel by pixel.

**Panel colour.** Origami paper is coloured on one side. A panel shows its coloured side when `faceUp ≠ whiteSideUp`, where `whiteSideUp` is the model's starting side. The colour is then:

* depth-shaded by `rank = topLayer − layer`, so the top layer is at full brightness
* lit per pixel through `fillPolygonShaded`
* outlined with Xiaolin Wu anti-aliased edges

A soft drop shadow is drawn first from the union of the panels. Every shadow pixel is blended once through a coverage mask, so overlapping panels do not darken it twice.

**Translucent X-ray.** With `alpha < 1`, each panel is alpha-blended (`C = α·panel + (1−α)·below`), so tucked-in layers show through. The test computes the expected colour of two stacked 50 % layers.

**Exploded view.** Tilt the model isometrically with `S(1, ½) · R(45°)`, then lift each layer by `layer × 17 px`. Labels are nudged so they do not overlap.

**Layer inspector.** One thumbnail per layer, top first: a pale ghost of the whole stack with that layer in its true colour.

**Fold diagrams.** The next crease is found by clipping the fold line to the model's convex hull with Cyrus–Beck. The arrow runs from the area-weighted centroid of the moving paper to its mirror image.

## Model library (`Models.cpp`)

| Model | Paper | Folds | Result |
|---|---|---|---|
| Dog face | diamond, white side up | top corner down, two ears, snout up (4 valley) | 8 panels, 8 layers |
| Kite cone | diamond, colour up | two valley folds on the **22.5° bisectors** of the top corner, then a mountain fold behind | 6 panels; one fold makes both M and V creases |
| Napkin triangle | square, white up | book fold, mountain fold, diagonal valley | 8 layers |

`creasePatternOf(state)` turns the creases the engine recorded into a `CreasePattern` for the editor, `.cp` and `.svg`. The dog's features are drawn in model coordinates and scaled by the view: eyes are midpoint circles, the nose a midpoint ellipse, and the mouth and brows Bézier curves.

**Crane mascot.** A concave 11-vertex outline is triangulated by ear clipping. Each facet is shaded by how its longest edge faces the light. The shadow is a translucent ellipse and the wind lines are Catmull–Rom splines.

## Text and sheet layout (`Font.*`, `Layout.*`)

* **Bitmap font.** A 5×7 font with each glyph stored as 7 strings of `#` and `.`, so it can be edited in the source. Each font pixel is drawn as a `scale × scale` block. Upper case only.
* **Layout.** `drawHeader` draws the dark title bar, and `drawPanel` a white panel with a title strip that returns its content rectangle. `gridCell(i, cols, rows, …)` lays panels out with equal margins. Every evidence sheet and the studio mock-up (sheet 14) are built from these.
* **Demo registry.** `demos/Registry.cpp` lists each sheet with its command-line key, output name, title, owner and function. `ori list`, `ori <key>` and the CI renders all read it. `makeSheet` creates a sheet with its header.
* **Studio UI (sheet 14)** contains the five regions from the design: crease editor, fold sequence (with timeline), folded preview, layer inspector and depth palette (slider, light dial, palettes). All of them are drawn from the live dog-face model.

## In the studio (`studio/StudioPreview.cpp`, `StudioFiles.cpp`, `StudioDialogs.cpp`)

The preview is the painter's-algorithm renderer working live. Hidden layers are filtered out before painting, the highlighted layer gets a thick outline, and the X-ray and exploded views are one click away. The layer inspector reuses `drawLayerThumbnail` for each row. The same layout helpers (panels, numbered regions) build the studio's dialogs and its open / save / export handling.
