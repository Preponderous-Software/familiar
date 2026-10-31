# Playtests for 0.1.1

## AI playtest (2026-10-30)

Three AI agents played the desktop build headless with the
[playtester](https://github.com/Preponderous-Software/playtester) kit, steering by screenshots
in first person, as for [0.1.0](../0.1.0/README.md). Each was also asked to judge, from the screen
alone, the five fixes made since 0.1.0. [ai-playtest/summary.md](ai-playtest/summary.md) is the
merged summary; each session's folder has its `report.json`, `session.json`, every harness command
(`actions.jsonl`), its raw notes (`findings.jsonl`) and the screenshots its findings cite. The
profile used, with the same controls notes as 0.1.0, is in [ai-playtest/profile/](ai-playtest/profile/).

| | |
|---|---|
| Build | `main` at `4d61f43` (after #22, #23, #24), Release, reporting 0.1.0; filed under the next expected version, 0.1.1 |
| Date | 2026-10-30 |
| Machine | 20 cores, WSL2, Mesa llvmpipe; 640×360 windows; three sessions at once |
| Clock | **Real time.** Familiar has no stepped or paused clock (one game minute every three seconds of play), so the night kept running while the agents read screenshots. Sessions were kept short (25-minute budgets; 9 to 15 minutes used), and "ran out of night" findings should be read with that in mind |

| Session | Goal | Seed | Outcome | Fun |
|---|---|---|---|---|
| s01 | The opening of the night as a brand-new player | 31 | partial: skeleton costume, 7 of 8 porches by 9 pm, 2 of 4 lanterns, cinnamon bought; did not reach the orchard | 3/5 |
| s02 | The witchcraft and the cat along the Samhain thread | 7 | partial: 2 doors, 5 candy, 2 of 4 lanterns, cat and back with the bag; lost Smoke's trail | 3/5 |
| s03 | Saves: F5, quit and restart, F9, a stop without quitting | 13 | achieved: nothing lost in three checks | 4/5 |

**Blockers: none.** No crash, no softlock, and no lost or wrong progress: F5 then quit and restart, F9
and a stop without quitting all brought back the clock, costume, candy, lanterns, notebook pages,
position and view (s03). The save format is unchanged and the v3 fixtures and byte-flip tests pass.
That the whole night can be finished is shown by the autopilot playthrough in CI; no session reached
the cider or midnight.

### The five fixes, as the agents saw them

| Fix | s01 | s02 | s03 | Verdict |
|---|---|---|---|---|
| Nana's window lantern (#12): the prompt offers to light it and E does | helped | helped | helped | **Fixed.** Lit 44-47 s into the night in every session, against about 20 game minutes stuck in 0.1.0 |
| Reach and the in-range cue (#13): a step away, orange crosshair, "come closer" | helped | helped | helped | **Fixed.** The "come closer" prompt also shows through a wall (#25) |
| The NEXT line (#17) | helped | helped | helped | **Fixed.** It moved on after every step and read clearly |
| The wayfinding marker (#14) | helped, flawed | mixed | helped | **Helps; follow-ups.** It got every agent to the street's doors, but it points in a straight line (into side walls, and across the creek after dark: #25), stays on the herb bed during the cat's paw-print trail (#26), and its edge arrow and placement need work (#27) |
| The dropped candy bag (#15): glint and label | helped, weak at night | helped | partly | **Better; follow-up.** Found and picked up in every session, but hard to see in the dark or from a few steps away (#28) |

### Findings

26 findings in 21 groups: **2 major, 14 minor, 10 polish** (two of the polish entries, 16 and 17, are
confirmations that fixes work rather than problems).

- **Major: the marker's straight line** leads into an obstacle on the way to the orchard after 9 pm,
  probably the creek, with nothing visible in the dark (s01; finding 1).
- **Major: the cat's paw-print trail.** The marker stays on Nana's herb bed for the whole step, so once
  on the trail it points back to where the trail starts; s02 lost the trail (finding 2). The night can
  still be finished (the prints are drawn, a cat sees Smoke all night, the autopilot finds her); in
  0.1.0 this step had no marker at all.
- Minor: the opening text overlap (all three, still #16), the marker's edge arrow and HUD overlap, the
  bag in the dark, Grandpa Ike's jar not looking lit, save messages that do not say when, and the
  Garcias' front step.

Liked: the lantern beat now lands in the first minute, the in-range cue, the NEXT line, the neighbours
and their jokes, and the notebook.

### What was done about it

Nothing was changed for 0.1.1; it is released with the five fixes, and these findings are open as
issues:

| Issue | Findings |
|---|---|
| #25 | The marker's straight line: side walls, the creek after dark, fences, prompts through walls (1, 13, part of 12) |
| #26 | The cat's paw-print trail and the herb-bed marker (2) |
| #27 | The marker's drawing: edge arrow near the goal, no up arrow, over the HUD and labels (5, 6, 12, 18, 21) |
| #28 | The dropped bag in the dark (4, 11) |
| #29 | Grandpa Ike's lantern: looks unlit, no marker on the porch (9, 10) |
| #30 | Save feedback and the second Esc (7, 20) |
| #31 | The Garcias' front step (8) |
| #16 (open) | Opening screen text, costume line (3, 14) |
| #18 (open) | Notebook overflow and page indicator (15, 19) |
