// Writes the save fixtures in tests/fixtures for the current format, through
// writeSave, from the autopilot's playthrough with seed 1:
//
//   make_save_fixtures <dir>
//
//   v<N>-bright-asking.save  Ms. Bright has just asked for a trick
//   v<N>-home.save           the playthrough's end (since format 3's fixtures
//                            were written, Smoke gone home at midnight, at
//                            the stones; before, home on Nana's porch)
//   v<N>-cat-bag-down.save   (from format 2) after the Garcias' door Wren became
//                            the cat, leaving the bag there, and the cat has
//                            seen the reader in the churchyard
//   v<N>-witchcraft.save     (from format 3) the playthrough the tick Wren
//                            first stirs the pot: lanterns lit, gates open,
//                            spirits seen, an apple carried, pages earned
//                            (format 3's was written by hand after the M5
//                            playthrough: Nana's lantern, the pot stirred once)
//
// Fixtures are written once per format and then kept: tests/test_saves.cpp
// loads every one of them, so a later format that cannot read an older save
// fails the build. Never regenerate an existing version's fixtures.
#include <cstdio>
#include <filesystem>
#include <string>

#include "save/save.h"
#include "sim/autopilot.h"
#include "sim/cat.h"
#include "sim/craft.h"
#include "sim/world.h"

using namespace familiar;
namespace fs = std::filesystem;

namespace {

bool write(const fs::path& dir, const std::string& name, const World& w) {
    const fs::path slot = dir / ("slot-" + name);
    std::string why;
    if (!writeSave(slot.string(), w, &why)) {
        std::fprintf(stderr, "make_save_fixtures: %s: %s\n", name.c_str(), why.c_str());
        return false;
    }
    std::error_code ec;
    const fs::path out = dir / ("v" + std::to_string(kSaveVersion) + "-" + name + ".save");
    fs::copy_file(savePath(slot.string()), out, fs::copy_options::overwrite_existing, ec);
    fs::remove_all(slot, ec);
    if (ec) return false;
    std::printf("%s (tick %lld)\n", out.string().c_str(), static_cast<long long>(w.tick));
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: make_save_fixtures <dir>\n");
        return 2;
    }
    const fs::path dir = argv[1];
    World w = newWorld(1);
    while (w.asking != town::Bright && w.tick < kPlaythroughLimit) tick(w, autopilot(w));
    if (w.asking != town::Bright || !write(dir, "bright-asking", w)) return 1;
    while (!playthroughDone(w) && w.tick < kPlaythroughLimit) tick(w, autopilot(w));
    if (!playthroughDone(w) || !write(dir, "home", w)) return 1;

    // The cat: the bag left outside the Garcias', a spirit seen.
    World c = newWorld(1);
    while (!c.knocked[town::Garcia] && c.tick < kPlaythroughLimit) tick(c, autopilot(c));
    Input q;
    q.yaw = c.player.yaw;
    q.toggleForm = true;
    tick(c, q);
    const Walker reader = spirits(c)[Reader];
    c.player.x = reader.at.x + 1.5f;
    c.player.z = reader.at.z;
    float feet = 0;
    if (!standAt(c, c.form, c.player.x, c.player.z, 10.0f, feet)) return 1;
    c.player.feet = feet;
    Input still;
    still.yaw = c.player.yaw;
    tick(c, still);
    if (!c.bagDown || c.form != Form::Cat || !(c.spiritsSeen & (1u << Reader)) || !write(dir, "cat-bag-down", c)) return 1;

    // Witchcraft: the playthrough the tick the pot is first stirred.
    w = newWorld(1);
    while (w.stirs == 0 && w.tick < kPlaythroughLimit) tick(w, autopilot(w));
    std::printf("witchcraft: lit %d, apples %d, rosemary %d, cinnamon %d, stirs %d, deed %d, pages %x\n", w.lit[NanaWindow] ? 1 : 0,
                w.apples, w.rosemary ? 1 : 0, w.cinnamon ? 1 : 0, w.stirs, static_cast<int>(w.deed), w.pages);
    if (!w.lit[NanaWindow] || w.deed == Deed::None || !write(dir, "witchcraft", w)) return 1;
    return 0;
}
