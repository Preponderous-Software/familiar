# Playtest summary: run

3 sessions, build familiar None (4d61f43), generated 2026-10-30 20:50.

**Findings:** 26 reports in 21 groups -- 0 blocker, 2 major, 14 minor, 10 polish.

## Outcomes

| Session | Goal | Outcome | Fun | Minutes | Objectives | Deaths | Findings | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| s01 | Play the opening of the night as a brand-new player who has never seen the game: work out what to do from the screen, choose a costume, trick-or-treat, and follow whatever the game points you to next; note every moment of confusion. | partial | 3 | 14.9 | 0 | 0 | 11 | The five fixes mostly help. The lantern prompt and E on Nana's porch (1), reach with 'come closer' and the orange crosshair (2), and the NEXT line (3) all work well. The marker (4) helps but points in a straight line, which is its weakness: it leads into the side walls of houses and, after 9 pm, into an invisible obstacle (likely the creek), and its edge arrow cannot show up or down. The bag (5) works close up but has no marker and is hard to find in the dark. I knocked 7 of 8 porches by 9 pm but lost the 8th to time spent working around houses, and never reached the orchard. secondsToObjective values are wall-clock seconds from pt start (the game has no probe). |
| s02 | Follow the witchcraft and the cat: choose a costume and knock at a door or two, then follow the HUD's NEXT line and Wren's notebook (Tab) toward the Samhain thread: light the lanterns, become the cat (Q) to climb, squeeze through gaps and look for spirits, gather what the cider needs. Get as far along the night's main thread as you can, and note anywhere you could not tell what to do or where to go, and anything that seems stuck. | partial | 3 | 11.8 | 0 | 0 | 7 | Four of the five fixes work: Nana's lantern prompts and lights with E (helped); the in-range cue (orange crosshair, 'come closer to light/knock') is clear (helped); the NEXT line moves on promptly and reads well (helped); the dropped candy bag shows a 'Your candy bag' label, a glint, a 'Fetch your candy bag' NEXT and picks up with E (helped). The wayfinding marker helps for doors (followed it to the Okafors and Garcias) but is mixed overall (no marker for Ike's lantern) and it made the cat paw-print step worse: it points back at the herb bed while you follow the trail, vanishes near the bed with no cue, and in the dark yard I lost the trail and Smoke from 7:16 to 9:12 pm. secondsToObjective are wall-clock seconds from pt start (no probe). |
| s03 | Test that the night is kept safely: play a few minutes (choose a costume, knock for candy, light a lantern if you find one, turn into the cat and back), noting the clock, candy and lanterns on the HUD. Press F5 and check what it says. Then quit the way a player would (Esc, then Esc again) and run pt stop, and start the game again with exactly the same pt start command: check that the night continues where you left it (clock, candy, costume, lanterns, where you stand). Also try F9 after playing on a little, and a pt stop WITHOUT quitting (like a crash) followed by pt start, and say what was kept. Any lost or wrong progress is a blocker. | achieved | 4 | 9 | 0 | 0 | 7 | Saves are safe. Quitting with Esc, Esc and restarting restored clock, candy, costume, lanterns, position and view exactly. A pt stop without quitting (like a crash) at 7:31 restored everything, Candy 8 included, and F9 correctly went back to the last save, though it gives no time and no confirmation. On the five fixes: Nana's porch lantern prompt and E: helped. Reach with glint, orange crosshair and 'come closer': helped. NEXT line moves on and is clear: helped. Wayfinding marker with distance, side arrow and door glow got me to each door: helped, though it often sits under the HUD. Dropped bag: partly helped (NEXT tells you to fetch it and E picks it up), but there is still no glint or label from a distance. Seconds are wall-clock (no probe). |

## Findings

### 1. [major] After 9 pm the marker leads straight into an invisible obstacle on the way to the orchard (likely the creek)

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** Following the cider NEXT goal ('2 apples (the orchard)') with the marker at '16 m v' and later '15 m v', Shift+w for 6-9 s moved Wren nowhere twice; the distance did not change. The scene after 9 pm is nearly black, so nothing shows what blocks the way. A 'The creek bridge' label was visible nearby, so the marker probably points straight across the creek rather than to the bridge. A new player would think the controls broke. Expected: the marker routes via the bridge, or the creek edge is visible at night.
- **Repro:** pt start --seed 31; trick-or-treat until 9 pm near the Whitfields'/Abbas's Market; follow the '< 28 m' orchard marker; hold Shift_L+w toward the 'v' marker
- **Screenshots:** [creek_wall.png](s01/shots/creek_wall.png)

### 2. [major] Cat paw-print trail: wayfinding marker keeps pointing back at Nana's herb bed, NEXT never advances

- **Category:** ux; **reported by** 1 agent(s): s02
- **Also reported as:** "Paw prints and the herb bed are nearly invisible in the dark back yard"
- **Detail:** As the cat, NEXT reads 'Smoke's paw prints glow for a cat: follow them from Nana's herb bed' and the marker points at the herb bed. Walking up to it, the marker vanishes at ~3-5 m but nothing on screen marks the herb bed, no message appears and NEXT does not change. The faint teal prints lead away (toward the church steeple); following them, the marker reappears pointing BACK to the herb bed (17, 33, 66 m), actively pulling the player off the trail. In the dark yard the prints fade after ~20 m and I lost them. Spent 7:16 to 9:12 pm in-game circling. Expected: on reaching the herb bed, a cue ('You catch Smoke's scent...') and the marker moving to the next print / trail end.
- **Repro:** pt start ... --seed 7; key 4; light Nana's lantern; knock Okafors + Garcias; walk to Nana's porch; key q; follow the 'm' marker around the house; observe marker disappears near the bed and then points back once you follow the prints.
- **Screenshots:** [trail_lost.png](s02/shots/trail_lost.png) [cat16.png](s02/shots/cat16.png)

### 3. [minor] Opening screen: Nana's greeting and the help list are drawn on top of each other

- **Category:** text; **reported by** 3 agent(s): s01, s02, s03
- **Also reported as:** "At start the help list is drawn over Nana's greeting"; "Help list overlaps Nana's opening greeting at start"
- **Detail:** At start the orange greeting ('Halloween in Hollowell. Nana waves...') and the white control help overlap in the bottom half, so neither can be read. The costume line also wraps as '3 / skeleton'.
- **Repro:** pt start --seed 31; pt shot start
- **Screenshots:** [overlap_text.png](s01/shots/overlap_text.png) [overlap.png](s02/shots/overlap.png) [overlap.png](s03/shots/overlap.png)

### 4. [minor] Dropped candy bag has no wayfinding marker; in the dark it is easy to lose

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** Fix 5 check. On the Okafors' porch it worked well: the bag was visible, NEXT said 'Fetch your candy bag from where you changed (E)', and E picked it up. In a dark field after 9 pm the bag showed only as a small glow, the 'Your candy bag' label appeared only within ~3 m, and after one step it left the view; it took ~1.5 real minutes to find it again. Every other goal gets a distance marker, but the bag does not.
- **Repro:** after 9 pm in the field: pt key q; pt hold w 4; pt key q; look for the bag
- **Screenshots:** [bag_lost_dark.png](s01/shots/bag_lost_dark.png)

### 5. [minor] Edge arrow near the goal does not respond to turning (stays '4 m >' / '< 5 m' after turning 40-90 degrees)

- **Category:** bug; **reported by** 1 agent(s): s02
- **Detail:** Close to the herb bed the side arrow kept saying '4 m >' after several right turns, and '< 5 m' after turning left; only strafing changed it, then it vanished. Looks like the edge arrow is wrong when the target is close/behind the camera.
- **Repro:** As cat near Nana's back yard: pt look 300 0 (x2) while marker shows '4 m >'; shot.
- **Screenshots:** [cat27.png](s02/shots/cat27.png)

### 6. [minor] Edge arrow only shows < or >, never up, so a target above a tilted-down view sends you turning in circles

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** With the view tilted down, a target straight ahead showed as '15 m >' and then '< 15 m' after a small turn; only tilting up showed the 'v' marker. Expected: an up arrow, or the marker clamped to the top edge.
- **Repro:** look 0 120 (tilt down), then pt look +-150 0 near a goal
- **Screenshots:** [marker_pitch.png](s01/shots/marker_pitch.png)

### 7. [minor] F5 'Saved.', F9 'Loaded your save.' and autosave tell the player nothing about when the save is from

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** Saving and loading worked every time, but F9 at 7:10 pm (Candy 5) silently rolled back to the 6:44 save (Candy 3). It asked for no confirmation, and 'Loaded your save.' did not say what time it went back to. The two-minute autosave showed no cue I could see. Expected something like 'Saved at 6:41 pm' and 'Loaded the 6:41 pm save', or a confirm on F9.
- **Repro:** pt start --profile <profile> --game-dir <game-dir> --seed 13; play; F5 at ~6:41; play on to the Garcias (Candy 5); F9
- **Screenshots:** [f9_rollback.png](s03/shots/f9_rollback.png)

### 8. [minor] Garcias' front step blocks walking straight at the door

- **Category:** bug; **reported by** 1 agent(s): s01
- **Detail:** At '5 m' from the Garcias' door, holding w for 2.5 s did not move Wren (still 'come closer to knock'). Strafing left first, then w, got onto the porch. The same thing happened at Abbas's Market (I came at it from the side).
- **Repro:** approach the Garcias' door head-on from the path; pt hold w 2.5
- **Screenshots:** [stuck_step.png](s01/shots/stuck_step.png)

### 9. [minor] Grandpa Ike's jar lantern still looks unlit after it is lit

- **Category:** bug; **reported by** 1 agent(s): s03
- **Detail:** E lit the Okafors' lantern (HUD went from Lanterns 1 to 2 of 4, with message text), but the jar beside the pumpkin on the porch stayed dark with no flame or glow. It was also lit by E while the crosshair was on the door, with the lantern off screen.
- **Repro:** pt start --profile <profile> --game-dir <game-dir> --seed 13; knock at the Okafors' (T); pt key e; look down at the end of the porch
- **Screenshots:** [bag_picked.png](s03/shots/bag_picked.png)

### 10. [minor] No wayfinding marker for Grandpa Ike's lantern on the Okafors' porch

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** After the joke, NEXT says 'Light Grandpa Ike's lantern on the Okafors' porch' but no 'v' marker or edge arrow appears. The glowing jack-o'-lantern at the right end of the porch gives no prompt and E does nothing; the actual (dark) lantern is at the other end. Once aimed at, the prompt 'E light Grandpa Ike's lantern' and E work fine.
- **Repro:** Knock at the Okafors, press T; look along the porch.
- **Screenshots:** [ike_nomarker.png](s02/shots/ike_nomarker.png)

### 11. [minor] The dropped candy bag has no glint or label until the crosshair is right on it

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** As the cat I walked ~2 m off the Okafors' porch and turned back. The porch was in view, but nothing marked the bag (a small red box), and the HUD said only '(bag on the ground)'. Back as Wren, NEXT said 'Fetch your candy bag from where you changed (E)', which helped. With the crosshair on the box, 'E pick up your candy bag' appeared and E worked (Candy 3 kept). No glint or label was visible from a distance, and no wayfinding marker pointed at the bag.
- **Repro:** pt start --profile <profile> --game-dir <game-dir> --seed 13; on a porch: pt key q; pt hold w 1.5; turn 180; pt key q; look for the bag
- **Screenshots:** [cat_lookback.png](s03/shots/cat_lookback.png)

### 12. [minor] The wayfinding marker and its target hide behind the left HUD block

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** Heading from the Okafors' to the Garcias and then to Ms. Bright, the 'v 17 m' marker sat over the Costume/You are lines of the HUD, so both were hard to read. The direct line to the house was also cut by low fences with no gate hint, and I had to strafe along them. The marker still got me to each door.
- **Repro:** pt start --profile <profile> --game-dir <game-dir> --seed 13; load at the Okafors' porch; turn left ~100 deg; follow the marker
- **Screenshots:** [hud_occludes.png](s03/shots/hud_occludes.png)

### 13. [minor] Wayfinding marker points straight-line at the door, so you hit house side walls and the screen goes black

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** At the Novaks' and the Whitfields', walking at the marker ended with Wren pressed against an unlit side wall and the whole view black (only HUD visible). At the Whitfields' it took about 3 real minutes to find the front porch; the marker also flipped between '10 m >' and '< 12 m' when passing a neighbour's porch. The door prompt 'The Whitfields' door: come closer to knock' showed through the wall. Expected: the marker leads to the porch steps / front path, and no prompt shows through walls.
- **Repro:** pt start --seed 31; after Abbas's Market follow the '16 m' marker to the Whitfields' with Shift_L+w
- **Screenshots:** [whitfields_lost.png](s01/shots/whitfields_lost.png)

### 14. [polish] Costume chooser line keeps showing after a costume is chosen

- **Category:** text, ux; **reported by** 2 agent(s): s01, s02
- **Also reported as:** "'Choose a costume: 1 ghost ...' line stays on screen after the costume is chosen"
- **Detail:** After pressing 3 (skeleton) the line 'Choose a costume: 1 ghost 2 pumpkin 3 skeleton 4 witch' kept reappearing on Nana's walk until the first knock, so it looked as if the choice had not taken.
- **Repro:** pt key 3; walk around Nana's porch
- **Screenshots:** [lit.png](s01/shots/lit.png) [lit.png](s02/shots/lit.png)

### 15. [polish] Notebook page has no page indicator or turn hint and runs to the bottom edge

- **Category:** text; **reported by** 2 agent(s): s01, s03
- **Also reported as:** "The notebook page runs to the bottom edge with no page indicator"
- **Detail:** The Samhain page fills to the bottom edge. Nothing shows whether it continues or that Left/Right turn pages. Source tags like [S1] [S15] read oddly in a game.
- **Repro:** pt key Tab
- **Screenshots:** [notebook.png](s01/shots/notebook.png) [notebook.png](s03/shots/notebook.png)

### 16. [polish] Fix 1+2 check: Nana's lantern prompt and reach work (positive)

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** From about two steps back the prompt said 'Nana's window lantern: come closer to light it'. At the porch it read 'E light Nana's window lantern, for Nana's mother, Ada', the crosshair turned orange, and E lit it (Lanterns 1 of 4). Doors gave 'E knock' with the orange crosshair from about a step away, even when aiming slightly off the door. Grandpa Ike's lantern could be lit from the Okafors' door.
- **Repro:** pt key 3; turn ~130 deg right; walk to the lit window; pt key e
- **Screenshots:** [lantern_prompt.png](s01/shots/lantern_prompt.png)

### 17. [polish] Fix 3 check: the NEXT line moves on after every step (positive)

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** NEXT moved costume -> light Nana's lantern -> knock (8 to go) -> light Grandpa Ike's lantern -> knock (7..1 to go) -> fetch bag (as Wren) / follow Smoke's paw prints (as cat) -> at 9 pm the cider ingredients. Each one is clear and actionable. There was no explicit 'porch lights are out' line at 9 pm.
- **Repro:** play the opening
- **Screenshots:** [nine_pm.png](s01/shots/nine_pm.png)

### 18. [polish] Marker distance and house-name labels overlap each other and the HUD

- **Category:** ux; **reported by** 1 agent(s): s01
- **Detail:** '18 m v' drawn over 'Ms. Bright's'; '16 m'/'5 m' markers drawn over the HUD 'Smoke'/'Basket' values; the 4-line NEXT block sits across the middle-left of the view and overlaps the lantern on Nana's porch. 'You have been here tonight' stays in the prompt line after you turn away from a visited door.
- **Repro:** turn toward a far house so its label and the marker line up
- **Screenshots:** [label_overlap.png](s01/shots/label_overlap.png)

### 19. [polish] Notebook 'Samhain' page overflows; footer missing

- **Category:** text; **reported by** 1 agent(s): s02
- **Detail:** Page 1 text runs to the bottom edge and the 'Page x of 4 / Left/Right turn / Tab close' footer seen on page 2 is not shown.
- **Repro:** After lighting lanterns, key Tab; shot
- **Screenshots:** [nb1.png](s02/shots/nb1.png)

### 20. [polish] The first Esc gives no hint that a second Esc quits

- **Category:** ux; **reported by** 1 agent(s): s03
- **Detail:** After one Esc the screen looked the same as before: no 'mouse freed / Esc again to quit (saves)' line. The quit itself saved correctly.
- **Repro:** pt start --profile <profile> --game-dir <game-dir> --seed 13; pt key Escape; pt shot
- **Screenshots:** [resume_quit.png](s03/shots/resume_quit.png)

### 21. [polish] Wayfinding marker drawn over the HUD stat block / NEXT text

- **Category:** ux; **reported by** 1 agent(s): s02
- **Detail:** When the goal is up-left the 'v 6 m' marker overlaps Basket/Smoke lines; side arrows '< 16' overlap the prompt line. Also clips at the top edge when looking down.
- **Repro:** Walk toward the Garcias' door from the street.
- **Screenshots:** [marker_hud.png](s02/shots/marker_hud.png)

## Objective timings

Game seconds from the start of the world to each objective (event tick / the game's ticks per second, from events.jsonl). Game time, not wall time: with many sessions on one machine a world can run slower than real time, and wall-clock timings would mostly measure the machine.

No objective was completed (or the game has no probe).

