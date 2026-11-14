# How we push the project: one package per member, any order

The old plan had each of us push specific parts in a strict sequence. Every push waited on the one before it, so one person running late held up everyone else. This plan removes the ordering.

* **Every file belongs to exactly one member.** The list is [`scripts/packages.tsv`](../scripts/packages.tsv). No file is in two packages, so two pull requests can never conflict.
* **Each member pulls `main` and pushes their whole package whenever they are ready**, on their own branch and in their own name. Their files are waiting in `handout/<member>/`. The order does not matter.
* **The packages are roughly the same size**, about 2,000–2,200 lines each (table below).
* **CI never blocks on a missing package.** A file that needs a header from a package that has not arrived yet is reported as *pending*, not failed. When the last package lands, the full build and tests run on Linux, macOS and Windows.

## The packages

| Member | Package | Files | Lines | Commits |
|---|---|---:|---:|---:|
| Ray Lukorito (`ray`) | App shell (`ori`), studio core + SDL2 window, PNG/BMP/PPM writers, build system, CI, README, push plan, screenshots | 35 | 2,194 | 5 |
| Mathew Nguithi (`mathew`) | Rasterizer (lines, fills, clipping), studio UI toolkit, test framework + core tests, demos 01, 02, 04, studio guide | 17 | 2,020 | 5 |
| Evans Mwendwa (`evans`) | Geometry, crease-pattern editor (library + studio tab), `.cp`/`.svg`, DEFLATE, demo 03, sample `.cp` files | 19 | 2,083 | 6 |
| Mohamed Bile (`mohamed`) | `Vec2`/`Mat3`, fold engine, `.ori` sequences, fold diagrams, studio fold tool, demos 07–10, algorithm index | 19 | 2,002 | 5 |
| Matthew Wachira (`matthew`) | Colour, shading, curves, circles, GIF, studio palette + timeline, end-to-end studio tests, demos 05, 06, 13 | 18 | 2,042 | 6 |
| Edgar Kareithi (`edgar`) | Layering, models, font/layout, demo registry, studio preview + layers + files + dialogs, demos 11, 12, 14 | 23 | 2,080 | 5 |

Lines include each member's code, tests and algorithm notes (`docs/algorithms/<area>.md`). The screenshots are images and are not counted. Run `sh scripts/ci_check.sh lines` for the current numbers.

Each package is split into **5–6 commits** with prepared messages (for example `raster: scanline fill with ET/AET, flood and boundary fill`), so the history shows each member's work module by module. `scripts/push_package.sh` makes these commits for you, using your own git name and email.

## Step 0: done by Ray

Ray's package is already on `main`. It contains the build files, `scripts/`, the package-aware CI workflow, the image writers, the `ori` program, the studio core and window, the README and the docs. It also adds a **`handout/`** folder with one sub-folder per remaining member, holding exactly that member's files.

Ray chose the **clean slate**: the old partial `src/`, `demos/`, `tests/` and `app/` files were removed from `main`. Every file comes back through its owner's package, so each member's share of the history matches the table above. The old commits are still in the history.

## For each member (any order)

```bash
git switch main
git pull
sh scripts/push_package.sh <member>     # mathew | evans | mohamed | matthew | edgar
git push -u origin package/<member>
```

Then open a pull request from `package/<member>` into `main` on GitHub.

`push_package.sh` does the following:

1. Moves your files from `handout/<member>/` into their real folders.
2. Checks that all of them are present and compile against what is already on `main`.
3. Creates the branch `package/<member>`.
4. Makes your 5–6 commits, using your own git name and email.
5. Makes a last commit that deletes your `handout/` folder.

It never pushes for you. `handout/<member>/PUSH.txt` lists your commits and files.

Before the first commit, set your identity once: `git config user.name "Your Name"` and `git config user.email you@example.com`. Use the email of your GitHub account so the commits count as yours. On Windows, run the commands in **Git Bash**. Without `g++` on the PATH the script skips the local compile check, and CI still runs it on the pull request.

When the last pull request is merged, `handout/` is gone and the full build runs.

## What CI reports

`scripts/ci_check.sh` runs on every push and pull request:

```
MEMBER    NAME                  FILES         LINES
ray       Ray Lukorito        32/32      1504  done
mathew    Mathew Nguithi      13/13      1576  done
evans     Evans Mwendwa        0/18         0  pending
...
  ok       src/raster/Fill.cpp
  pending  src/editor/CreasePattern.cpp  waits for: src/geometry/Line.h(evans) ...
  FAILED   src/curves/Curves.cpp
compiled 21, pending 9, failed 1
```

* **ok**: the file and everything it includes are present, and it compiles.
* **pending**: the file includes something from a package that has not arrived. The owner is shown in brackets. This is not an error.
* **FAILED**: everything the file needs is present but it does not compile. This is a real error and the check fails. Fix it before merging.

When every package is present, the `build` job runs: CMake on Linux, macOS and Windows, all unit tests, every evidence sheet, every `patterns/*.ori` folded, and the images uploaded as `ori-renders`.

## Rules that keep this order-free

* **Only commit files from your own package.** If you need a change in someone else's file, open an issue or ask its owner. The file header and `scripts/packages.tsv` say who that is.
* **Do not rename or move files** without updating `scripts/packages.tsv` in the same pull request.
* **After everything has landed**, normal development continues with small pull requests. Each file keeps its owner as reviewer (see `.github/CODEOWNERS.example`).

## Useful commands

```bash
sh scripts/ci_check.sh status            # which packages are in, line counts
sh scripts/ci_check.sh missing evans     # which of evans's files are not in the tree yet
sh scripts/ci_check.sh verify mathew     # all of mathew's files present and compiling?
sh scripts/ci_check.sh lines             # lines per member
make packages                            # same as status
```
