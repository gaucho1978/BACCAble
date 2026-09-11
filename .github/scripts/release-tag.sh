#!/usr/bin/env bash
# Validate release identity and move only the explicitly rolling continuous tag.
set -euo pipefail
mode=$1
kind=$2
tag=$3
[[ "$tag" =~ ^[a-zA-Z0-9][a-zA-Z0-9._-]*$ ]]
if [[ "$kind" == stable ]]; then
    [[ "$tag" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]
else
    [[ "$kind" == beta && ! "$tag" =~ ^v[0-9]+\.[0-9]+\.[0-9]+ ]]
fi
head=$(git rev-parse HEAD)
remote=$(git ls-remote origin "refs/tags/$tag")
old=${remote%%[[:space:]]*}
if [[ -n "$old" && "$tag" != continuous ]]; then
    git fetch --no-tags origin "refs/tags/$tag"
    test "$(git rev-parse FETCH_HEAD^{commit})" = "$head"
fi
# Fail on API errors too: a network failure must not permit an overwrite.
published=$(gh api --paginate "repos/$GITHUB_REPOSITORY/releases" --jq '.[] | .tag_name')
if [[ "$tag" != continuous ]]; then
    if printf '%s\n' "$published" | grep -Fxq "$tag"; then
        echo "Release $tag already exists; choose a new version." >&2
        exit 1
    fi
fi
if [[ "$mode" == publish ]]; then
    if [[ "$tag" == continuous ]]; then
        git push --force-with-lease="refs/tags/$tag:$old" origin "HEAD:refs/tags/$tag"
        # A debug rolling build must not leave CAN images from an older source.
        if printf '%s\n' "$published" | grep -Fxq "$tag"; then
            assets=$(gh api "repos/$GITHUB_REPOSITORY/releases/tags/$tag" --jq '.assets[].name')
            while IFS= read -r asset; do
                if [[ "$asset" == baccable* && "$asset" != */* && ! -f "dist/$asset" ]]; then
                    gh release delete-asset "$tag" "$asset" --repo "$GITHUB_REPOSITORY" --yes
                fi
            done <<< "$assets"
        fi
    elif [[ -z "$old" ]]; then
        git push origin "HEAD:refs/tags/$tag"
    fi
else
    [[ "$mode" == validate ]]
fi
