#!/usr/bin/env bash
# Bump the patch component (third number) of the latest vMAJOR.MINOR.PATCH tag and push it.
#
# Usage (from repo root):
#   ./releaseNextMinor.sh
#
# Triggers the Release workflow (push tag v*).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${REPO_ROOT}"

if ! git rev-parse --git-dir >/dev/null 2>&1; then
    echo "Not a git repository: ${REPO_ROOT}" >&2
    exit 1
fi

mapfile -t tags < <(git tag -l 'v*' --sort=-v:refname)

echo "Existing tags:"
if ((${#tags[@]} == 0)); then
    echo "  (none matching v*)"
    echo "No vMAJOR.MINOR.PATCH tags found; cannot infer next version." >&2
    exit 1
fi
printf '  %s\n' "${tags[@]}"

latest="${tags[0]}"
echo
echo "Latest tag: ${latest}"

ver="${latest#v}"
if [[ ! "${ver}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Latest tag is not vMAJOR.MINOR.PATCH: ${latest}" >&2
    exit 1
fi

IFS=. read -r major minor patch <<<"${ver}"
new_tag="v${major}.${minor}.$((patch + 1))"

if git rev-parse -q --verify "refs/tags/${new_tag}" >/dev/null; then
    echo "Tag already exists locally: ${new_tag}" >&2
    exit 1
fi

echo "New tag:    ${new_tag}"
git tag "${new_tag}"
git push --tags

echo "Done: created and pushed ${new_tag}"
