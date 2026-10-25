# MVP

The MVP is the whole night in [CONCEPT.md](CONCEPT.md), small: **one town, one night, both forms,
trick-or-treating, the four family lanterns, Nana's cider and Smoke brought home at midnight,**
playable before Halloween 2026. If walking Elm Street in the dark, knocking on doors and slipping
over a fence as a cat feels like being out on Halloween, the rest is more of it.

## In scope

| Area | The MVP has |
|---|---|
| Place | Hollowell: Elm Street and its houses, the green, the market, the church and graveyard, the creek and bridge, the orchard, the woods' edge, Lantern Hill |
| Time | 31 October 2026, 5:20 pm to midnight and on, with the sun on its real path for 42.4 N, 72.5 W |
| Player | First person as Wren, or as the cat (Q); walk, hurry, look, use (E), trick (T) |
| Trick-or-treat | Eight houses and the shop, porch lights, four costumes, candy as currency, tricks |
| Witchcraft | The lantern jar, four family lanterns, three ingredients, the cider, the offering |
| Cat | Climbing fences, sheds and roofs; slipping through gaps; nosing up latches; seeing spirits; the dropped bag |
| Spirits | Smoke and her trail, three gentle spirits in the churchyard, the lamplighter on the hill |
| Notebook | Pages on Samhain, the Wheel of the Year, the Rede, lanterns, guising, jack-o'-lanterns, black cats, and the eight secrets, with sources |
| Sound | Synthesised, as in Atman: wind, leaves, the creek, an owl, footsteps, knocks, the bag, a purr, the church bell |
| Saves | Save and continue anywhere; a damaged save is refused and the previous one loads |
| Autopilot | Plays the whole MVP from a new game to Smoke's crossing, on a pinned fingerprint |
| Platform | Windows and Linux desktop; a browser build with touch controls (built and tested, not deployed) |

## Out of scope

Deferred, not rejected:

- Interiors. Doors open onto lit hallways and neighbours, but nobody goes in.
- Voice, music and recorded sound.
- More costumes, more houses, a second night.
- Publishing the browser build anywhere, a release, and listing the game on any site: decisions
  for the owner.
- A practising Wiccan's review of the writing (see
  [RESEARCH.md](RESEARCH.md#sensitivity-notes)). Required before the game is promoted, not before
  it is playable.

## What "done" means

| Step | Done when |
|---|---|
| Night | The sun sets on its real path, it gets properly dark, and porch lights, streetlamps and jack-o'-lanterns light the street. |
| Explore | A player can walk from Nana's porch to every landmark, and sees landmarks from a distance by their lights. |
| Trick-or-treat | Knocking at a lit porch (E) brings the neighbour to the door, who gives candy; some ask for a trick (T); costumes change what some give; houses give once; porch lights go off at 9 pm. |
| Cat | Q turns Wren into a cat and back. The cat climbs onto a fence, a shed and a roof, and fits through a gap that Wren cannot. The bag and jar stay where Wren changed until she picks them up. Turning back is refused where a girl would not fit. |
| Lanterns | Each family lantern can be lit, and Mr. Hale's and the lychgate's need the cat to open the way first. |
| Cider | Two apples, rosemary and a cinnamon stick bought with candy brew into cider at the pot. |
| Smoke | As a cat the player sees Smoke's trail, finds her, and leads her to the circle. At midnight, with the offering made and the lanterns lit, she goes home. |
| Notebook | Every page about belief or history cites a source that exists in RESEARCH.md. |
| Saves | Quitting and reloading returns to the same place and time. A save with any one byte flipped is refused with a message, and the previous save loads. |
| Autopilot | `--headless --autopilot` plays the whole MVP and ends on the pinned fingerprint. |
| Browser | The page builds, its tests pass as wasm, and a desktop and a phone profile can walk, look, knock and change form in headless Chromium. |

## Milestones

Each milestone is one pull request, merged when every check is green.

| Milestone | Scope |
|---|---|
| M0 | Docs (this set), the rules library's skeleton, CI on Linux and Windows |
| M1 Night town | Terrain, Elm Street, houses, landmarks, walking with collision, the real sun, porch lights and streetlamps, the HUD, the headless tour |
| M2 Trick-or-treat | Costumes, knocking, neighbours, tricks, candy, the shop, porch-light hours, children in the street |
| M3 Cat | The cat form, climbing, gaps, latches, the dropped bag, seeing spirits |
| M4 Witchcraft | The lantern jar, family lanterns, ingredients, the cider, Smoke and the offering, the notebook |
| M5 Saves and autopilot | Checksummed saves, the autopilot's playthrough on a pinned fingerprint, synthesised sound |
| M6 Browser | The WebGL 2 build with touch controls and IndexedDB saves, smoke-tested in CI (not deployed) |
| M7 Windows | `tools/play-familiar.bat` and a release workflow (no release cut) |
