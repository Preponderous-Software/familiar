# Architecture

Familiar is built the way the other Preponderous games on Strata are built, and most of its
structure is copied from them on purpose (see [Reused](#reused)).

## Layers

| Layer | Directory | Depends on | What it is |
|---|---|---|---|
| Rules | `src/sim` | nothing | The town's layout, walking and climbing, the clock, and (from M2) everything that happens in the night. Deterministic: no engine, no clocks, no `std::sin` |
| Geometry | `src/bridge/geometry.*` | rules | The town as triangle meshes and a terrain grid, built from the same layout the rules use |
| Scene | `src/bridge/scene.*` | geometry, Strata | Uploads the meshes and keeps the changing ones (gates, figures, lanterns) in step with the world |
| Play | `src/play` | scene, Strata | The camera, the sky, the lights, the HUD, input and the game session |
| Saves | `src/save` (M5) | rules | A checksummed save format, written atomically, with the previous save kept |
| Sound | `src/audio` (M5) | rules | Cues and ambience from the world, a synthesiser, and a miniaudio device |
| Apps | `app/` | play | `main.cpp` (desktop: play, `--tour`, `--autopilot`) and `web_main.cpp` (browser, M6) |

The rules build and test without a GPU in seconds (`-DFAMILIAR_WITH_STRATA=OFF`). CI builds them
on Linux and Windows and runs the tests on both.

## Determinism

The rules are compiled without fused multiply-adds (`-ffp-contract=off`, `/fp:precise`) and use
their own sine, cosine and arctangent built from `+ - * /` (`src/sim/math.h`), so the same inputs
give bit-identical worlds on every compiler. `fingerprint()` hashes the whole `World`; tests pin
the fingerprints of scripted runs and of the autopilot's playthrough.

## The town and climbing

`src/sim/town.*` holds one fixed layout. Every solid thing is a `Solid`: a box or an upright
cylinder from the ground up to a top that is flat or a gable roof. A body may step onto a solid
whose top is no more than its climb above its feet (Wren 0.5 m: a porch, not a fence; the cat
1.6 m: a fence or a rain barrel, then a shed from the barrel, then a roof from the shed), and stands
on the highest solid it overlaps. Anything taller stops it. Gaps are just gaps: the hedge's gap is
narrower than Wren and wider than the cat. Gates are solids that stop counting once opened.

## The cat

`src/sim/cat.*` holds the rest of what being a cat changes (M3). A cat cannot carry the candy bag,
so it stays where Wren changed (`World::bag`) until she picks it up, by changing back beside it or
with E; Wren cannot trick-or-treat without it, and a cat at a door only gets a treat for the kitty.
Mr. Hale's gate and the lychgate open only from the inside, with E as a cat within reach of the
latch. The spirits (three in the churchyard and the lamplighter on the hill path) are, like the
children, a function of the clock alone; only the cat sees them, and `World::spiritsSeen` records
each one she has met. They are the game's fairy tale, labelled as such on the page, and gentle,
per the [sensitivity notes](RESEARCH.md#sensitivity-notes).

## Reused

| From | What |
|---|---|
| [Atman](https://github.com/Preponderous-Software/atman) | The layering, `math.h` (deterministic trig), the village-style layout and collision, the tick/Input/fingerprint pattern, the HUD with its NEXT line and bearings, doorway lamps as point lights, the real sun from date and place, the synthesised sound, the save format, the browser shell with touch controls and IndexedDB saves, CI and the play script |
| [Kindling](https://github.com/Preponderous-Software/kindling) | The save format Atman took from it (magic, version, length, payload, SHA-256), the headless tour and autopilot playthrough with pinned fingerprints, the release workflow |
| [Strata](https://github.com/Preponderous-Software/strata) | The engine: terrain tile, static and unlit meshes, translucent meshes for spirits, point lights, the solar position, overlays and text, the drive channel for test harnesses |
