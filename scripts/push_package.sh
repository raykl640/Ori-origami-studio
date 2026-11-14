#!/bin/sh
# ============================================================================
#  ORI - commit one member's package on its own branch
#
#    git switch main && git pull
#    sh scripts/push_package.sh MEMBER [HAND-OUT]
#
#  HAND-OUT is your package from the "Member hand-outs" draft release: the
#  MEMBER.zip file itself, or the MEMBER/ folder you extracted from it. If you
#  leave it out, the script looks for ./MEMBER/ and ./MEMBER.zip in the clone.
#  It:
#    1. copies your files from the hand-out into their real folders,
#    2. checks every file of your package is present and compiles,
#    3. creates (or reuses) the branch  package/MEMBER,
#    4. makes one commit per group listed in scripts/packages.tsv, with your
#       own git name and email,
#    5. prints the push command. It never pushes for you.
#  Packages do not depend on each other's order: push whenever you are ready.
# ============================================================================
set -eu
cd "$(dirname "$0")/.."
MANIFEST=scripts/packages.tsv
TAB=$(printf '\t')
m=${1:?usage: sh scripts/push_package.sh MEMBER   (ray, mathew, evans, mohamed, matthew, edgar)}

git rev-parse --git-dir >/dev/null 2>&1 || { echo "not inside a git clone" >&2; exit 1; }
name=$(git config user.name || true)
[ -n "$name" ] || { echo "set your identity first: git config user.name \"Your Name\"; git config user.email you@example.com" >&2; exit 1; }

# 1. Find the hand-out (a folder or a zip) and copy its files into place.
src=""
for cand in "${2:-}" "$m" "$m.zip"; do
    [ -n "$cand" ] && [ -e "$cand" ] && { src=$cand; break; }
done
tmp=""
case "$src" in
    *.zip)
        command -v unzip >/dev/null 2>&1 || { echo "no 'unzip' here - extract $src with your file manager, then run this again" >&2; exit 1; }
        tmp=$(mktemp -d)
        unzip -q "$src" -d "$tmp"
        src="$tmp/$m"
        [ -d "$src" ] || src=$tmp
        ;;
esac
if [ -n "$src" ] && [ -d "$src" ]; then
    # an extracted zip sometimes ends up one level deeper (MEMBER/MEMBER/...)
    [ -d "$src/$m" ] && [ ! -e "$src/src" ] && [ ! -e "$src/studio" ] && src="$src/$m"
    echo "== copying your files from $src"
    (cd "$src" && find . -type f ! -name PUSH.txt) | while IFS= read -r f; do
        f=${f#./}
        mkdir -p "$(dirname "$f")"
        cp "$src/$f" "$f"
    done
    [ -n "$tmp" ] && rm -r "$tmp"
else
    echo "== no hand-out found - using the files already in place"
fi

echo "== checking package '$m'"
sh scripts/ci_check.sh verify "$m"

branch="package/$m"
if git show-ref --verify --quiet "refs/heads/$branch"; then
    git switch "$branch"
else
    git switch -c "$branch"
fi

echo "== committing as $name"
made=0
awk -F'\t' -v m="$m" '$1 == "C" && $2 == m { print $3 "\t" $4 }' "$MANIFEST" |
while IFS="$TAB" read -r group msg; do
    awk -F'\t' -v m="$m" -v g="$group" '$1 == "F" && $2 == m && $3 == g { print $4 }' "$MANIFEST" |
    while IFS= read -r path; do
        # shellcheck disable=SC2086
        for p in $path; do [ -e "$p" ] && git add -- "$p"; done
    done
    if git diff --cached --quiet; then
        echo "   (nothing new for: $msg)"
    else
        git commit -q -m "$msg"
        echo "   committed: $msg"
    fi
done

echo
git log --oneline -n 8
echo
echo "Next:"
echo "   git push -u origin $branch"
echo "and open a pull request from $branch into main. You can delete the hand-out"
echo "folder / zip now (it is not part of the repository). CI compiles everything whose"
echo "dependencies are already on main and lists the rest as 'pending'."
