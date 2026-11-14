# How we push the project: one zip per member, any order

`main` was reset to a **clean slate**: only `LICENSE` and `.gitignore` remain. The previous commits are still in the history. Every file of the finished project is in exactly one member's zip. When all six branches are merged into `main`, the project is complete.

* **Every file belongs to one member** ([`scripts/packages.tsv`](../scripts/packages.tsv)). No two zips contain the same file, so pushes can never conflict.
* **Everyone pushes to their own branch** (`ray`, `mathew`, `evans`, `mohamed`, `matthew`, `edgar`), in any order. The branches are merged into `main` at the end.
* **The shares are roughly equal**, about 2,000–2,300 lines each.
* **Each member's code enters the repository in their own commit**, so the history shows who wrote what.

## The zips

| Member | Zip | Part | Files | Lines |
|---|---|---|---:|---:|
| Ray Lukorito | `ray.zip` | App shell (`ori`), studio core + SDL2 window, PNG/BMP/PPM writers, build system, CI, README, screenshots | 35 | 2,256 |
| Mathew Nguithi | `mathew.zip` | Rasterizer (lines, fills, clipping), studio UI toolkit, test framework, demos 01, 02, 04 | 17 | 2,020 |
| Evans Mwendwa | `evans.zip` | Geometry, crease-pattern editor (library + studio tab), `.cp`/`.svg`, DEFLATE, demo 03 | 19 | 2,083 |
| Mohamed Bile | `mohamed.zip` | `Vec2`/`Mat3`, fold engine, `.ori` sequences, fold diagrams, studio fold tool, demos 07–10 | 19 | 2,002 |
| Matthew Wachira | `matthew.zip` | Colour, shading, curves, circles, GIF, studio palette + timeline, studio tests, demos 05, 06, 13 | 18 | 2,042 |
| Edgar Kareithi | `edgar.zip` | Layering, models, font/layout, demo registry, studio preview, files and dialogs, demos 11, 12, 14 | 23 | 2,080 |

Each zip holds its files at their **project paths** (`src/...`, `demos/...`, `tests/...`). Extracting it into the project folder puts every file in the right place.

## What each member does

Run these in a terminal (Git Bash on Windows) inside your clone of the repository. Keep your zip **outside** the project folder, for example in Downloads.

```bash
git fetch origin
git switch main
git reset --hard origin/main          # make your copy identical to GitHub
git switch -c <name>                  # your own branch: ray, mathew, evans, mohamed, matthew, edgar
unzip -o ~/Downloads/<name>.zip -d .  # or Explorer: right-click the zip -> Extract All -> choose this folder
git status                            # only your files should be listed as new
git add -A
git commit -m "<Name>: <your part>"
git push -u origin <name>
```

Each member pushes only to their own branch, so pushes can never be rejected or conflict. `git reset --hard origin/main` throws away anything uncommitted in your copy. Use your own git name and the email of your GitHub account (`git config user.name "Your Name"`, `git config user.email you@example.com`), so the commit is credited to you.

## Merging (once all six branches are pushed)

Either open one pull request per branch on GitHub and merge them in any order, or run:

```bash
git fetch origin
git switch main
git reset --hard origin/main
for b in ray mathew evans mohamed matthew edgar; do
    git merge --no-edit origin/$b
done
git push origin main
```

The merges never conflict, because no two zips share a file. Each merge keeps the member's own commit, so the history shows who wrote what.

## Until everything is merged

The project only compiles once all six parts are on `main`, because every part uses the others. CI runs on every pushed branch that contains Ray's part. On a single branch, most files show as *pending* (waiting for other parts), which is expected. After the final merge, CI builds everything on Linux, macOS and Windows, runs the tests and uploads the renders.

```bash
sh scripts/ci_check.sh status            # which parts are on this branch (needs Ray's part)
make && make test && ./build/ori_studio  # once all six are merged
```
