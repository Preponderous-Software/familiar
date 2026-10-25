# Familiar

A small open-world game about one Halloween night in a New England hill town, on the
[Strata](https://github.com/Preponderous-Software/strata) engine. You are Wren, twelve, out
trick-or-treating, and helping your grandmother, who is a Wiccan, keep Samhain: lighting lanterns
for the town's beloved dead, brewing her cider, and bringing her old cat's spirit home by midnight.
And tonight you can turn into a cat.

**Status: in development.** See [CONCEPT.md](CONCEPT.md) for the game, [MVP.md](MVP.md) for what
the first playable version holds and its milestones, [RESEARCH.md](RESEARCH.md) for the sources
behind its portrayal of Samhain and Wicca, and [ARCHITECTURE.md](ARCHITECTURE.md) for how it is
built.

## Building

The rules and tests build without a GPU:

```sh
cmake -S . -B build -DFAMILIAR_WITH_STRATA=OFF
cmake --build build -j
ctest --test-dir build
```

## Licence

MIT; see [LICENSE](LICENSE).
