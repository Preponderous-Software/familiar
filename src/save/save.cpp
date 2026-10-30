#include "save/save.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <type_traits>

#include "save/sha256.h"
#include "sim/cat.h"
#include "sim/notebook.h"

namespace fs = std::filesystem;

namespace familiar {

namespace {

constexpr char kMagic[8] = {'F', 'A', 'M', 'I', 'L', 'I', 'A', 'R'};
constexpr size_t kHeader = 8 + 4 + 8;  // magic, version, length
constexpr size_t kDigest = 32;

// A reminder for whoever grows the World: every field the rules keep must be
// in the save (Fields::world below), behind a new kSaveVersion, with a test
// that a save from the version before still loads (tests/test_saves.cpp).
// When the World changes size this stops the build; update the size here
// once the save carries the new fields.
static_assert(sizeof(World) == 176, "World changed: add its new fields to the save (src/save/save.cpp), bump kSaveVersion");

// Writes little-endian.
struct ByteWriter {
    std::vector<uint8_t> out;
    void raw(uint64_t v, int bytes) {
        for (int i = 0; i < bytes; ++i) out.push_back(static_cast<uint8_t>(v >> (8 * i)));
    }
    template <class T>
    void value(const T& v) {
        if constexpr (std::is_same_v<T, bool>) {
            raw(v ? 1 : 0, 1);
        } else if constexpr (std::is_same_v<T, float>) {
            uint32_t bits;
            std::memcpy(&bits, &v, 4);
            raw(bits, 4);
        } else if constexpr (std::is_enum_v<T>) {
            raw(static_cast<uint64_t>(static_cast<int64_t>(v)), 4);
        } else {
            raw(static_cast<uint64_t>(v), static_cast<int>(sizeof(T)));
        }
    }
    // An array: its length, then its elements.
    template <class T, size_t N>
    void array(std::array<T, N>& a) {
        raw(N, 4);
        for (T& x : a) value(x);
    }
};

// Reads little-endian, refusing to read past the end.
struct ByteReader {
    const uint8_t* p;
    size_t n, at = 0;
    bool ok = true;
    uint64_t raw(int bytes) {
        if (!ok || n - at < static_cast<size_t>(bytes)) {
            ok = false;
            return 0;
        }
        uint64_t v = 0;
        for (int i = 0; i < bytes; ++i) v |= static_cast<uint64_t>(p[at + i]) << (8 * i);
        at += static_cast<size_t>(bytes);
        return v;
    }
    template <class T>
    void value(T& v) {
        if constexpr (std::is_same_v<T, bool>) {
            const uint64_t b = raw(1);
            if (b > 1) ok = false;
            v = b != 0;
        } else if constexpr (std::is_same_v<T, float>) {
            const auto bits = static_cast<uint32_t>(raw(4));
            std::memcpy(&v, &bits, 4);
        } else if constexpr (std::is_enum_v<T>) {
            v = static_cast<T>(static_cast<int32_t>(static_cast<uint32_t>(raw(4))));
        } else {
            v = static_cast<T>(raw(static_cast<int>(sizeof(T))));
        }
    }
    // An array saved with its length: a shorter one (from a town with fewer
    // gates or houses) leaves the rest at their defaults; a longer one is
    // refused, because this game has nowhere to keep it.
    template <class T, size_t N>
    void array(std::array<T, N>& a) {
        const uint64_t len = raw(4);
        if (len > N) {
            ok = false;
            return;
        }
        for (size_t i = 0; i < len && ok; ++i) value(a[i]);
    }
};

// One description of the save's fields, used both ways. `version` is the
// format being read; writing is always kSaveVersion. New fields go at the end,
// behind `if (version < N) return;`.
template <class IO>
struct Fields {
    IO& io;
    uint32_t version = kSaveVersion;

    template <class T>
    void v(T& x) { io.value(x); }
    void world(World& w) {
        v(w.seed);
        v(w.tick);
        v(w.player.x);
        v(w.player.z);
        v(w.player.feet);
        v(w.player.yaw);
        v(w.player.pitch);
        v(w.form);
        io.array(w.gateOpen);
        v(w.candy);
        io.array(w.knocked);
        v(w.answered);
        v(w.answeredTick);
        v(w.answer);
        v(w.asking);
        v(w.jokes);
        v(w.costume);
        if (version < 2) return;  // a format 1 save was made before the cat could drop the bag
        v(w.bagDown);
        v(w.bag.x);
        v(w.bag.y);
        v(w.bag.z);
        v(w.spiritsSeen);
        if (version < 3) return;  // a format 2 save was made before the witchcraft
        io.array(w.lit);
        v(w.apples);
        v(w.rosemary);
        v(w.cinnamon);
        v(w.stirs);
        v(w.cider);
        v(w.offered);
        v(w.smoke);
        v(w.smokeX);
        v(w.smokeZ);
        v(w.homeTick);
        v(w.pages);
        v(w.secrets);
        v(w.deed);
        v(w.deedArg);
        v(w.deedTick);
    }
};

// What a decoded world must look like before the rules may run on it. Only
// what can never happen is refused: a good save is never turned away for
// being unusual.
bool plausible(const World& w, std::string* error) {
    auto fail = [error](const char* why) {
        if (error) *error = why;
        return false;
    };
    const float floats[] = {w.player.x, w.player.z, w.player.feet, w.player.yaw, w.player.pitch, w.bag.x, w.bag.y, w.bag.z, w.smokeX, w.smokeZ};
    for (float f : floats)
        if (!std::isfinite(f)) return fail("a number in the save is not finite");
    if (std::fabs(w.player.x) > town::kWalkLimit + 1 || std::fabs(w.player.z) > town::kWalkLimit + 1)
        return fail("the player is outside the town");
    if (w.player.feet < -10 || w.player.feet > 100) return fail("the player is in the sky");
    if (std::fabs(w.player.pitch) > 2) return fail("the player is looking somewhere impossible");
    if (w.tick < 0) return fail("the clock is impossible");
    const int form = static_cast<int>(w.form);
    if (form < 0 || form > static_cast<int>(Form::Cat)) return fail("an unknown form");
    const int costume = static_cast<int>(w.costume);
    if (costume < 0 || costume > static_cast<int>(Costume::Witch)) return fail("an unknown costume");
    const int answer = static_cast<int>(w.answer);
    if (answer < 0 || answer > static_cast<int>(Answer::Kitty)) return fail("an unknown answer at a door");
    if (w.answered < -1 || w.answered >= town::HouseCount) return fail("a door answered at a house that is not there");
    if (w.asking < -1 || w.asking >= town::HouseCount) return fail("a trick asked for at a house that is not there");
    if (w.answeredTick < -1 || w.answeredTick > w.tick) return fail("a door answered at an impossible time");
    if (w.candy < 0 || w.jokes < 0) return fail("a count below zero");
    if (std::fabs(w.bag.x) > town::kWalkLimit + 1 || std::fabs(w.bag.z) > town::kWalkLimit + 1 || w.bag.y < -10 || w.bag.y > 100)
        return fail("the candy bag is outside the town");
    if ((w.spiritsSeen >> SpiritCount) != 0) return fail("a spirit seen that is not out tonight");
    if (w.apples < 0 || w.stirs < 0) return fail("a count below zero");
    const int smoke = static_cast<int>(w.smoke);
    if (smoke < 0 || smoke > static_cast<int>(SmokeState::Home)) return fail("Smoke is somewhere unknown");
    if (std::fabs(w.smokeX) > town::kWalkLimit + 1 || std::fabs(w.smokeZ) > town::kWalkLimit + 1)
        return fail("Smoke is outside the town");
    if (w.homeTick < -1 || w.homeTick > w.tick) return fail("Smoke went home at an impossible time");
    if ((w.pages >> PageCount) != 0) return fail("a notebook page that does not exist");
    if ((w.secrets >> SecretCount) != 0) return fail("a secret that does not exist");
    const int deed = static_cast<int>(w.deed);
    if (deed < 0 || deed > static_cast<int>(Deed::Home)) return fail("an unknown deed");
    if (w.deedTick < -1 || w.deedTick > w.tick) return fail("a deed done at an impossible time");
    return true;
}

bool readFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return !f.bad();
}

}  // namespace

std::vector<uint8_t> encodeSave(const World& w) {
    ByteWriter payload;
    Fields<ByteWriter> fields{payload};
    World copy = w;  // the ByteWriter only reads, but Fields takes the World by reference
    fields.world(copy);

    ByteWriter file;
    file.out.insert(file.out.end(), kMagic, kMagic + 8);
    file.value(kSaveVersion);
    file.value(static_cast<uint64_t>(payload.out.size()));
    file.out.insert(file.out.end(), payload.out.begin(), payload.out.end());
    const std::array<uint8_t, 32> digest = sha256(file.out.data(), file.out.size());
    file.out.insert(file.out.end(), digest.begin(), digest.end());
    return file.out;
}

bool decodeSave(const std::vector<uint8_t>& bytes, World& out, std::string* error) {
    auto fail = [error](const std::string& why) {
        if (error) *error = why;
        return false;
    };
    if (bytes.size() < kHeader + kDigest || std::memcmp(bytes.data(), kMagic, 8) != 0) return fail("not a Familiar save");
    ByteReader head{bytes.data(), bytes.size()};
    head.at = 8;
    uint32_t version = 0;
    uint64_t length = 0;
    head.value(version);
    head.value(length);
    if (length != bytes.size() - kHeader - kDigest) return fail("the save is truncated or has extra bytes");
    // The checksum covers the header too, so a damaged version or length is
    // caught here, before either is trusted.
    const std::array<uint8_t, 32> digest = sha256(bytes.data(), kHeader + static_cast<size_t>(length));
    if (std::memcmp(digest.data(), bytes.data() + kHeader + length, kDigest) != 0)
        return fail("the save is damaged (its checksum does not match)");
    // Older formats still load: each later version only adds fields.
    if (version < 1 || version > kSaveVersion)
        return fail("saved by a different version of Familiar (format " + std::to_string(version) + ")");

    ByteReader r{bytes.data() + kHeader, static_cast<size_t>(length)};
    Fields<ByteReader> fields{r, version};
    // Fields an older format lacks keep a new night's defaults (Smoke under
    // the old oak, for one); everything the format has is read over them.
    World w = newWorld(0);
    fields.world(w);
    if (!r.ok || r.at != r.n) return fail("the save's contents do not parse");
    // Migration: a save from before the notebook (format 3) gets the pages its
    // night has already earned (a joke told, a porch knocked at), as the next
    // tick would give them; the rules derive them from the World alone.
    if (version < 3) notebookTick(w);
    if (!plausible(w, error)) return false;
    out = w;
    return true;
}

std::string savePath(const std::string& slot) { return (fs::path(slot) / "familiar.save").string(); }
std::string previousSavePath(const std::string& slot) { return (fs::path(slot) / "familiar.save.prev").string(); }

bool writeSave(const std::string& slot, const World& w, std::string* error) {
    std::error_code ec;
    fs::create_directories(slot, ec);
    const std::vector<uint8_t> bytes = encodeSave(w);
    // Never write a save this game could not read back.
    {
        World check;
        std::string why;
        if (!decodeSave(bytes, check, &why)) {
            if (error) *error = "the save would not read back (" + why + ")";
            return false;
        }
    }
    const fs::path tmp = fs::path(slot) / "familiar.save.tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        f.flush();
        if (!f) {
            if (error) *error = "could not write " + tmp.string();
            return false;
        }
    }
    // And check what reached the disk before it replaces anything.
    {
        std::vector<uint8_t> back;
        if (!readFile(tmp.string(), back) || back != bytes) {
            fs::remove(tmp, ec);
            if (error) *error = "the save did not reach the disk intact";
            return false;
        }
    }
    // Keep what was there as the previous save, then move the new one in. A
    // damaged save never replaces a good previous one: it is set aside as
    // familiar.save.damaged instead (never deleted, in case it can be mended).
    const fs::path cur = savePath(slot), prev = previousSavePath(slot);
    if (fs::exists(cur, ec)) {
        std::vector<uint8_t> old;
        World check;
        if (readFile(cur.string(), old) && decodeSave(old, check, nullptr)) {
            fs::remove(prev, ec);
            fs::rename(cur, prev, ec);
            if (ec) {
                if (error) *error = "could not keep the previous save: " + ec.message();
                return false;
            }
        } else {
            const fs::path damaged = fs::path(slot) / "familiar.save.damaged";
            fs::remove(damaged, ec);
            fs::rename(cur, damaged, ec);
            if (ec) {
                if (error) *error = "could not set the damaged save aside: " + ec.message();
                return false;
            }
        }
    }
    fs::rename(tmp, cur, ec);
    if (ec) {
        if (error) *error = "could not move the new save into place: " + ec.message();
        return false;
    }
    return true;
}

LoadResult loadSave(const std::string& slot, World& out) {
    LoadResult r;
    std::vector<uint8_t> bytes;
    std::string why;
    const bool haveCurrent = readFile(savePath(slot), bytes);
    if (haveCurrent && decodeSave(bytes, out, &why)) {
        r.ok = true;
        r.message = "Loaded your save.";
        return r;
    }
    std::string whyPrev;
    if (readFile(previousSavePath(slot), bytes) && decodeSave(bytes, out, &whyPrev)) {
        r.ok = true;
        r.fromPrevious = true;
        r.message = haveCurrent ? "Your latest save was refused (" + why + "). Loaded the one before it."
                                : "Loaded your previous save.";
        return r;
    }
    r.message = haveCurrent ? "Your save was refused (" + why + ") and there is no good earlier save."
                            : "No save to load.";
    return r;
}

bool hasSave(const std::string& slot) {
    std::error_code ec;
    return fs::exists(savePath(slot), ec) || fs::exists(previousSavePath(slot), ec);
}

std::string defaultSlot() {
#ifdef _WIN32
    char* appData = nullptr;
    size_t len = 0;
    std::string base = ".";
    if (_dupenv_s(&appData, &len, "APPDATA") == 0 && appData) base = appData;
    std::free(appData);
    return (fs::path(base) / "Familiar" / "default").string();
#else
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg && *xdg) return (fs::path(xdg) / "familiar" / "default").string();
    const char* home = std::getenv("HOME");
    return (fs::path(home ? home : ".") / ".local" / "share" / "familiar" / "default").string();
#endif
}

}  // namespace familiar
