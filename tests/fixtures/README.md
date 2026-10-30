# Save fixtures

Saves written by earlier formats, kept so that every later version of Familiar is tested against
them: `tests/test_saves.cpp` loads each one and plays on from it to the pinned playthrough's end.
A format change that cannot read one of these fails the build.

| File | Format | Written by | What it holds |
|---|---|---|---|
| `v1-bright-asking.save` | 1 | `make_save_fixtures` at M5 (2026-10-29) | Seed 1's autopilot playthrough the tick Ms. Bright asks for a trick (tick 626, ghost costume) |
| `v1-home.save` | 1 | `make_save_fixtures` at M5 (2026-10-29) | The playthrough's end: every lit porch visited, home on Nana's porch with 18 candy (tick 2831) |
| `v2-bright-asking.save` | 2 | `make_save_fixtures` after M3 (2026-10-29) | As `v1-bright-asking.save`, with M3's fields (bag in hand, no spirits seen) |
| `v2-home.save` | 2 | `make_save_fixtures` after M3 (2026-10-29) | As `v1-home.save`, with M3's fields |
| `v2-cat-bag-down.save` | 2 | `make_save_fixtures` after M3 (2026-10-29) | Wren became the cat after the Garcias' door, leaving the bag on their doorstep; the cat has seen the reader in the churchyard (tick 210) |

Each was written through `writeSave` (so it is exactly what the game writes) by
`tools/make_save_fixtures.cpp`:

```
cmake --build build --target make_save_fixtures
./build/make_save_fixtures tests/fixtures
```

That writes the current format's files. When the format's version is bumped, run it once to add
the new version's fixtures, and never regenerate an older version's: they stand for the saves
players already have.
