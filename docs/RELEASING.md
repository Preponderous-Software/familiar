# Releasing

A release is a bare version tag on `main` (`0.1.0`, never `v0.1.0`) with a GitHub release on it.
No release is made by merging; one is cut only when the owner asks for it.

Publishing the release starts [`.github/workflows/release.yml`](../.github/workflows/release.yml),
which builds the game for Windows x64, Linux x64 and the browser and attaches the downloads to the
release. macOS is not built. The arcade is separate: the browser build deploys there on every push
to `main` (`browser.yml`).

## Steps

0. **Playtest the candidate.** Play `main` and commit the notes, and what was changed for them, under
   `docs/playtests/<version>/`. A playtest of an unreleased build is filed under the next expected
   version, with the exact commit in its README.

1. **Set the version.** On a branch, set the version in `project(familiar VERSION ...)` at the top
   of [`CMakeLists.txt`](../CMakeLists.txt) and open a pull request. It is the version the game
   reports (`familiar --version`), and the release workflow refuses a tag that does not match it.

   In the same pull request, make sure the playtest evidence is named for the version being
   released: if `docs/playtests/` holds a directory for a different version, rename it with
   `git mv`. From 0.1.0 on, the release workflow fails a tag whose evidence directory is missing,
   or when any evidence directory is named for a later version.

2. **Create the release on the merged commit.** Pass the full SHA, not a branch name, so the tag
   lands on exactly the commit that was checked:

   ```bash
   git fetch origin
   sha=$(git rev-parse origin/main)
   gh release create 0.1.0 --target "$sha" --prerelease \
     --title "Familiar 0.1.0" --notes-file notes.md
   ```

3. **Wait for the builds.** The `Release` workflow runs on the published release and adds:

   | Asset | Contents |
   | --- | --- |
   | `Familiar-<version>-win-x64.zip` | `familiar.exe`, `README.txt`, `LICENSE.txt`, `STRATA-LICENSE.txt`, `THIRD-PARTY.md` |
   | `Familiar-<version>-linux-x64.tar.gz` | `familiar` and the same four files |
   | `Familiar-<version>-web.zip` | `index.html`, `familiar.js`, `familiar.wasm` and the licences |
   | `*.sha256` | The SHA-256 of each archive |

   If the workflow fails, fix the cause and re-run it (`gh run rerun <id>`); uploads replace assets
   of the same name.

4. **itch.io updates itself.** The workflow's `Push to itch.io` job sends the same web package to
   <https://danthetechman.itch.io/familiar> (channel `html5`, version = the tag) with butler. It
   needs the repository secret `BUTLER_API_KEY` (an itch.io API key, set by the owner:
   `gh secret set BUTLER_API_KEY -R Preponderous-Software/familiar`); without it the job skips
   with a notice. The page's text, cover and visibility are edited on itch.io, not here.

Pushing a version tag without creating a release also works: the workflow builds the tag and
creates a pre-release for it with generated notes.

A pull request that changes the workflow or `CMakeLists.txt`, or a `workflow_dispatch` run with no
tag, builds all three as a dry run: the packages are kept as workflow artifacts and no release is
touched.

## What is in a download

Strata and SDL3 are linked into the executable, and the Windows build links the C++ runtime
statically (`/MT`), so each desktop download is the one executable, [`README-download.txt`](README-download.txt)
as `README.txt`, the licence, and Strata's licence and third-party notices at the pinned commit
(copied from the fetched Strata at build time, so they always match what is linked). The player
needs an OpenGL 4.5 graphics driver on Windows 10/11 x64 or Linux x64, and on Linux glibc 2.34 or
newer (the build runs on Ubuntu 22.04).
