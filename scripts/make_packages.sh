#!/bin/sh
# ============================================================================
#  ORI - build one hand-out folder (and zip) per member from the finished tree
#
#    sh scripts/make_packages.sh [OUTDIR] [--no-zip]   (default: dist/packages)
#
#  For the coordinator only. Each member gets OUTDIR/<member>/ containing
#  their files at their repository paths plus PUSH.txt (their commits and the
#  exact steps), and OUTDIR/<member>.zip (or .tar.gz when zip is missing).
# ============================================================================
set -eu
OUT=""
ZIP=1
for arg in "$@"; do
    case "$arg" in
        --no-zip) ZIP=0 ;;
        *) OUT=$arg ;;
    esac
done
# A relative OUTDIR is relative to where you ran the script from.
case "$OUT" in
    "") ;;
    /*) ;;
    *) OUT="$(pwd)/$OUT" ;;
esac
cd "$(dirname "$0")/.."
MANIFEST=scripts/packages.tsv
TAB=$(printf '\t')
OUT=${OUT:-dist/packages}

missing=$(sh scripts/ci_check.sh missing 2>/dev/null)
if [ -n "$missing" ]; then
    echo "The tree is not complete - these files are missing:" >&2
    printf '%s\n' "$missing" >&2
    exit 1
fi

rm -rf "$OUT"
mkdir -p "$OUT"
for m in $(awk -F'\t' '$1 == "M" { print $2 }' "$MANIFEST"); do
    dir="$OUT/$m"
    mkdir -p "$dir"
    full=$(awk -F'\t' -v m="$m" '$1 == "M" && $2 == m { print $3 }' "$MANIFEST")
    area=$(awk -F'\t' -v m="$m" '$1 == "M" && $2 == m { print $4 }' "$MANIFEST")
    n=0; lines=0
    for row in $(awk -F'\t' -v m="$m" '$1 == "F" && $2 == m { print $4 }' "$MANIFEST"); do
        for p in $row; do
            [ -e "$p" ] || continue
            mkdir -p "$dir/$(dirname "$p")"
            cp "$p" "$dir/$p"
            n=$((n + 1))
            case "$p" in *.png|*.gif) ;; *) lines=$((lines + $(wc -l < "$p" | tr -d ' '))) ;; esac
        done
    done
    {
        echo "ORI package for $full - $area"
        echo "$n files, $lines lines"
        echo
        echo "Your commits (made for you by scripts/push_package.sh):"
        awk -F'\t' -v m="$m" '$1 == "C" && $2 == m { printf "  %s. %s\n", $3, $4 }' "$MANIFEST"
        echo
        echo "Steps (any time, in any order relative to the others):"
        echo "  1. In your clone:  git switch main && git pull"
        echo "  2. Put $m.zip (or the $m/ folder extracted from it) in the clone's top folder"
        echo "  3. sh scripts/push_package.sh $m"
        echo "     (copies these files into place and commits them under your name)"
        echo "  4. git push -u origin package/$m   and open a pull request into main."
        echo "  5. Delete $m.zip / $m/ - they are not part of the repository."
        echo
        echo "Files:"
        awk -F'\t' -v m="$m" '$1 == "F" && $2 == m { printf "  [%s] %s\n", $3, $4 }' "$MANIFEST"
        echo
        echo "Full plan: docs/PUSH_PLAN.md"
    } > "$dir/PUSH.txt"
    if [ "$ZIP" = 0 ]; then
        archive="$dir/"
    elif command -v zip >/dev/null 2>&1; then
        (cd "$OUT" && zip -qr "$m.zip" "$m")
        archive="$OUT/$m.zip"
    else
        tar -czf "$OUT/$m.tar.gz" -C "$OUT" "$m"
        archive="$OUT/$m.tar.gz"
    fi
    printf '%-9s %-17s %3s files %6s lines   %s\n' "$m" "$full" "$n" "$lines" "$archive"
done
