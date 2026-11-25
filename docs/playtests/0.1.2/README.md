# Playtests for 0.1.2

## AI playtest (2026-11-24)

Three AI agents played the desktop build headless with the
[playtester](https://github.com/Preponderous-Software/playtester) kit, steering by screenshots
in first person, as for [0.1.1](../0.1.1/README.md). Each was also asked to judge, from the screen
alone, the fixes made since 0.1.1. [ai-playtest/summary.md](ai-playtest/summary.md) is the
merged summary; each session's folder has its `report.json`, `session.json`, every harness command
(`actions.jsonl`), its raw notes (`findings.jsonl`) and the screenshots its findings cite (as JPEG).
The profile used is in [ai-playtest/profile/](ai-playtest/profile/); its controls notes now list F5
and F9 in the help and say that a click after the first Esc plays on.

| | |
|---|---|
| Build | A trial merge of #36, #37, #38, #39 and #40 onto `main` at `309d2fd` (commit `234260b`, tree `9518189bd957f6d37b04c75e99f44170387d3ca7`). `main` at `f783661`, after those five merged, has the identical tree, so this is the build released as 0.1.2, Release, reporting 0.1.1; filed under the next expected version, 0.1.2 |
| Date | 2026-11-24 |
| Machine | 20 cores, WSL2, Mesa llvmpipe; 640×360 windows; three sessions at once on a heavily loaded machine |
| Clock | **Real time.** Familiar has no stepped or paused clock (one game minute every three seconds of play), so the night kept running while the agents read screenshots. Sessions had 25-minute budgets and used 12 to 18 minutes. Movement and mouse-look were slow at this machine's frame rate (finding 16), so "ran out of night" findings should be read with that in mind |

| Session | Goal | Seed | Outcome | Fun |
|---|---|---|---|---|
| s01 | The opening of the night as a brand-new player | 31 | partial: witch costume, 3 doors, 8 candy, 2 of 4 lanterns by 7:20 pm; stuck fetching the bag at Ms. Bright's door (finding 1) | 3/5 |
| s02 | The witchcraft and the cat along the Samhain thread, following Smoke's trail with the marker | 7 | partial: 3 doors, 8 candy, 2 of 4 lanterns; as the cat followed the prints over the creek to the old oak, found Smoke at 7:47 pm and led her to the ring of stones at 7:57 pm; stopped at 8:07 pm 239 m from the bag | 3/5 |
| s03 | Saves: F5, quit and restart, F9, a stop without quitting | 13 | achieved: nothing lost in four checks | 3/5 |

**Blockers: none.** No crash, no softlock, and no lost or wrong progress. In s03, quitting with Esc, Esc
and restarting, a stop without quitting (like a crash), and a save made as the cat all brought back the
clock, candy, lanterns, form, the bag on the ground and the exact position. F9 went back to the last save
as it should. The costume could not be checked from the screen, because the HUD stops showing it after
Nana's walk (finding 8).

Finding 1 (a bag that could not be picked up) was checked as a possible softlock, because the lantern jar
and basket travel in the bag. It is not one. Loaded into the game's own simulation, s01's last save has
the bag on open ground 3.3 m from Wren. Walking straight at the bag from that spot reaches it, and E
picks it up. Straight walks from 6 m out reach it from most directions, but three of the starting points
south of it, out on the street, stop 5 m short against the porch. So the agent probably walked into the
porch or the door frame and stayed pressed against it, with the door's prompt on screen. That is still a
real problem for a player. Changing back into Wren beside the bag also picks it up.

### The fixes, as the agents saw them

| Fix | s01 | s02 | s03 | Verdict |
|---|---|---|---|---|
| The NEXT marker (#25, #26, #27): routes, the trail, edge arrows, off the HUD | mixed | mixed | mixed | **Better; follow-ups.** It follows the streets and porches. Its edge arrows work, including the new up arrow, and on open ground it leads along Smoke's trail about 8 m ahead (s02 found Smoke with it). It still draws over the prompt line and sits against the NEXT card (4, 20). At the creek bridge it led s02 off the deck into the gully (3). Heading straight at it can leave the prints, and the step then goes back to "find her trail" (5). It jumped from 8 m to 25 m once near Ms. Bright's (12) |
| The dropped bag at night (#28) | mixed | not seen | mixed | **Better as Wren; follow-up.** After dark its "Your candy bag" label can be read from 35 m, and the fetch-bag marker leads back to it. As the cat at dusk, s03 could not see the bag, and nothing marks it until you are Wren again (15). Picking it up next to a door went wrong (1) |
| Grandpa Ike's lantern (#29) | helped | did not help | not seen | **Mixed.** The prompt and E work from the Okafors' door. But it has no marker of its own, and from the porch neither agent could tell which object it was or see that it was lit. The jack-o'-lantern beside it draws the eye (10, 19) |
| Walking straight to doors (#31) | helped | helped | helped | **Fixed.** All three walked straight up to the Garcias', the Okafors' and Ms. Bright's doors |
| HUD, notebook and text (#16, #18, #19) | mixed | mixed | mixed | **Mostly fixed.** The opening screen is clear: the help waits for the greeting, and the costume line goes once a costume is chosen. The notebook fits and shows "Page N of M". Left: the marker and house labels overlap the HUD (4, 20), the cat card wraps to 8 lines (18), and the notebook's "Sources in RESEARCH.md" line (17) |
| Save times (#30) | helped | helped | helped | **Fixed.** Every message gives the time: "Saved at 5:56 pm.", "Saved 5:33 pm" at the top right on an autosave, "Loaded your save from 5:59 pm." and, on a restart, "… The night goes on." The first Esc says "The mouse is free. Esc again saves and quits; click to play on." The autosave note is easy to miss (7), and F9 asks for no confirmation (9) |

### Findings

26 findings in 21 groups: **3 major, 15 minor, 8 polish** (one of the polish entries, 21, is a
positive confirmation that saves work rather than a problem).

- **Major: a bag dropped at a door could not be picked up** (s01; finding 1). After the agent turned into
  the cat at Ms. Bright's door, Wren could not get within reach of the bag. The door's prompt kept E, and
  it said "You need your candy bag… It's where you became a cat." This ended trick-or-treating for the
  run. It happened again after F9. It is not a softlock in the simulation (see above).
- **Major: NEXT never points to the cat thread** (s02; finding 2). For about 50 game minutes NEXT says
  only "Knock where a porch light is on". The paw-print goal appears only once the player presses Q
  without being told to. This is the same design as in 0.1.1, now raised as a problem.
- **Major: the trail at the creek bridge** (s02; finding 3). The marker led off the bridge into a black
  gully. Twice the step went back from "Over the creek… the old oak" to "find her trail". Only short
  steps along the deck got the cat across.
- Minor: the marker over the HUD and prompts, losing the trail when heading straight for the marker, the
  239 m walk back for the bag after the stones, the autosave note, the costume missing from the HUD, F9
  without a confirmation, Ike's lantern, the marker against the bag label and its 8-to-25 m jump, reach
  at lanterns, a stacked message box (the cited shot does not show it clipped), the bag for the cat, and
  slow movement on the software renderer.

Liked: walking straight to doors, the save messages and the Esc line, the clean opening screen, the
notebook's page indicator, the bag's label at night, the jokes at the doors, and following Smoke on open
ground.

### What was done about it

No blocker was found, so 0.1.2 was released on `f783661` with the findings filed for the next pass:

| Issue | Finding |
|---|---|
| #41 (major) | A bag dropped beside a door cannot be picked up: the door prompt takes E |
| #42 (major) | NEXT never points to the cat or the Samhain thread |
| #43 (major) | The paw-print step goes backwards at the creek bridge; the marker leads off the deck |
| #44 | The marker over the prompt line and the NEXT card; the 8-to-25 m jump |
| #45 | The bag cannot be seen as the cat at dusk |
| #46 | Ike's lantern: no marker, and the lit jar is hard to tell apart |
| #47 | The costume leaves the HUD after Nana's walk |
| #48 | F9 has no confirmation or undo |
| #49 | The long walk back for the bag after the stones |
| #50 | "Sources in RESEARCH.md" in the notebook text |

