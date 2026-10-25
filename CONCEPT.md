# Concept

Familiar is a small open-world game about one Halloween night in a New England hill town. You are
Wren, twelve years old, out trick-or-treating in a costume of your choosing. Your grandmother,
Nana Rowan, is a Wiccan, and for her this night is Samhain: the turn of the year when the beloved
dead are remembered and lanterns are lit to guide them home. Tonight you help her keep it. And
tonight, because it is that kind of night, you can turn into a cat.

Sources are cited by key, such as [S1], from [RESEARCH.md](RESEARCH.md). Its
[sensitivity notes](RESEARCH.md#sensitivity-notes) govern everything in this document.

## Pitch

The town of Hollowell, on 31 October 2026, from just before sunset until midnight. The sun goes
down where it really goes down at 42.4 N, the porch lights come on, the street fills with ghosts and
skeletons, and the woods at the edge of town go dark. You walk anywhere: up Elm Street from door
to door, across the green, over the creek bridge, through the orchard and up Lantern Hill, where
Nana will be waiting at midnight.

Two things to do, and both matter:

- **Trick-or-treat.** Knock on doors with a porch light on. Each neighbour has a personality: some
  just hand over candy, some ask for a trick first, and you tell them a joke, as children guising
  in Scotland and Ireland have done for a long time [S20]. Your costume pleases some of them more
  than others. Candy is also money: the corner shop sells what Nana's recipe needs.
- **Keep Samhain with Nana.** Light the lanterns that four families have asked for, one each for
  someone they have lost, set in the window or by the door the way candles were set to light the
  dead home [S12]. Gather apple and rosemary and buy cinnamon, and brew Nana's spiced cider. Find
  Smoke, Nana's old cat, who died this spring and has lost her way, and lead her up Lantern Hill
  to the circle where Nana makes her offering at midnight.

And the verb that ties them together: **press Q and you are a black cat.** Small, quick, eyes a
hand's height off the ground. You hop onto fences and sheds and from there onto roofs, slip
through gaps in hedges, and see what people cannot: the spirits out walking on the one night the
veil is thin, Smoke among them. But a cat cannot knock on a door, and cannot carry a candy bag or a
lantern jar. Everything Wren carries stays on the ground where she changed until she comes back
for it. Most of the night's puzzles need both of you: the cat gets in, the girl gets things done.

| | |
|---|---|
| Audience | Families; anyone who likes a cosy walk at night |
| Length | About half an hour for the night's goals, longer to find every secret |
| Engine | [Strata](https://github.com/Preponderous-Software/strata) |
| Targets | Windows and Linux desktop, and the browser (WebGL 2) |

## Tone: two kinds of witch

The game holds two very different things, and keeps them apart on purpose.

**The costume witch** is Halloween fun: a green face, a pointy hat, a broom from the hall closet.
Wren can wear it. The Whitfields, who have turned their front yard into a haunted house, love it.

**Nana's witchcraft is a religion.** Wicca is a living, legally recognised faith, nature-based,
with its own calendar, the Wheel of the Year, on which Samhain is one of eight festivals [S1] [S5]
[S6]. Nana's practice is ordinary and warm: candles, a garden of herbs, a simmering pot, the names
of her dead said out loud, an offering of apple and cider, and the Wiccan Rede's "harm none" as
the rule she lives by [S8]. Nothing in the game connects Nana or her coven with the devil, curses,
blood or fear. She is the kindest person on the street.

Where the game is fantasy, it says so. The cat form and the visible spirits are the one piece of
fairy tale in the night: a game's magic, not a claim about what Wiccans believe or do. The black
cat comes from European folklore about witches, much of it from the literature of the witch
trials, which persecuted real people [S24]; the game borrows the cat, not the persecution, and
never makes the trials a joke. The notebook's pages that explain Samhain, the Wheel of the Year and
the Rede say plainly which parts are history, which are modern practice and which are the game's
own invention, with their sources.

## The night

| Time | What happens |
|---|---|
| 5:20 pm | Nana's porch. The sun is low in the south-west. Choose a costume (1–4); Nana gives you a candy bag and a lantern jar with a lit taper, and asks you to light her window lantern first |
| about 5:44 pm | Sunset [S30]. Porch lights come on along Elm Street; children appear in the street |
| about 6:14 pm | The end of civil twilight [S30]. It is properly dark: the lantern jar and porch lights matter |
| 9:00 pm | Trick-or-treating ends. Porch lights go off, except for a house where something has changed |
| about 10:46 pm | The waning gibbous moon rises in the east-north-east [S30] |
| 10:30 pm | Nana walks up Lantern Hill with her basket |
| midnight | The church clock strikes twelve. If Smoke is at the circle, the offering made and the family lanterns lit, Smoke goes home |

Time runs at one game minute for every three seconds of play, so the whole night is a little over
half an hour. Nothing fails if you are late: the veil stays thin until dawn, and the clock strikes
every hour. Midnight is simply when it is best.

## The town

Hollowell is a hill town of a dozen houses along one street, built from the same kit of parts as
the owner's other Strata games (see [ARCHITECTURE.md](ARCHITECTURE.md)).

- **Elm Street** runs east to west through the middle of town, with houses on both sides, picket
  fences, sidewalks and a few streetlamps.
- **Nana's house** at the west end, with her herb garden and the iron pot on the fire pit out back.
- **The green** with its bandstand, where the children swap candy.
- **Abbas's Market** on the corner, open late on Halloween.
- **St. Brendan's** church on a rise to the north, its graveyard behind a stone wall and a tall
  hedge, entered by a roofed lychgate.
- **The creek**, crossed by a wooden bridge where Elm Street ends.
- **The orchard** across the creek, and beyond it **the woods' edge**.
- **Lantern Hill** to the north-east, with a ring of standing stones on top, the highest place
  around. From there the whole town shows as a string of lights.

## The neighbours

| House | Who | At the door |
|---|---|---|
| Nana's | Nana Rowan | Gives the bag and the lantern jar; brews the cider with you |
| 1 | The Okafors | Love a joke. Ask for a trick, then ask you to light Grandpa Ike's lantern |
| 2 | Mr. Hale | Porch light off. His wife June died last winter; her lantern is in the back yard behind a latched gate |
| 3 | The Lindqvists | Full-size bars; delighted by a pumpkin |
| 4 | Dr. Pell, the dentist | A toothbrush, then, with a wink, one candy |
| 5 | The Garcias | Generous; they love a skeleton |
| 6 | Ms. Bright, a teacher | Asks for a trick; partial to ghosts |
| 7 | The Novaks | A giant inflatable spider and two candies each |
| 8 | The Whitfields | A haunted front yard; they love a costume witch |
| Shop | Mr. Abbas | Sells cinnamon sticks for three candies |

## The family lanterns

| Lantern | For | How |
|---|---|---|
| Nana's window | Nana's mother, Ada | The first thing you do: E at the window with the lantern jar |
| The Okafors' porch | Grandpa Ike | Tell the Okafors a joke and they ask you to light it |
| Mr. Hale's back step | June Hale | The gate is latched from inside. As a cat, jump the fence by the woodpile and nose the latch up; then come back as Wren with the lantern jar |
| The lychgate | Everyone in the churchyard | The lychgate is bolted for the night. As a cat, find the gap in the churchyard hedge and work the bolt from inside |

Lighting Mr. Hale's lantern turns his porch light back on, however late it is.

## Nana's cider

Two apples from the orchard (one for the pot, one for the offering), a sprig of rosemary from
Nana's garden ("for remembrance" [S16]) and a cinnamon stick from Abbas's Market. Stand at the
iron pot behind Nana's house and stir (E) until it is ready. The cider and an apple are the
offering; apples and cider belong to this night in both its Halloween and its Samhain traditions
[S17] [S18].

## Smoke

As a cat you see faint, glowing paw prints that people cannot: Smoke's trail, from Nana's garden
to wherever she has wandered. Find her and she follows you, but only while you are a cat (she
will not come to people tonight). Lead her up Lantern Hill to the circle. On the way you pass
other spirits out walking, all of them gentle: the town's own dead, visiting.

## Secrets

Eight things to find, each a page in Wren's notebook (Tab): the initials carved in the bridge
rail, the owl in the old oak, the fairy ring at the woods' edge, the view from Nana's roof (cat
only), the stone cat over the lychgate, the scarecrow in the orchard, the old lamplighter who
walks the hill path (cat only), and the candy-swap tree on the green. Secrets earn nothing but the
page and the count.

## Wren's notebook

Tab opens Wren's notebook. Its pages fill in as the night goes: what Samhain is, the Wheel of the
Year, the Rede, why lanterns go in windows, guising, why a turnip came before the pumpkin, the
black cat in folklore, and the secrets. The pages about real belief and history cite their sources
and say where the game invents.

## What success looks like

Midnight on Lantern Hill, the church clock striking, the town below a string of porch lights and
lanterns, a small grey cat walking into the dark between the stones and not looking back, Nana
saying her name. Then the night carries on, and you can walk home.
