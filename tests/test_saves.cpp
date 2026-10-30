// Saves: a save round-trips the World exactly; any one byte flipped, any
// truncation, and extra bytes are refused, never half-loaded and never a
// crash; an impossible world is refused even with a good checksum; writing
// keeps the save it replaces, and a damaged current save gives way to the
// previous one (and is set aside, not deleted); saves from every earlier
// format still load; and the playthrough saved and loaded anywhere ends on
// the same pinned fingerprint.
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

#include "check.h"
#include "save/save.h"
#include "save/sha256.h"
#include "sim/autopilot.h"
#include "sim/cat.h"
#include "sim/doors.h"
#include "sim/world.h"

using namespace familiar;
namespace fs = std::filesystem;
namespace T = familiar::town;

namespace {

// The autopilot's world after `ticks`.
World played(int64_t ticks) {
    World w = newWorld(1);
    for (int64_t t = 0; t < ticks; ++t) tick(w, autopilot(w));
    return w;
}

bool roundTrips(const World& w) {
    World back = newWorld(99);  // anything not from the save would show
    std::string why;
    if (!decodeSave(encodeSave(w), back, &why)) {
        std::printf("refused: %s\n", why.c_str());
        return false;
    }
    return fingerprint(back) == fingerprint(w) && encodeSave(back) == encodeSave(w);
}

std::vector<uint8_t> readAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

void writeAll(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

// Re-seals a save's checksum after its bytes were changed on purpose, to show
// that what the checksum does not catch is still refused.
std::vector<uint8_t> reseal(std::vector<uint8_t> bytes) {
    const size_t body = bytes.size() - 32;
    const std::array<uint8_t, 32> d = sha256(bytes.data(), body);
    std::memcpy(bytes.data() + body, d.data(), 32);
    return bytes;
}

// Every way of damaging `good` is refused, and leaves `out` as it was.
int damagedAccepted(const std::vector<uint8_t>& good) {
    int accepted = 0;
    const World sentinel = newWorld(7);
    auto tryBad = [&](const std::vector<uint8_t>& bad) {
        World w = sentinel;
        if (decodeSave(bad, w, nullptr)) ++accepted;
        else if (fingerprint(w) != fingerprint(sentinel)) ++accepted;  // half-loaded
    };
    for (size_t i = 0; i < good.size(); ++i)
        for (uint8_t bit = 1; bit != 0; bit = static_cast<uint8_t>(bit << 1)) {
            std::vector<uint8_t> bad = good;
            bad[i] ^= bit;
            tryBad(bad);
        }
    for (size_t n = 0; n < good.size(); ++n) tryBad(std::vector<uint8_t>(good.begin(), good.begin() + static_cast<long>(n)));
    std::vector<uint8_t> longer = good;
    longer.push_back(0);
    tryBad(longer);
    tryBad({});
    return accepted;
}

}  // namespace

int main(int argc, char** argv) {
    const fs::path dir = fs::path(argc > 1 ? argv[1] : ".") / "test_saves_slot";
    std::error_code ec;
    fs::remove_all(dir, ec);
    const std::string slot = dir.string();
    const int64_t second = kTickRate;

    // The format: magic, version, length, payload, SHA-256.
    {
        const std::vector<uint8_t> b = encodeSave(newWorld(1));
        CHECK(b.size() > 52);
        CHECK(std::memcmp(b.data(), "FAMILIAR", 8) == 0);
        CHECK(b[8] == kSaveVersion && b[9] == 0 && b[10] == 0 && b[11] == 0);
    }

    // Round trips: a new game, mid-walk, a neighbour waiting for a joke, a
    // cat up on something, after the porch lights go off, and the end.
    CHECK(roundTrips(newWorld(1)));
    CHECK(roundTrips(newWorld(0xFFFFFFFFFFFFFFFFULL)));
    for (int64_t t : {int64_t{1}, 5 * second + 7, 30 * second, 90 * second, 200 * second}) CHECK(roundTrips(played(t)));
    {
        World w = newWorld(1);
        while (w.asking < 0 && w.tick < kPlaythroughLimit) tick(w, autopilot(w));
        CHECK(w.asking >= 0);
        CHECK(roundTrips(w));
    }
    {
        World w = newWorld(1);
        Input q;
        q.toggleForm = true;
        tick(w, q);
        w.player.feet = 1.0f;
        w.gateOpen[T::HaleGate] = true;
        CHECK(w.bagDown);
        w.spiritsSeen = 0b1010;
        CHECK(roundTrips(w));
        World late = newWorld(1);
        late.tick = 5 * 60 * 60 * kTickRate;  // 10:20 pm
        CHECK(roundTrips(late));
    }

    // Damage: every single bit flipped, every truncation, an extra byte, and
    // nothing at all, at three points of the night. None loads, and none
    // touches the world it would have loaded into.
    for (int64_t t : {int64_t{0}, 30 * second, 200 * second}) {
        const std::vector<uint8_t> good = encodeSave(played(t));
        CHECK(damagedAccepted(good) == 0);
    }
    {
        const std::vector<uint8_t> good = encodeSave(played(30 * second));
        std::string why;
        World w;
        std::vector<uint8_t> bad = good;
        bad[30] ^= 0x01;
        CHECK(!decodeSave(bad, w, &why));
        CHECK(why.find("damaged") != std::string::npos);
        CHECK(!decodeSave(std::vector<uint8_t>(good.begin(), good.end() - 1), w, &why));
        CHECK(why.find("truncated") != std::string::npos);
        // A newer format (sealed properly, as a newer Familiar would) is refused, not half-read.
        bad = good;
        bad[8] = static_cast<uint8_t>(kSaveVersion + 1);
        CHECK(!decodeSave(reseal(bad), w, &why));
        CHECK(why.find("different version") != std::string::npos);
        bad[8] = 0;
        CHECK(!decodeSave(reseal(bad), w, &why));
        // Not a save at all.
        CHECK(!decodeSave(std::vector<uint8_t>(100, 0), w, &why));
        CHECK(why.find("not a Familiar save") != std::string::npos);
    }

    // A checksummed save of an impossible world is still refused.
    {
        auto refused = [](World odd) {
            World w;
            std::string why;
            const bool ok = decodeSave(encodeSave(odd), w, &why);
            return !ok && !why.empty();
        };
        World odd = newWorld(1);
        odd.costume = static_cast<Costume>(9);
        CHECK(refused(odd));
        odd = newWorld(1);
        odd.form = static_cast<Form>(-1);
        CHECK(refused(odd));
        odd = newWorld(1);
        odd.answer = static_cast<Answer>(77);
        CHECK(refused(odd));
        odd = newWorld(1);
        odd.answered = T::HouseCount;
        CHECK(refused(odd));
        odd = newWorld(1);
        odd.asking = -2;
        CHECK(refused(odd));
        odd = newWorld(1);
        odd.answeredTick = 5;  // after the clock
        CHECK(refused(odd));
        odd = newWorld(1);
        odd.tick = -1;
        CHECK(refused(odd));
        odd = newWorld(1);
        odd.candy = -3;
        CHECK(refused(odd));
        odd = newWorld(1);
        odd.player.x = 1e9f;
        CHECK(refused(odd));
        odd = newWorld(1);
        odd.player.yaw = std::numeric_limits<float>::quiet_NaN();
        CHECK(refused(odd));
        odd = newWorld(1);
        odd.spiritsSeen = 1u << SpiritCount;
        CHECK(refused(odd));
        odd = newWorld(1);
        odd.bagDown = true;
        odd.bag = {1e7f, 0, 0};
        CHECK(refused(odd));
        // A bool that is neither 0 nor 1, sealed: refused.
        std::vector<uint8_t> b = encodeSave(newWorld(1));
        const size_t firstGate = 20 + 8 + 8 + 5 * 4 + 4 + 4;  // header, seed, tick, player, form, gate count
        CHECK(b[firstGate - 4] == T::GateCount);
        b[firstGate] = 2;
        World w;
        CHECK(!decodeSave(reseal(b), w, nullptr));
        // More gates than this town has: refused.
        b = encodeSave(newWorld(1));
        b[firstGate - 4] = static_cast<uint8_t>(T::GateCount + 1);
        CHECK(!decodeSave(reseal(b), w, nullptr));
    }

    // Writing keeps the previous save; a damaged current save gives way to it.
    {
        CHECK(!hasSave(slot));
        World none;
        LoadResult r = loadSave(slot, none);
        CHECK(!r.ok);
        CHECK(r.message == "No save to load.");
        const World first = played(30 * second), secondSave = played(60 * second);
        std::string why;
        CHECK(writeSave(slot, first, &why));
        CHECK(writeSave(slot, secondSave, &why));
        CHECK(hasSave(slot));
        CHECK(!fs::exists(fs::path(slot) / "familiar.save.tmp"));
        World loaded;
        r = loadSave(slot, loaded);
        CHECK(r.ok && !r.fromPrevious);
        CHECK(r.message == "Loaded your save.");
        CHECK(fingerprint(loaded) == fingerprint(secondSave));
        World kept;
        CHECK(decodeSave(readAll(previousSavePath(slot)), kept, nullptr));
        CHECK(fingerprint(kept) == fingerprint(first));

        // Any one byte of the current save flipped: the previous one loads, with a message.
        const std::vector<uint8_t> current = readAll(savePath(slot));
        for (size_t i = 0; i < current.size(); ++i) {
            std::vector<uint8_t> bytes = current;
            bytes[i] ^= 0x01;
            writeAll(savePath(slot), bytes);
            World got = newWorld(5);
            r = loadSave(slot, got);
            CHECK(r.ok && r.fromPrevious);
            CHECK(fingerprint(got) == fingerprint(first));
            CHECK(r.message.find("refused") != std::string::npos);
        }
        // Cut short, the same.
        writeAll(savePath(slot), std::vector<uint8_t>(current.begin(), current.begin() + 40));
        r = loadSave(slot, loaded);
        CHECK(r.ok && r.fromPrevious && fingerprint(loaded) == fingerprint(first));

        // Saving over a damaged save keeps the good previous one, and sets the damaged one aside.
        std::vector<uint8_t> damaged = current;
        damaged[damaged.size() / 2] ^= 0x40;
        writeAll(savePath(slot), damaged);
        const World third = played(90 * second);
        CHECK(writeSave(slot, third, &why));
        r = loadSave(slot, loaded);
        CHECK(r.ok && !r.fromPrevious && fingerprint(loaded) == fingerprint(third));
        World prev;
        CHECK(decodeSave(readAll(previousSavePath(slot)), prev, nullptr));
        CHECK(fingerprint(prev) == fingerprint(first));
        CHECK(readAll((fs::path(slot) / "familiar.save.damaged").string()) == damaged);

        // Both damaged: nothing loads, the world is untouched, and the player is told.
        std::vector<uint8_t> a = readAll(savePath(slot)), b = readAll(previousSavePath(slot));
        a[25] ^= 0x02;
        b[26] ^= 0x04;
        writeAll(savePath(slot), a);
        writeAll(previousSavePath(slot), b);
        World untouched = newWorld(3);
        r = loadSave(slot, untouched);
        CHECK(!r.ok);
        CHECK(fingerprint(untouched) == fingerprint(newWorld(3)));
        CHECK(r.message.find("no good earlier save") != std::string::npos);

        // Only a previous save (the current one lost): it loads.
        fs::remove(savePath(slot), ec);
        CHECK(writeSave(slot, first, &why));  // the damaged previous stays; this becomes current
        CHECK(loadSave(slot, loaded).ok && fingerprint(loaded) == fingerprint(first));
        fs::remove(slot + "/familiar.save.prev", ec);
        fs::rename(savePath(slot), previousSavePath(slot), ec);
        r = loadSave(slot, loaded);
        CHECK(r.ok && r.fromPrevious && fingerprint(loaded) == fingerprint(first));
        CHECK(r.message == "Loaded your previous save.");
    }

    // Saves of every format still load, and the night goes on from them to
    // the same end. The fixtures were written through writeSave
    // (tools/make_save_fixtures.cpp; see tests/fixtures/README.md): for each
    // format, one with Ms. Bright waiting for a joke and one at the
    // playthrough's end; from format 2, a cat who left the bag behind.
    for (const char* v : {"v1", "v2"}) {
        const std::string dirName = FAMILIAR_FIXTURES "/";
        World asking;
        std::string why;
        CHECK(decodeSave(readAll(dirName + v + "-bright-asking.save"), asking, &why));
        CHECK(asking.asking == T::Bright && asking.costume == Costume::Ghost);
        // Format 1 knew nothing of the bag or the spirits: it loads with the bag in hand and none seen.
        CHECK(!asking.bagDown && asking.spiritsSeen == 0);
        CHECK(fingerprint(asking) == fingerprint(played(asking.tick)));  // just as the rules have it now
        while (!playthroughDone(asking) && asking.tick < kPlaythroughLimit) tick(asking, autopilot(asking));
        CHECK(fingerprint(asking) == kPlaythroughFingerprint);

        World end;
        CHECK(decodeSave(readAll(dirName + v + "-home.save"), end, &why));
        CHECK(playthroughDone(end) && end.candy == 18);
        CHECK(fingerprint(end) == kPlaythroughFingerprint);
        CHECK(roundTrips(end));
    }
    {
        // A format 1 save re-saved today is format 2, and comes back the same.
        World old;
        const std::vector<uint8_t> v1 = readAll(FAMILIAR_FIXTURES "/v1-home.save");
        CHECK(v1.size() > 12 && v1[8] == 1);
        CHECK(decodeSave(v1, old, nullptr));
        const std::vector<uint8_t> resaved = encodeSave(old);
        CHECK(resaved[8] == kSaveVersion && resaved.size() > v1.size());
        CHECK(roundTrips(old));
        // And every bit of a format 1 save is still guarded.
        CHECK(damagedAccepted(v1) == 0);

        World cat;
        std::string why;
        CHECK(decodeSave(readAll(FAMILIAR_FIXTURES "/v2-cat-bag-down.save"), cat, &why));
        CHECK(cat.form == Form::Cat && cat.bagDown && cat.knocked[T::Garcia]);
        CHECK(std::fabs(cat.bag.x - T::doorstep(T::Garcia).x) < 0.5f && std::fabs(cat.bag.z - T::doorstep(T::Garcia).z) < 0.5f);  // where Wren changed
        CHECK((cat.spiritsSeen & (1u << Reader)) != 0);
        CHECK(roundTrips(cat));
        CHECK(damagedAccepted(readAll(FAMILIAR_FIXTURES "/v2-cat-bag-down.save")) == 0);
    }

    // Saved and loaded anywhere (every 97 ticks, through the slot), the
    // playthrough ends where it always does.
    {
        World w = newWorld(1);
        int reloads = 0;
        while (!playthroughDone(w) && w.tick < kPlaythroughLimit) {
            tick(w, autopilot(w));
            if (w.tick % 97 == 0) {
                std::string why;
                CHECK(writeSave(slot, w, &why));
                World back = newWorld(2);
                CHECK(loadSave(slot, back).ok);
                w = back;
                ++reloads;
            }
        }
        CHECK(reloads > 10);
        std::printf("the playthrough reloaded %d times and ended on %016" PRIx64 "\n", reloads, fingerprint(w));
        CHECK(fingerprint(w) == kPlaythroughFingerprint);
    }

    fs::remove_all(dir, ec);
    TEST_MAIN_END();
}
