# Familiar

A small open-world game about one Halloween night in a New England hill town, on the
[Strata](https://github.com/Preponderous-Software/strata) engine. You are Wren, twelve, out
trick-or-treating, and helping your grandmother, who is a Wiccan, keep Samhain: lighting lanterns
for the town's beloved dead, brewing her cider, and bringing her old cat's spirit home by midnight.
And tonight you can turn into a cat.

**Status: in development.** See [CONCEPT.md](CONCEPT.md) for the game, [MVP.md](MVP.md) for what
the first playable version holds and its milestones, [RESEARCH.md](RESEARCH.md) for the sources
behind its portrayal of Samhain and Wicca, and [ARCHITECTURE.md](ARCHITECTURE.md) for how it is
built.

## Building

The rules and tests build without a GPU:

```sh
cmake -S . -B build -DFAMILIAR_WITH_STRATA=OFF
cmake --build build -j
ctest --test-dir build
```

### Playing on Windows

`tools/play-familiar.bat` builds and starts the game in one step: download it and double-click it.
It needs Git; it installs CMake and the Visual Studio C++ build tools with winget if they are
missing (then asks to be run again). It clones or updates `main` into `%USERPROFILE%\familiar-play`
(or `%FAMILIAR_PLAY_DIR%`), builds with the newest Visual Studio that has the C++ tools (Visual
Studio 2019 is fine, and is used when a 2022 install lacks C++), and starts the game. The first
build fetches and builds Strata and SDL3 and takes a few minutes; later runs only rebuild what
changed. Extra arguments go to the game, for example `play-familiar.bat --seed 7` or
`play-familiar.bat --tour`.

By hand, from a Developer or plain command prompt:

```bat
cmake -S . -B build -A x64 -DFAMILIAR_BUILD_TESTS=OFF
cmake --build build --config Release --parallel --target familiar
build\Release\familiar.exe
```

Released versions also come as downloads for Windows, Linux and the browser on the
[releases page](https://github.com/Preponderous-Software/familiar/releases), built by
`.github/workflows/release.yml` ([docs/RELEASING.md](docs/RELEASING.md)).

## The loading shell

The page loads behind the Strata web kit's loading shell
(`web/strata-kit/strata-loader.js`, vendored from Strata at the commit in
`web/strata-kit/PIN`; `web/strata-kit/update.sh <sha>` re-vendors it). It shows
the page's own colours (no trailer or poster exists yet; add `poster` and `trailer` to the `StrataBoot.start` call in `web/shell.html` when they do) and real progress with the size ("0.5 of 1.3 MB"). The loader streams
the `.wasm` and hands its bytes to the engine, so nothing downloads twice. If
Familiar can't run (no WebGL 2, the renderer fails, the engine stops, a lost
WebGL context does not come back), the shell says why, with a link to more games.
A device that draws under 12 frames a second is offered that link without
losing the game (not in automated browsers or `?bench` runs).

`web/strata-boot.js` bridges this classic (non-`MODULARIZE`) build to the
loader: emcc's script tag sits inert in `<template id="strata-glue">`, the
bridge starts it, and the engine waits in `Module.instantiateWasm` for the
loader's bytes. After each link, `web/strata_page.cmake` puts the loader and
the bridge inline in the page (so the site is still the same three files) and stamps the download's size on the
template, because the arcade compresses on the fly and sends no
`Content-Length`.

## Licence

MIT; see [LICENSE](LICENSE).
