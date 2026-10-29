// Writes the save fixtures in tests/fixtures for the current format, through
// writeSave, from the autopilot's playthrough with seed 1:
//
//   make_save_fixtures <dir>
//
//   v<N>-bright-asking.save  Ms. Bright has just asked for a trick
//   v<N>-home.save           the playthrough's end, home on Nana's porch
//
// Fixtures are written once per format and then kept: tests/test_saves.cpp
// loads every one of them, so a later format that cannot read an older save
// fails the build. Never regenerate an existing version's fixtures.
#include <cstdio>
#include <filesystem>
#include <string>

#include "save/save.h"
#include "sim/autopilot.h"
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
    return 0;
}
