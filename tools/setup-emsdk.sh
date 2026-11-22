#!/usr/bin/env bash
# Installs the pinned Emscripten SDK into externals/emsdk (gitignored), or reuses $EMSDK if it is set.
# Safe to run again: it only clones/installs what is missing. Never changes your shell profile.
set -euo pipefail

EMSDK_VERSION="4.0.23"

repoRoot="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ -n "${EMSDK:-}" ]]; then
    emsdkDir="${EMSDK}"
    echo "Using the existing emsdk from \$EMSDK: ${emsdkDir}"
    installed="$(cat "${emsdkDir}/upstream/emscripten/emscripten-version.txt" 2>/dev/null | tr -d '"' || true)"
    if [[ "${installed}" != "${EMSDK_VERSION}" ]]; then
        echo "warning: \$EMSDK has Emscripten '${installed:-unknown}', NACad pins ${EMSDK_VERSION}" >&2
    fi
    exit 0
fi

emsdkDir="${repoRoot}/externals/emsdk"
if [[ ! -d "${emsdkDir}/.git" ]]; then
    mkdir -p "${repoRoot}/externals"
    git clone https://github.com/emscripten-core/emsdk.git "${emsdkDir}"
fi

cd "${emsdkDir}"
if [[ "$(git describe --tags --exact-match 2>/dev/null || true)" != "${EMSDK_VERSION}" ]]; then
    git fetch --tags
    git checkout --quiet "${EMSDK_VERSION}"
fi

./emsdk install "${EMSDK_VERSION}"
./emsdk activate "${EMSDK_VERSION}"

echo
echo "Emscripten ${EMSDK_VERSION} is ready. In each new shell run:"
echo "    source externals/emsdk/emsdk_env.sh"
