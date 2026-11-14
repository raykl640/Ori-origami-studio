#!/bin/sh
# ============================================================================
#  ORI - package checks (used by CI and by members before they push)
#
#    sh scripts/ci_check.sh status            which packages are in the tree;
#                                             prints complete=true|false
#    sh scripts/ci_check.sh missing [MEMBER]  list files not pushed yet
#    sh scripts/ci_check.sh partial           compile every .cpp whose includes
#                                             all exist (others are "pending")
#    sh scripts/ci_check.sh verify MEMBER     MEMBER's files are all present
#                                             and each of them compiles
#    sh scripts/ci_check.sh lines             line count per member
#
#  Packages can arrive in any order: a file that includes a header from a
#  package nobody has pushed yet is reported as pending, never as a failure.
#  Real compile errors fail.
# ============================================================================
set -eu
cd "$(dirname "$0")/.."
MANIFEST=scripts/packages.tsv
CXX=${CXX:-g++}
TAB=$(printf '\t')
# The studio window (studio/sdl/) needs SDL2's headers; without them that one
# file is reported as pending, never failed.
SDL_FLAGS=$(pkg-config --cflags sdl2 2>/dev/null || true)

[ -f "$MANIFEST" ] || { echo "missing $MANIFEST" >&2; exit 2; }

# "member<TAB>path" for every file row, globs expanded against the tree.
files() {
    awk -F'\t' -v m="${1:-}" '$1 == "F" && (m == "" || $2 == m) { print $2 "\t" $4 }' "$MANIFEST" |
    while IFS="$TAB" read -r member path; do
        case "$path" in
            *'*'*)
                found=0
                for p in $path; do
                    if [ -e "$p" ]; then printf '%s\t%s\n' "$member" "$p"; found=1; fi
                done
                [ "$found" = 1 ] || printf '%s\t%s\n' "$member" "$path"   # glob with no match = missing
                ;;
            *) printf '%s\t%s\n' "$member" "$path" ;;
        esac
    done
}

members() { awk -F'\t' '$1 == "M" { print $2 }' "$MANIFEST"; }
fullname() { awk -F'\t' -v m="$1" '$1 == "M" && $2 == m { print $3 }' "$MANIFEST"; }

ALL_FILES=""
ownerOf() {   # owner of a repo-relative path, or "?" (file list cached per run)
    [ -n "$ALL_FILES" ] || ALL_FILES=$(files)
    printf '%s\n' "$ALL_FILES" | awk -F'\t' -v p="$1" '$2 == p && !found { print $1; found = 1 } END { if (!found) print "?" }'
}

lineCount() { if [ -e "$1" ] && [ -f "$1" ]; then wc -l < "$1" | tr -d ' '; else echo 0; fi; }

cmd_status() {
    complete=true
    printf '%-9s %-17s %9s %13s\n' MEMBER NAME FILES LINES >&2
    for m in $(members); do
        total=0; present=0; lines=0
        tmp=$(files "$m")
        total=$(printf '%s\n' "$tmp" | grep -c . || true)
        for p in $(printf '%s\n' "$tmp" | cut -f2); do
            if [ -e "$p" ]; then
                present=$((present + 1))
                case "$p" in *.png|*.gif) ;; *) lines=$((lines + $(lineCount "$p"))) ;; esac
            fi
        done
        [ "$present" = "$total" ] || complete=false
        mark="pending"; [ "$present" = "$total" ] && mark="done"
        printf '%-9s %-17s %4s/%-4s %7s  %s\n' "$m" "$(fullname "$m")" "$present" "$total" "$lines" "$mark" >&2
    done
    # Source files nobody owns (keeps the manifest honest).
    listed=$(files | cut -f2)
    for p in $(find src demos app tests studio -type f \( -name '*.cpp' -o -name '*.h' \) 2>/dev/null | sort); do
        printf '%s\n' "$listed" | grep -qx "$p" || echo "warning: $p is not in $MANIFEST" >&2
    done
    echo "complete=$complete"
}

cmd_missing() {
    n=0
    for row in $(files "${1:-}" | tr '\t' '|'); do
        m=${row%%|*}; p=${row#*|}
        if [ ! -e "$p" ]; then echo "$m  $p"; n=$((n + 1)); fi
    done
    [ "$n" = 0 ] && echo "nothing missing" >&2
    return 0
}

# Resolve an include path relative to the file that included it.
resolve() {
    case "$1" in
        /*) printf '%s\n' "$1" ;;
        *) if command -v realpath >/dev/null 2>&1 && realpath -m . >/dev/null 2>&1; then
               realpath -m --relative-to=. "$(dirname "$2")/$1"
           else
               printf '%s\n' "$1"
           fi ;;
    esac
}

# Compile-check the given .cpp files. Returns non-zero on a real error.
check_sources() {
    if ! command -v "$CXX" >/dev/null 2>&1; then
        echo "warning: no C++ compiler ($CXX) found - skipping the compile check (CI will still do it)" >&2
        return 0
    fi
    ok=0; pending=0; failed=0
    for f in "$@"; do
        [ -e "$f" ] || continue
        deps=$("$CXX" -std=c++17 $SDL_FLAGS -MM -MG "$f" 2>/dev/null | sed -e 's/^[^:]*://' -e 's/\\$//' | tr ' ' '\n' | grep -v '^$' || true)
        waits=""
        for d in $deps; do
            if [ ! -e "$d" ]; then
                r=$(resolve "$d" "$f")
                waits="$waits $r($(ownerOf "$r"))"
            fi
        done
        if [ -n "$waits" ]; then
            echo "  pending  $f  waits for:$waits"
            pending=$((pending + 1))
        elif "$CXX" -std=c++17 -Wall -Wextra -Wpedantic $SDL_FLAGS -fsyntax-only "$f"; then
            echo "  ok       $f"
            ok=$((ok + 1))
        else
            echo "  FAILED   $f"
            failed=$((failed + 1))
        fi
    done
    echo "compiled $ok, pending $pending, failed $failed"
    [ "$failed" = 0 ]
}

cmd_partial() {
    # shellcheck disable=SC2046
    check_sources $(find src demos app tests studio -name '*.cpp' 2>/dev/null | sort)
}

cmd_verify() {
    m=${1:?usage: ci_check.sh verify MEMBER}
    [ -n "$(fullname "$m")" ] || { echo "unknown member '$m' - one of: $(members | tr '\n' ' ')" >&2; exit 2; }
    gone=$(cmd_missing "$m" 2>/dev/null)
    if [ -n "$gone" ]; then
        echo "These files of package '$m' are missing:"; printf '%s\n' "$gone"
        exit 1
    fi
    echo "all files of package '$m' are present"
    # shellcheck disable=SC2046
    check_sources $(files "$m" | cut -f2 | grep '\.cpp$' || true)
}

cmd_lines() {
    total=0
    for m in $(members); do
        n=0
        for p in $(files "$m" | cut -f2); do
            case "$p" in *.png|*.gif) ;; *) n=$((n + $(lineCount "$p"))) ;; esac
        done
        printf '%-9s %-17s %6s\n' "$m" "$(fullname "$m")" "$n"
        total=$((total + n))
    done
    printf '%-27s %6s\n' "total" "$total"
}

case "${1:-status}" in
    status)  cmd_status ;;
    missing) cmd_missing "${2:-}" ;;
    partial) cmd_partial ;;
    verify)  cmd_verify "${2:-}" ;;
    lines)   cmd_lines ;;
    *) sed -n '3,17p' "$0"; exit 2 ;;
esac
