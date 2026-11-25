# Playtest summary: ai-playtest

3 sessions, build familiar None (234260b), generated 2026-11-24 19:31.

**Findings:** 26 reports in 21 groups -- 0 blocker, 3 major, 15 minor, 8 polish.

## Outcomes

| Session | Goal | Outcome | Fun | Minutes | Objectives | Deaths | Findings | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| s01 | Play the opening of the night as a brand-new player who has never seen the game: work out what to do from the screen, choose a costume, trick-or-treat, and follow whatever the game points you to next; note every moment of confusion. | partial | 3 | 16.5 | 0 | 0 | 10 | The opening reads well: the costume line, Nana's greeting, NEXT text and jokes at the door make it clear what to do, and 3 doors, 2 lanterns and 8 candy came quickly. The run stalled on a bug: after turning into the cat at Ms. Bright's door, the dropped bag could not be picked up again because the door's 'You need your candy bag' prompt takes E even from 2 m away. That ended trick-or-treating at 7:20 pm, well before porch lights go off at 9, so time was not the cause. Fix verdicts: marker MIXED (follows streets and porches and shows edge arrows, but often draws over the HUD and prompt text, and on the paw-print step following it drops you off the trail); bag MIXED (the bag and its 'Your candy bag' label are easy to see at night, but it cannot be picked up when dropped by a door); Ike's lantern HELPED (lit from the door, visible from the street later, no marker needed); doors HELPED (walked straight up to the Garcias' door); HUD/text MIXED (opening screen and notebook paging are good, but there are marker/label overlaps, a clipped stacked message box and an ugly cat card wrap); save times HELPED (the autosave, F5 and F9 all show the time; the Esc quit flow and restart were not tested). secondsToObjective is wall-clock seconds, as there is no probe. |
| s02 | Follow the witchcraft and the cat: choose a costume and knock at a door or two, then follow the HUD's NEXT line and Wren's notebook (Tab) toward the Samhain thread: light the lanterns, become the cat (Q) to climb, squeeze through gaps and look for spirits, gather what the cider needs. On the cat's step, follow Smoke's paw-print trail with the NEXT marker and say how well it leads you. Get as far along the night's main thread as you can, and note anywhere you could not tell what to do or where to go, and anything that seems stuck. | partial | 3 | 17.7 | 0 | 0 | 9 | Got Wren (witch) through Nana's lantern, three doors (8 candy, Ike's lantern) and, as the cat, along Smoke's paw prints to the old oak and up Lantern Hill to the ring of stones by 7:57 pm; stopped at 8:07 with NEXT asking for the candy bag 239 m away, so cider and the remaining lanterns were not reached (time budget, partly harness perf). The biggest problem is discoverability and the creek: NEXT never points to the cat thread until you press Q yourself, and near the creek bridge the marker led me off the deck into a black gully and the step went backwards twice. Fix verdicts: marker MIXED (routes via streets, edge arrows incl. up, and follows the trail well on open ground, but overlaps prompts/labels and fails at the bridge); bag NOT SEEN (never got back to it); Ike's lantern DID NOT HELP (no marker, could not see it lit); doors HELPED (walked straight up to the Okafors', Ms. Bright's and the Garcias'); HUD/text MIXED (clean opening screen and notebook with page indicator, but overlaps and awkward wraps); save times HELPED (autosave 'Saved 5:33 pm', F5 'Saved at 8:01 pm', F9 'Loaded your save from 8:01 pm', first Esc explains the second). secondsToObjective are wall-clock seconds (no probe). |
| s03 | Test that the night is kept safely: play a few minutes (choose a costume, knock for candy, light a lantern if you find one, turn into the cat and back), noting the clock, candy and lanterns on the HUD. Press F5 and check what it says. Then quit the way a player would (Esc, then Esc again; note what the first Esc says) and run pt stop, and start the game again with exactly the same pt start command: check that the night continues where you left it (clock, candy, costume, lanterns, where you stand) and what the game says about it. Also try F9 after playing on a little, and a pt stop WITHOUT quitting (like a crash) followed by pt start, and say what was kept. Judge whether each save and load message tells you when the save is from, and watch for the autosave. Any lost or wrong progress is a blocker. | achieved | 3 | 12.3 | 0 | 0 | 6 | The night is kept safely: quitting with Esc Esc, a pt stop without quitting (crash) and a save as the cat all restored clock, candy, lanterns, form and exact position, and every save/load message names the game time ('Saved at 5:56 pm.', 'Loaded your save from 6:27 pm. The night goes on.'); the only gaps are that the costume vanishes from the HUD so its restore cannot be checked, F9 has no confirmation, and the autosave toast is easy to miss. Fix verdicts: marker mixed (edge arrows work and the marker routed along streets, but it overlapped a prompt and sat against the HUD), bag mixed (good 'fetch your bag' marker as Wren, but the bag is invisible to the cat at dusk), Ike's lantern not seen, doors helped (walked straight up to the Garcias' and Ms. Bright's doors), HUD/text mixed (notebook fits with 'Page 1 of 5', opening screen is clear, minor overlaps), save times helped. secondsToObjective are wall-clock seconds (no probe). |

## Findings

### 1. [major] Candy bag dropped at a door cannot be picked up again: the door steals E

- **Category:** bug; **reported by** 1 agent(s): s01
- **Detail:** I turned into the cat (Q) right at Ms. Bright's door after knocking. Coming back as Wren at 7:20 pm, the marker sat on the bag at 2-3 m, but I could not get any closer and the prompt was the door's 'You've had your treat here'. Every E gave 'You need your candy bag to trick-or-treat. It's where you became a cat.', which tells you to go where you already are. Approaching from the other side and from the front steps, and again after F9, gave the same result. I expected an 'E pick up your candy bag' prompt within reach. Because players naturally press Q where they stand, a door is a very likely place to drop the bag.
- **Repro:** pt start ... --seed 31; key 4; light Nana's lantern; knock Okafors, Garcias, Ms. Bright's (T for jokes); at Ms. Bright's door press q; walk away a little; press q; walk back to the bag on her porch; key e
- **Screenshots:** [r21.jpg](s01/shots/r21.jpg)

### 2. [major] Cat/Samhain thread is never pointed to by NEXT until the player presses Q unprompted

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** From 5:26 to 6:13 pm NEXT only said 'Knock (E) where a porch light is on: N to go'. The paw-print goal ('Smoke's paw prints glow for a cat: follow them from Nana's herb bed') appeared only after I pressed Q on my own initiative. A player who follows NEXT alone would knock doors until 9 pm and might never learn the main thread exists. Expected NEXT (or Nana's greeting) to say 'try Q' / 'become the cat at the herb bed' once a door or two is done.
- **Repro:** pt start --profile .../prof/familiar.json --game-dir .../gamedir --seed 7; pt key 4; light Nana's lantern (E), knock 3 doors; watch NEXT; then pt key q
- **Screenshots:** [catnext.jpg](s02/shots/catnext.jpg)

### 3. [major] Paw-print step regresses near the creek bridge; marker leads off the bridge into a dark gully

- **Category:** bug; **reported by** 1 agent(s): s02
- **Detail:** Twice, right after NEXT changed to 'Over the creek, Smoke's paw prints lead to the old oak' (marker 6-13 m ahead), following the marker/prints for 4-6 s dropped me into the pitch-black creek gully or back onto the near bank, and NEXT reverted to 'Smoke's scent goes off toward the creek: follow her paw prints east' or 'find her trail, then follow it east' (marker jumped to 24 m). Only slow 3 s steps that kept to the bridge deck got me across. Expected the marker to stay on the bridge and the step not to go backwards once over the creek.
- **Repro:** pt start --profile .../prof/familiar.json --game-dir .../gamedir --seed 7; pt key 4; pt key q at the Garcias'; follow NEXT marker from Nana's herb bed east to the creek bridge; hold Shift_L+w 4-6 s toward the marker after 'Over the creek' appears
- **Screenshots:** [regress.jpg](s02/shots/regress.jpg)

### 4. [minor] NEXT marker and edge arrow are drawn over the HUD card and the prompt line

- **Category:** text, ux; **reported by** 3 agent(s): s01, s02, s03
- **Also reported as:** "Edge-arrow marker draws over the interaction prompt line"; "NEXT edge arrow overlaps the prompt line after lighting Nana's lantern"; "Off-screen 'up' marker arrow sits against the HUD NEXT card"
- **Detail:** The marker label often lands on the text: '3 m' inside 'Nana's window lantern:3come closer'; the '20 m >' edge arrow on top of the end of 'The neighbours' porches are lit'; '10 m'/'^ 8 m' over the NEXT card text; the '>' arrow over 'Knock first' at the Okafors. House labels ('Ms. Bright's', 'Nana's house', 'Your candy bag') also overlap the HUD card's bottom lines. I expected the marker and labels to keep clear of the HUD and the prompt.
- **Repro:** key 4; look right ~400 px; hold w 3 toward Nana's window lantern; then light it and look at the right edge
- **Screenshots:** [mk_overlap.jpg](s01/shots/mk_overlap.jpg) [overlap1.jpg](s02/shots/overlap1.jpg) [overlap.jpg](s03/shots/overlap.jpg) [markerhud.jpg](s03/shots/markerhud.jpg)

### 5. [minor] Following the cat-trail marker takes you off the paw prints and NEXT flips back to 'find her trail'

- **Category:** ux; **reported by** 2 agent(s): s01, s02
- **Also reported as:** "Paw-print trail is easy to lose: NEXT flips to 'find her trail' after a few seconds of hurrying"
- **Detail:** On the paw-print step the prints show as a dotted glowing line and the marker sits on the trail, which is good. Twice, though, running straight at the marker ('^ 49 m') made NEXT revert to 'Smoke's paw prints glow for a cat: find her trail, then follow it east' and moved the marker to an edge arrow. The marker seems to aim at a far point on the trail instead of leading along the prints, and the step text changes without saying why.
- **Repro:** As the cat at Ms. Bright's: follow the marker to Nana's herb bed, then hold Shift_L+w toward the marker for ~6 s
- **Screenshots:** [c15.jpg](s01/shots/c15.jpg) [losttrail.jpg](s02/shots/losttrail.jpg)

### 6. [minor] After leading Smoke to the stones, NEXT sends Wren 239 m back for the candy bag

- **Category:** balance; **reported by** 1 agent(s): s02
- **Detail:** Turning back into Wren on Lantern Hill gives 'Fetch your candy bag from where you changed (E)', marker 239 m. With the clock past 8 pm, that is a long backtrack (and at this harness's walking speed, minutes of real time). The previous NEXT as the cat at the stones ('Climb, slip through gaps, meet spirits. Q to be Wren') also had no marker. Expected either a hint before changing that the bag stays behind (it was said once in the cat toast) or a nearer pickup.
- **Repro:** pt start --profile .../prof/familiar.json --game-dir .../gamedir --seed 7; pt key 4; reach the stones with Smoke, pt key q
- **Screenshots:** [f9.jpg](s02/shots/f9.jpg)

### 7. [minor] Autosave toast is easy to miss; only one of at least two autosaves was seen

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** The first autosave showed 'Saved 5:33 pm' in the top right about two minutes in (good: it names the time). A later autosave did happen (the crash test lost nothing), but screenshots every 5-6 s for two and a half minutes never caught a toast. Expected the autosave toast to stay up a few seconds so a player can notice it.
- **Repro:** pt start ...; play ~2 min; take pt shot every 5 s
- **Screenshots:** [autosave1.jpg](s03/shots/autosave1.jpg)

### 8. [minor] Costume disappears from the HUD after leaving Nana's walk, so a restored costume cannot be checked

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** The HUD showed 'Costume pumpkin' after pressing 2, but the line was gone a few seconds later on the street (before any knock) and never came back; the notebook's first page did not show it either. After every restart I could confirm clock, candy, lanterns and position but had no on-screen way to tell whether the pumpkin costume was kept. Expected the costume to stay on the HUD or be shown in the notebook / load message.
- **Repro:** pt start ...; pt key 2; walk toward the Garcias' (pt hold Shift_L+w 5); pt shot; Esc Esc; pt stop; pt start ...; pt shot
- **Screenshots:** [restart_quit.jpg](s03/shots/restart_quit.jpg)

### 9. [minor] F9 loads at once with no confirmation, silently undoing play since the last save

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** After the restart I played on to Ms. Bright's (candy 2 to 5, 5:59 to 6:15) and pressed F9: it immediately jumped back to 5:59 with candy 2 at the Garcias' door. The message 'Loaded your save from 5:59 pm.' is clear about when the save is from, but there is no 'are you sure' and no undo; a stray F9 costs a real chunk of a 20-minute night.
- **Repro:** pt start ...; knock at Ms. Bright's (E, T); pt key F9; pt shot
- **Screenshots:** [f9.jpg](s03/shots/f9.jpg)

### 10. [minor] Ike's lantern on the Okafors' porch has no visible marker and does not visibly light

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** After the joke, the 'E light Grandpa Ike's lantern' prompt was already active at the door, so I lit it without ever seeing it. Stepping back along the porch, I could not tell which object was Ike's lantern or see anything newly lit (the wall lantern above the door still looked dark).
- **Repro:** pt start --profile .../prof/familiar.json --game-dir .../gamedir --seed 7; pt key 4; walk to the Okafors' door, pt key e, pt key t, pt key e, pt hold s 1.5, look along the porch
- **Screenshots:** [ike2b.jpg](s02/shots/ike2b.jpg)

### 11. [minor] Marker directions conflict with the 'Your candy bag' label when walking back for the bag

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** At 7:03 pm the 'Your candy bag' label was straight ahead across the street, but the marker showed '< 35 m' at the left edge. While I followed it, it flipped between left and right edge arrows, and the distance once grew from 35 to 42 m. Walking straight toward the label, I was stopped by something unseen at the street edge for ~10 s of holding W. When I came at it from another angle, the marker did lead to the porch. It may be routing around the creek, but nothing on screen shows why.
- **Repro:** After the cat trail, press q at ~6:50 pm and follow NEXT 'Fetch your candy bag'
- **Screenshots:** [r8.jpg](s01/shots/r8.jpg)

### 12. [minor] Marker jumps from 8 m to 25 m near Ms. Bright's without any goal change

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** Heading to Ms. Bright's ('8 m'), a few steps around the blue house made the marker jump to '25 m' pointing past the house. The NEXT text did not change, so a player cannot tell whether the goal or the route changed.
- **Repro:** After the Garcias, follow the marker to Ms. Bright's around the blue house
- **Screenshots:** [mk_jump.jpg](s01/shots/mk_jump.jpg)

### 13. [minor] Reach for lanterns and doors is unclear: 'come closer' persists at '3 m' while W seems to stop

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** At Nana's window lantern, and again at the Garcias' and Ms. Bright's doors, the prompt stayed on 'come closer to ...' with the marker at 3 m while holding W, with the crosshair on the target. It flipped to 'E ...' only after strafing or turning. The step itself did not block me at the Garcias' (#31 looks fixed), but the reach distance feels inconsistent.
- **Repro:** key 4; turn right to the window lantern; hold w 1.2, then hold w 2; read the prompt
- **Screenshots:** [mk_overlap.jpg](s01/shots/mk_overlap.jpg)

### 14. [minor] Stacked message box after the Okafors' joke runs off the bottom edge

- **Category:** text; **reported by** 1 agent(s): s01
- **Detail:** Telling a joke stacks 6 lines (the joke, the reply, two 'new notebook page' lines) and the last line sits flush on the bottom edge with its box cut off. It also pushes the prompt up into the middle of the screen.
- **Repro:** Knock at the Okafors (E), then press t
- **Screenshots:** [msg_clip.jpg](s01/shots/msg_clip.jpg)

### 15. [minor] The dropped candy bag is hard to find as the cat at dusk; no marker for it until you turn back into Wren

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** Turned into the cat on Ms. Bright's front walk (6:30 pm), walked back a couple of steps, turned around and looked down: the bag was not visible anywhere (the ground is very dark and the cat's camera is low, close to the jack-o-lanterns). The HUD says '(bag on the ground)' but there is no marker for it while you are the cat. Once back as Wren, NEXT says 'Fetch your candy bag from where you changed (E)' with a 2 m marker, which works, though the marker sat on a jack-o-lantern and I had to walk into it about 3 s before E picked up the bag.
- **Repro:** pt start ...; at Ms. Bright's walk: pt hold s 1.5; pt key q; pt hold s 1.5; turn around (pt look 300 0 x4) and pt look 0 80; pt shot
- **Screenshots:** [nobag.jpg](s03/shots/nobag.jpg)

### 16. [minor] Very slow movement and dropped mouse-look on the shared software renderer

- **Category:** perf; **reported by** 1 agent(s): s02
- **Detail:** Holding Shift+W for 10 s moved Wren ~12 m (marker 239->227 m); some 150-250 px look nudges turned only ~10 degrees. Likely the harness (llvmpipe, several sessions), but it made the cat trail and the bag backtrack much slower.
- **Repro:** pt start --profile .../prof/familiar.json --game-dir .../gamedir --seed 7; pt key 4; pt hold Shift_L+w 10 and compare marker distance
- **Screenshots:** [slow.jpg](s02/shots/slow.jpg)

### 17. [polish] Notebook page cites 'Sources in RESEARCH.md'

- **Category:** text; **reported by** 2 agent(s): s01, s02
- **Also reported as:** "Notebook shows developer-facing 'Sources in RESEARCH.md: S1, S2...' line"
- **Detail:** The Samhain page ends 'Sources in RESEARCH.md: S1, S2, S3, S4, S5, S15', which points the player at a repository file they cannot see. The page itself fits well, and the 'Page 2 of 5 / Secrets 0 of 8' indicator is clear.
- **Repro:** After lighting Nana's lantern press Tab, then Right
- **Screenshots:** [nb2.jpg](s01/shots/nb2.jpg) [nb2b.jpg](s02/shots/nb2b.jpg)

### 18. [polish] Cat HUD card wraps mid-phrase and grows to 8 lines

- **Category:** text; **reported by** 1 agent(s): s01
- **Detail:** As the cat, the card reads 'Candy 8 (bag on the / ground)' and 'You are a black cat (Q: / Wren)', so it covers the top-left third of the screen. Shorter wording ('bag dropped', 'Q: back to Wren' on its own line) would fit.
- **Repro:** Press q anywhere
- **Screenshots:** [cat2.jpg](s01/shots/cat2.jpg)

### 19. [polish] Hard to confirm Grandpa Ike's lantern is lit from the door; no separate marker

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** Ike's lantern lights straight from the Okafors' door prompt ('E light Grandpa Ike's lantern') with no walk and no marker. Looking down the porch right after, I could not pick it out. Later, from the street, a lit lantern at the porch end was visible. The success message is clear ('Mrs. Okafor ... puts her hand to the glass').
- **Repro:** Knock at the Okafors, t, e; look down the porch
- **Screenshots:** [ike_lit.jpg](s01/shots/ike_lit.jpg)

### 20. [polish] House-name labels overlap the NEXT card

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** 'Mr. Hale's' label drawn inside the NEXT text ('follow her paw Mr. Hale's'); 'The creek bridge' label likewise drawn across the top HUD card. Labels are not kept clear of the HUD.
- **Repro:** pt start --profile .../prof/familiar.json --game-dir .../gamedir --seed 7; pt key 4; as the cat on the paw-print trail near Mr. Hale's
- **Screenshots:** [labelov.jpg](s02/shots/labelov.jpg)

### 21. [polish] Save/load behaviour is solid: every message names the game time (positive)

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** F5 'Saved at 5:56 pm.'; first Esc 'The mouse is free. Esc again saves and quits; click to play on.'; restart after Esc Esc 'Loaded your save from 5:59 pm. The night goes on.' with candy 2, lanterns 1/4, same spot and view; pt stop without quitting then restart 'Loaded your save from 6:27 pm. The night goes on.' with candy 5 and the same spot; saving as the cat restores the cat with the bag still on the ground. No progress lost in any check.
- **Repro:** see notes in findings.jsonl
- **Screenshots:** [crash_kept.jpg](s03/shots/crash_kept.jpg)

## Objective timings

Game seconds from the start of the world to each objective (event tick / the game's ticks per second, from events.jsonl). Game time, not wall time: with many sessions on one machine a world can run slower than real time, and wall-clock timings would mostly measure the machine.

No objective was completed (or the game has no probe).

