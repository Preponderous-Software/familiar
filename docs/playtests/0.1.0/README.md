# Playtests for 0.1.0

## AI playtest (2026-10-30)

Three AI agents played the desktop build headless with the
[playtester](https://github.com/Preponderous-Software/playtester) kit, steering by screenshots
in first person, each toward its own goal. [ai-playtest/summary.md](ai-playtest/summary.md) is the
merged summary; each session's folder has its `report.json`, `session.json`, every harness command
(`actions.jsonl`), its raw notes (`findings.jsonl`) and the screenshots its findings cite. Familiar
has no bundled playtester profile yet, so the profile used is kept in
[ai-playtest/profile/](ai-playtest/profile/).

| | |
|---|---|
| Build | `main` at `995271c` (after #11), Release, reporting 0.0.1; no release existed yet, so this is filed under the next expected version, 0.1.0 |
| Date | 2026-10-30 |
| Machine | 20 cores, WSL2, Mesa llvmpipe; 640×360 windows; three sessions at once |
| Clock | **Real time.** Familiar has no stepped or paused clock (one game minute every three seconds of play, a whole night in about 20 minutes), so the night kept running while the agents read screenshots. Sessions were kept short (25-minute budgets), and the timings and "ran out of night" findings should be read with that in mind |

| Session | Goal | Seed | Outcome | Fun |
|---|---|---|---|---|
| s01 | The opening of the night as a brand-new player: costume, trick-or-treat, follow the game | 31 | partial: witch costume, 4 doors, 10 candy, 2 of 4 lanterns | 3/5 |
| s02 | Follow the witchcraft and the cat along the Samhain thread | 7 | partial: 2 doors, 5 candy, 2 of 4 lanterns, cat twice, 5 notebook pages | 3/5 |
| s03 | Saves: F5, quit and restart, F9, a stop without quitting | 13 | achieved: nothing lost in four checks | 3/5 |

**Blockers: none.** No crash, no softlock, and no lost or wrong progress: F5, quit and restart, F9 and a
stop without quitting all brought back the clock, costume, candy, form, dropped bag, visited doors,
position and view (s03). That the whole night can be finished is shown by the autopilot playthrough in
CI, not by this run: no session reached the cider or midnight.

What the agents tripped over (22 findings in 17 groups: 5 major, 9 minor, 8 polish):

- **The first objective.** On Nana's porch the door prompt is shown instead of the window lantern's, Nana
  stands in front of the window, and nothing says Wren is too far away (s01, s02, s03).
- **Reach and feedback.** Doors and lanterns need Wren almost touching them, with no in-range cue (s01).
- **Wayfinding.** No map, compass or marker, and dark back yards; much of each night went to being lost
  (s01, s02).
- **The dropped candy bag** after a cat change could not be found again (s02).
- **Text at 640×360**: the help list over Nana's greeting, the costume line wrapping and lingering, the HUD
  covering the view, low contrast against lit windows; notebook overflow and citation tags; small polish.

Liked by all three: the writing, the neighbours and their jokes, Grandpa Ike's lantern, and the notebook
pages.

### What was done about it

Nothing was changed for 0.1.0; it is released as a pre-release with these findings open as issues:

| Issue | Findings |
|---|---|
| #12 | Nana's window lantern: door prompt wins, Nana in the way (1, 9) |
| #13 | Reach and in-range cue for doors and lanterns (3) |
| #14 | Wayfinding (4, 5, 10, 12) |
| #15 | The dropped candy bag (2) |
| #16 | Opening screen text, costume line, HUD size, contrast (6, 7, 11, 13) |
| #17 | The NEXT line's order (8) |
| #18 | Notebook polish (14, and s03's "Page 0 of 0") |
| #19 | Small text polish (15, 16, 17) |
