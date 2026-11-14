#!/bin/sh
# ============================================================================
#  ORI - commit one member's package on its own branch
#
#    git switch main && git pull
#    sh scripts/push_package.sh MEMBER
#
#  Run it in your own clone of the repository. It:
#    1. moves your files from handout/MEMBER/ into their real folders
#       (skip this by copying them in yourself if there is no hand-out),
#    2. checks every file of your package is present and compiles,
#    3. creates (or reuses) the branch  package/MEMBER,
#    4. makes one commit per group listed in scripts/packages.tsv, with your
#       own git name and email, and a last commit removing handout/MEMBER/,
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

# 1. Hand-out: copy the files into place (the folder itself is removed below).
if [ -d "handout/$m" ]; then
    echo "== moving your files out of handout/$m/"
    (cd "handout/$m" && find . -type f ! -name PUSH.txt) | while IFS= read -r f; do
        f=${f#./}
        mkdir -p "$(dirname "$f")"
        cp "handout/$m/$f" "$f"
    done
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

if [ -d "handout/$m" ] && git ls-files --error-unmatch "handout/$m" >/dev/null 2>&1; then
    git rm -r -q "handout/$m"
    git commit -q -m "chore: remove handout/$m (files are now in place)"
    echo "   committed: removed handout/$m"
fi

echo
git log --oneline -n 8
echo
echo "Next:"
echo "   git push -u origin $branch"
echo "and open a pull request from $branch into main. CI compiles everything whose"
echo "dependencies are already on main and lists the rest as 'pending'."
