#!/usr/bin/env bash
# Re-takes every image in docs/images (the README, CONCEPT, MVP, RESEARCH,
# ARCHITECTURE and docs/RELEASING images) from the current checkout:
#
#   tools/capture-docs.sh [darkroom shoot options, e.g. --only hero]
#
# 1. builds the desktop game (build-docs-native) and the browser game
#    (build-docs-web), the way ci.yml and browser.yml do, unless
#    FAMILIAR_NATIVE (a built familiar) or FAMILIAR_SITE (a directory with
#    index.html, familiar.js and familiar.wasm) say where they already are;
# 2. serves the browser build on 127.0.0.1:$DOCS_PORT (default 8846);
# 3. runs darkroom (https://github.com/Preponderous-Software/darkroom) on
#    tools/docs-media/media.json, the shot list: the native shots come from
#    `familiar --headless --tour` on Mesa llvmpipe, the browser shots from
#    headless Chromium driving the page through the smoke test's hooks
#    (window.familiar) and tools/docs-media/page-helpers.js.
#
# Every shot is re-taken (--force). Stills are fitted under 200 KB and GIFs
# under 3 MB. Look at every image before committing it.
#
# Needs: darkroom 0.7.1 on PATH (or $DARKROOM), with Playwright findable
# through $NODE_PATH (darkroom setup-web also works); for the builds, CMake
# 3.20+, a C++ compiler with the X11/EGL headers ci.yml installs, and an
# activated emsdk (emcmake on PATH) with $STRATA_DIR optional, as in
# browser.yml. Install darkroom with:
#   pip install "darkroom[video] @ git+https://github.com/Preponderous-Software/darkroom@v0.7.1"
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
port="${DOCS_PORT:-8846}"
darkroom="${DARKROOM:-darkroom}"
command -v "$darkroom" > /dev/null || { echo "capture-docs: darkroom not found (see the install line in $0)" >&2; exit 2; }
strata=()
[ -n "${STRATA_DIR:-}" ] && strata=(-DFETCHCONTENT_SOURCE_DIR_STRATA="$STRATA_DIR")

native="${FAMILIAR_NATIVE:-}"
if [ -z "$native" ]; then
  cmake -S "$root" -B "$root/build-docs-native" -DCMAKE_BUILD_TYPE=Release -DFAMILIAR_BUILD_TESTS=OFF "${strata[@]}" -Wno-dev
  cmake --build "$root/build-docs-native" -j "$(nproc)" --target familiar
  native="$root/build-docs-native/familiar"
fi

site="${FAMILIAR_SITE:-}"
if [ -z "$site" ]; then
  emcmake cmake -S "$root" -B "$root/build-docs-web" -DCMAKE_BUILD_TYPE=Release -DFAMILIAR_WITH_STRATA=OFF \
    -DFAMILIAR_WEB=ON "${strata[@]}" -Wno-dev
  cmake --build "$root/build-docs-web" -j "$(nproc)"
  site="$root/build-docs-web/site"
  mkdir -p "$site"
  cp "$root/build-docs-web/familiar.html" "$site/index.html"
  cp "$root/build-docs-web/familiar.js" "$root/build-docs-web/familiar.wasm" "$site/"
fi

if (exec 3<> "/dev/tcp/127.0.0.1/$port") 2> /dev/null; then
  echo "capture-docs: port $port is already in use; set DOCS_PORT" >&2
  exit 2
fi
python3 -m http.server "$port" --bind 127.0.0.1 --directory "$site" > /dev/null 2>&1 &
server=$!
trap 'kill "$server" 2> /dev/null || true' EXIT
sleep 1

cd "$root/tools/docs-media"
"$darkroom" shoot media.json --force --var "native=$native" --var "base=http://127.0.0.1:$port/" "$@"
ls -l "$root/docs/images"
