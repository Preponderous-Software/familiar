# Playtest summary: Familiar 0.1.0 candidate

3 sessions, build familiar None (995271c), generated 2026-10-30 09:48.

**Findings:** 22 reports in 17 groups -- 0 blocker, 5 major, 9 minor, 8 polish.

## Outcomes

| Session | Goal | Outcome | Fun | Minutes | Objectives | Deaths | Findings | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| s01 | Play the opening of the night as a brand-new player who has never seen the game: work out what to do from the screen, choose a costume, trick-or-treat, and follow whatever the game points you to next; note every moment of confusion. | partial | 3 | 15.5 | 0 | 0 | 9 | The writing, the neighbours, the jokes and the notebook pages are charming, and trick-or-treating is fun once you're on the porch. But the opening objective, Nana's window lantern, is the hardest thing in the first act: Nana blocks the window, the E range is tiny and nothing says you're out of range, so I spent ~20 game minutes stuck and lost much of the porch-light window. A 'press E' prompt, a bigger use radius and a way to find Nana's house again would fix most of it. Times in secondsToObjective are wall-clock seconds from pt start (the game has no probe). |
| s02 | Follow the witchcraft and the cat: choose a costume and knock at a door or two, then follow the HUD's NEXT line and Wren's notebook (Tab) toward the Samhain thread: light the lanterns, become the cat (Q) to climb, squeeze through gaps and look for spirits, gather what the cider needs. Get as far along the night's main thread as you can, and note anywhere you could not tell what to do or where to go, and anything that seems stuck. | partial | 3 | 13 | 0 | 0 | 7 | The writing and the knock/joke/lantern beats are lovely: Mrs. Okafor's 'tell us a joke' and the lantern for Grandpa Ike landed, and the notebook pages are rich. But the night is a race against a fast clock in a town with no map, no markers and pitch-black back yards, so most of my time went to being lost; I stalled at 2/4 lanterns looking for Mr. Hale's back step and then for my own candy bag after a cat change. secondsToObjective are wall-clock seconds (no probe), estimated from the in-game clock at ~3 s per game minute. |
| s03 | Test that the night is kept safely: play a few minutes (choose a costume, knock for candy, light a lantern if you find one, turn into the cat and back), noting the clock, candy and lanterns on the HUD. Press F5 and check what it says. Then quit the way a player would (Esc, then Esc again) and run pt stop, and start the game again with exactly the same pt start command: check that the night continues where you left it (clock, candy, costume, lanterns, where you stand). Also try F9 after playing on a little, and a pt stop WITHOUT quitting (like a crash) followed by pt start, and say what was kept. Any lost or wrong progress is a blocker. | achieved | 3 | 11.4 | 0 | 0 | 6 | Saving is solid: F5 shows 'Saved.', quit (Esc, Esc) then restart restored clock, costume, candy, cat/Wren form, the dropped bag, the visited door ('You have been here tonight', no double candy) and the exact position and view; F9 rolled back correctly; and a pt stop without quitting lost nothing (the game evidently saves on SIGTERM; a hard SIGKILL was not tried). No progress was lost in four save/restore checks. The main friction was the very first objective: I could not get an E prompt at Nana's window lantern, so lanterns stayed 0 of 4 and their persistence is untested; secondsToObjective are wall-clock seconds (no probe). |

## Findings

### 1. [major] Nana's window lantern (the first objective) cannot be lit from the walk: Nana stands in front of it and E range is tiny

- **Category:** ux; **reported by** 2 agent(s): s01, s03
- **Also reported as:** "Nana's window lantern (first NEXT objective) never offers an E prompt; E does nothing at the window"
- **Detail:** NEXT says 'Light Nana's window lantern: E at the window left of her door'. Walking up the porch, Nana stands directly in front of that window. With the crosshair on the dark lantern from 3 distances (tap E and a 0.3 s hold), nothing happened: no prompt, no message, Lanterns stayed 0 of 4 for ~20 game minutes. It only lit at 10:03 pm after I came at it from the side of the porch, past Nana. Expected: Nana not blocking the window, a larger use radius, or an 'E: light the lantern' prompt / 'come closer' hint when the crosshair is on it.
- **Repro:** pt start --seed 31; pt key 4; turn ~180 deg (pt look 315 0 x4) to face Nana's porch; pt look -150 0; pt hold w 1.0; aim at lantern in the left window; pt key e (repeat) -> nothing. Later: strafe around Nana's left side to the window and pt key e -> lit.
- **Screenshots:** [lantern_no_response.png](s01/shots/lantern_no_response.png) [lantern_closeup.png](s03/shots/lantern_closeup.png)

### 2. [major] Candy bag left when turning into the cat cannot be found again; NEXT 'Fetch your candy bag' stuck

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** Turned into the cat in a side yard, walked to Ms. Bright's porch, turned back with Q. NEXT became 'Fetch your candy bag from where you changed (E)' and HUD says 'bag on the ground', but the bag has no glint/label/marker and was not visible in ~10 screenshots of scanning the nearby yards over 9:00-9:36. Progress on the main thread stalled there (lantern jar is in the bag). Expected the bag to be marked or a direction hint.
- **Repro:** after lighting 2 lanterns: pt key q (cat) in a yard; pt hold w 5; pt key q; try to find the bag
- **Screenshots:** [nobag.png](s02/shots/nobag.png)

### 3. [major] No in-range prompt for E anywhere; lanterns and doors need you almost touching them

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** The Okafors' porch lantern only lit once it was nearly under my feet and I looked down; pressing E ~1.5 m away with the crosshair on it did nothing. Doors were similar: every knock (Okafors', Ms. Bright's, Novaks') took 2-4 rounds of turning and stepping on the porch. There is no highlight or 'E: knock / light' hint, so a new player cannot tell a miss from a broken interaction.
- **Repro:** At the Okafors' after Mrs. Okafor asks for the lantern: walk to the small dark lantern by the pumpkin at the left porch end; pt key e at ~1.5 m -> nothing; pt hold w 0.6; pt look 0 100; pt key e -> 'You light Grandpa Ike's lantern', Lanterns 1 of 4.
- **Screenshots:** [okafor_lantern_no_response.png](s01/shots/okafor_lantern_no_response.png)

### 4. [major] No way to find your way home: no map, compass or arrow to Nana's house

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** After trick-or-treating four houses I tried to return to Nana's (NEXT was still the window lantern) and got lost: house names only appear when close, NEXT gives no direction, and I wandered to Abbas's Market and the Bandstand before finding Nana's by her purple door at 9:45 pm. Half the night's porch-light window was spent navigating. Expected a 'which way' cue: an arrow, a notebook map page, or a beacon on Nana's house.
- **Repro:** Seed 31: knock Garcias', Okafors', Ms. Bright's, Novaks', then try to walk back to Nana's using only on-screen information.
- **Screenshots:** [lost_no_map.png](s01/shots/lost_no_map.png)

### 5. [major] No way to tell where anything is: no map/compass/marker, NEXT lines give no direction

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** NEXT lines name places ('Nana's window...', 'Mr. Hale's back step', 'Nana's herb bed') but nothing points to them. House labels appear only when close; back yards are pitch black. Walking straight from spawn toward the lit red house led to The Garcias', not Nana's. Later I lost ~40 game minutes behind the Okafors'/Mr. Hale's looking for the back step and herb bed and ended up at Ms. Bright's and the Lindqvists' instead. Expected a minimap, compass or a NEXT line with a relative direction.
- **Repro:** pt start --seed 7; pt key 4; pt hold Shift_L+w 8 (toward the red house ahead) -> it is The Garcias'; then try to find 'Mr. Hale's back step' or 'Nana's herb bed' from the HUD alone
- **Screenshots:** [lost.png](s02/shots/lost.png)

### 6. [minor] HUD covers most of the top-left of a 640x360 view

- **Category:** ux; **reported by** 2 agent(s): s01, s02
- **Also reported as:** "HUD block covers the top-left third of the view (more in cat form)"
- **Detail:** The six-line status block plus NEXT take about half the screen width and the top half of the screen, hiding porches, labels and the lantern I was trying to aim at. F1 hides only the help list, not the HUD.
- **Repro:** pt start; pt key F1; look at any porch.
- **Screenshots:** [start.png](s01/shots/start.png) [cat1.png](s02/shots/cat1.png)

### 7. [minor] Spawn screen: help list overlaps Nana's greeting; costume prompt wraps mid-option and stays after choosing

- **Category:** text; **reported by** 2 agent(s): s02, s03
- **Also reported as:** "Opening screen: Nana's greeting and the controls help list overlap and are unreadable"
- **Detail:** At start the control list is drawn on top of the orange greeting text so both are hard to read. 'Choose a costume: 1 ghost 2 pumpkin 3 / skeleton 4 witch' breaks between '3' and 'skeleton', and the line remains on screen after picking a costume.
- **Repro:** pt start --seed 7; pt shot start; pt key 4; pt shot c4
- **Screenshots:** [overlap.png](s02/shots/overlap.png) [overlap_text.png](s03/shots/overlap_text.png)

### 8. [minor] NEXT stays stuck on Nana's lantern and never mentions the trick-or-treating or the Okafors' lantern request

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** While the first lantern is unlit, NEXT shows only that one step for the whole opening, even after the Garcias' knock and after Mrs. Okafor asks me to light her porch lantern. A player stuck on step 1 gets no hint that knocking on doors is also part of the night.
- **Repro:** Skip Nana's lantern, knock at the Garcias' and Okafors' (tell a joke with T); NEXT still says 'Light Nana's window lantern'.
- **Screenshots:** [garcia_knock.png](s01/shots/garcia_knock.png)

### 9. [minor] Nana's window lantern has no prompt; 'Nana's door' text overrides it and E does nothing until right on top of it

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** At Nana's porch with the crosshair on the window lantern from ~1-2 m, the prompt stayed 'Nana's door. The neighbours' porches are lit' and E did nothing. Walking right up to the lantern and pressing E finally lit it (2 of 4). The Okafors' lantern, by contrast, showed its own 'E light Grandpa Ike's lantern' prompt from the door.
- **Repro:** go to Nana's porch (olive house, red door); aim at lantern in the window left of the door; pt key e -> nothing; pt hold w 0.6; pt key e -> lit
- **Screenshots:** [nanalantern.png](s02/shots/nanalantern.png)

### 10. [minor] Night is too short for the searching required on this renderer

- **Category:** balance; **reported by** 1 agent(s): s02
- **Detail:** ~3 s per game minute plus input lag (mouse-look often applied a beat late, overshooting turns). 5:20-9:36 passed in ~13 wall minutes; I got 2 knocks (Garcias, Okafors) and 2/4 lanterns, never reached Smoke's paw prints or the cider. Porch lights went off at 9 pm with the NEXT line still at 'Knock... 6 to go'. Partly a property of the harness (time runs while screenshots are read), but navigation cost makes it much worse.
- **Repro:** play from start; observe clock vs. progress
- **Screenshots:** [pace.png](s02/shots/pace.png)

### 11. [minor] Orange dialogue and white HUD text are unreadable against lit doors and windows

- **Category:** text; **reported by** 1 agent(s): s01
- **Detail:** When you stand right up to a lit door (which knocking needs), the screen fills with flat orange, and the orange dialogue (joke prompt, Mrs. Okafor's reply) and the white HUD text have very little contrast. Expected a dark backing panel or outline behind the text.
- **Repro:** At the Okafors': pt hold Shift_L+w 2 into the door; pt key e; pt key t.
- **Screenshots:** [joke_contrast.png](s01/shots/joke_contrast.png)

### 12. [minor] Yard around Nana's is too dark to find the herb bed; cat form shows no paw prints there

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** Once the lantern is lit, NEXT asks for rosemary from Nana's herb bed (and as a cat: follow Smoke's glowing paw prints from the herb bed). Around the side and back of the house the screen was almost black, no label appeared, and the cat saw no glowing prints. A dark low box might have been the bed. I ran out of time looking.
- **Repro:** After lighting Nana's lantern, walk around the left side and back of Nana's house at ~10:10 pm; pt key q; turn a full circle.
- **Screenshots:** [herb_bed_dark.png](s01/shots/herb_bed_dark.png)

### 13. [polish] Costume picker banner stays on screen long after a costume is chosen

- **Category:** ux; **reported by** 2 agent(s): s01, s03
- **Also reported as:** "'Choose a costume: 1 ghost 2 pumpkin...' banner stays on screen long after a costume is chosen"
- **Detail:** After pressing 4 (witch) the confirmation shows, but 'Choose a costume: 1 ghost 2 pumpkin 3 skeleton 4 witch' stays mid-screen until you walk off Nana's walk, covering the porch while you try to light the lantern.
- **Repro:** pt start; pt key 4; turn to face the porch.
- **Screenshots:** [e3.png](s01/shots/e3.png) [resume_after_quit.png](s03/shots/resume_after_quit.png)

### 14. [polish] Notebook 'Samhain' page overflows: footer (page/secrets/controls) missing

- **Category:** text; **reported by** 2 agent(s): s02, s03
- **Also reported as:** "Notebook says 'Page 0 of 0' before any page exists"
- **Detail:** The Samhain page text runs to the panel's bottom edge and the 'Page N of 4  Secrets  Left/Right turn  Tab close' footer is not shown; other pages show it.
- **Repro:** light Nana's lantern; pt key Tab; pt key Left (Samhain page)
- **Screenshots:** [samhaincut.png](s02/shots/samhaincut.png) [nb.png](s03/shots/nb.png)

### 15. [polish] 'You have been here tonight' shows at once after your first visit and lingers; Mr. Novak says 'Two each' to one child

- **Category:** text; **reported by** 1 agent(s): s01
- **Detail:** Right after the Garcias' first treat, 'You have been here tonight' appears and it stays as long as you are on the porch (it reads like a refusal). Mr. Novak's 'Two each' implies a group. There are also source tags like [S19] [S21] in the notebook pages, which look like debug citations to a player.
- **Repro:** Knock at the Garcias' or Novaks'; open Tab -> Jack-o'-lanterns page.
- **Screenshots:** [novak_knock.png](s01/shots/novak_knock.png)

### 16. [polish] First Esc gives no 'press Esc again to quit' hint

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** Pressing Esc once frees the mouse but shows nothing on screen; a player would not know a second Esc quits (and saves). The help list does say 'Esc free the mouse' only.
- **Repro:** pt key Escape; pt shot esc1
- **Screenshots:** [esc1.png](s03/shots/esc1.png)

### 17. [polish] Resume after quit has no 'welcome back / the night goes on' message

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** After a normal quit and restart, progress is restored correctly but the only on-screen text is the help list; the crash-restart shows 'Loaded your save.' while the clean-quit restart (first one) showed nothing. A consistent 'Loaded your save' / time line would reassure.
- **Repro:** pt key F5; pt key Escape Escape; pt stop; pt start ... (same); pt shot resume1
- **Screenshots:** [resume_after_quit.png](s03/shots/resume_after_quit.png)

## Objective timings

Game seconds from the start of the world to each objective (event tick / the game's ticks per second, from events.jsonl). Game time, not wall time: with many sessions on one machine a world can run slower than real time, and wall-clock timings would mostly measure the machine.

No objective was completed (or the game has no probe).

