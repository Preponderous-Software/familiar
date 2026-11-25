#include "sim/world.h"

#include <cmath>
#include <cstdio>

#include "sim/cat.h"
#include "sim/craft.h"
#include "sim/doors.h"
#include "sim/notebook.h"

namespace familiar {

namespace {

constexpr float kMaxPitch = 1.45f;  // just short of straight up or down

// Wren walks and hurries as a twelve-year-old does; the cat is low, quick and
// jumps about a metre and a half onto things.
constexpr Body kGirl{0.3f, 0.5f, 1.45f, 1.5f, 3.6f};
constexpr Body kCat{0.15f, 1.6f, 0.28f, 2.2f, 5.5f};

// How far to either side a blocked walk looks for a way past: enough to round
// a lamp post or the end of a picket fence at a front gate, not enough to
// find a way through a wall.
constexpr float kAssistReach = 0.5f;
constexpr int kAssistProbes = 5;
// A slide along one axis is worth taking when it keeps at least this much of the step.
constexpr float kSlideAtLeast = 0.25f;

// Walking straight into the end of something narrow (a lamp post, the fence
// beside a gate) stops dead, since neither axis on its own gets past it. If a
// short sidestep, clear all the way, would let the walk go on, take one step
// that way: the nearer side, and the left on a tie. move() asks first for the
// side she is bearing toward, and only if nothing else moves her for either. Into a wall, nothing
// within reach frees the way, so Wren stays put rather than drifting along it.
bool sidestep(World& w, float dx, float dz, bool eitherSide) {
    Player& p = w.player;
    const float len = std::sqrt(dx * dx + dz * dz);
    if (len <= 0) return false;
    const float ux = -dz / len, uz = dx / len;  // to the left of the way she walks
    // The lesser part of the way she walks, across the main axis of it.
    const float mx = std::fabs(dx) < std::fabs(dz) ? dx : 0.0f, mz = std::fabs(dx) < std::fabs(dz) ? 0.0f : dz;
    for (int k = 1; k <= kAssistProbes; ++k) {
        const float off = kAssistReach * static_cast<float>(k) / kAssistProbes;
        for (float side : {1.0f, -1.0f}) {
            // First only toward the side she is already bearing (either, walking
            // dead along an axis): stepping away would undo a slide past a rail's end.
            if (!eitherSide && side * (ux * mx + uz * mz) < 0) continue;
            // Clear the whole way across to the offset, then onward from it.
            float feet = p.feet, f = 0;
            bool clear = true;
            for (int j = 1; j <= k && clear; ++j) {
                const float o = side * kAssistReach * static_cast<float>(j) / kAssistProbes;
                clear = standAt(w, w.form, p.x + ux * o, p.z + uz * o, feet, f);
                feet = f;
            }
            if (!clear || !standAt(w, w.form, p.x + side * ux * off + dx, p.z + side * uz * off + dz, feet, f)) continue;
            // One step's worth toward that side, no further than the offset itself.
            const float s = std::fmin(len, off) * side;
            if (!standAt(w, w.form, p.x + ux * s, p.z + uz * s, p.feet, f)) continue;
            p.x += ux * s;
            p.z += uz * s;
            p.feet = f;
            return true;
        }
    }
    return false;
}

// Moves the player by (dx, dz), sliding along whatever blocks the way: each
// axis is tried on its own, so walking into a fence at an angle follows it.
// Walking nearly straight into something, where a slide would barely move
// her, a sidestep round the end of it comes first.
void move(World& w, float dx, float dz) {
    Player& p = w.player;
    float feet = 0;
    if (standAt(w, w.form, p.x + dx, p.z + dz, p.feet, feet)) {
        p.x += dx;
        p.z += dz;
        p.feet = feet;
        return;
    }
    const float len = std::sqrt(dx * dx + dz * dz);
    const bool slideX = std::fabs(dx) >= kSlideAtLeast * len, slideZ = std::fabs(dz) >= kSlideAtLeast * len;
    for (int pass = 0; pass < 2; ++pass) {
        // First the slides worth taking and a sidestep the way she bears; then
        // any slide at all, and last a sidestep either way.
        if ((pass || slideX) && dx != 0 && standAt(w, w.form, p.x + dx, p.z, p.feet, feet)) {
            p.x += dx;
            p.feet = feet;
            return;
        }
        if ((pass || slideZ) && dz != 0 && standAt(w, w.form, p.x, p.z + dz, p.feet, feet)) {
            p.z += dz;
            p.feet = feet;
            return;
        }
        if (sidestep(w, dx, dz, pass == 1)) return;
    }
}

struct Fnv {
    uint64_t h = 1469598103934665603ULL;
    void bytes(const void* data, size_t n) {
        const auto* b = static_cast<const unsigned char*>(data);
        for (size_t i = 0; i < n; ++i) {
            h ^= b[i];
            h *= 1099511628211ULL;
        }
    }
    template <typename T>
    void add(T v) {
        bytes(&v, sizeof v);
    }
};

}  // namespace

const Body& body(Form f) { return f == Form::Cat ? kCat : kGirl; }

bool standAt(const World& w, Form form, float x, float z, float feet, float& out) {
    const Body& b = body(form);
    if (x < -town::kWalkLimit || x > town::kWalkLimit || z < -town::kWalkLimit || z > town::kWalkLimit) return false;
    float top = town::groundHeight(x, z);
    bool onSolid = false;
    for (const town::Solid& s : town::solids()) {
        if (s.gate >= 0 && w.gateOpen[static_cast<size_t>(s.gate)]) continue;
        if (!s.overlaps(x, z, b.radius)) continue;
        const float h = s.heightAt(x, z);
        if (h > feet + b.climb) return false;
        if (h > top) {
            top = h;
            onSolid = true;
        }
    }
    // The creek: nobody wades in, except over the bridge.
    if (!onSolid && top < town::kWaterY + 0.05f) return false;
    out = top;
    return true;
}

World newWorld(uint64_t seed) {
    World w;
    w.seed = seed;
    w.player.x = town::kStart.x;
    w.player.z = town::kStart.z;
    w.player.yaw = town::kStartYaw;
    float feet = 0;
    if (standAt(w, w.form, w.player.x, w.player.z, 10.0f, feet)) w.player.feet = feet;
    w.smokeX = kSmokeStart.x;
    w.smokeZ = kSmokeStart.z;
    return w;
}

void tick(World& w, const Input& in) {
    Player& p = w.player;
    p.yaw = in.yaw;
    p.pitch = clampf(in.pitch, -kMaxPitch, kMaxPitch);

    float f = clampf(in.forward, -1.0f, 1.0f), s = clampf(in.strafe, -1.0f, 1.0f);
    const float len2 = f * f + s * s;
    if (len2 > 1.0f) {
        const float inv = 1.0f / std::sqrt(len2);
        f *= inv;
        s *= inv;
    }
    const Body& b = body(w.form);
    const float step = (in.hurry ? b.hurry : b.walk) / kTickRate;
    const float sy = detSin(p.yaw), cy = detCos(p.yaw);
    // Forward is (sin yaw, -cos yaw) on the ground; right is (cos yaw, sin yaw).
    if (f != 0 || s != 0) move(w, (sy * f + cy * s) * step, (-cy * f + sy * s) * step);
    ++w.tick;
    const Form before = w.form;
    if (in.toggleForm) {
        if (w.form == Form::Girl) w.form = Form::Cat;
        else if (canBecomeGirl(w)) w.form = Form::Girl;
    }
    // The bag, the latches and the spirits come first, then the night's
    // witchcraft: E used there, at a lantern, a tree or the pot, is not a knock.
    Input rest = in;
    if (catTick(w, in, before)) rest.use = false;
    if (craftTick(w, rest)) {
        rest.use = false;
        rest.trick = false;
    }
    doorsTick(w, rest);
    notebookTick(w);
}

bool canBecomeGirl(const World& w) {
    if (w.form == Form::Girl) return true;
    const Player& p = w.player;
    if (p.feet > town::groundHeight(p.x, p.z) + 0.6f) return false;  // up on something
    float feet = 0;
    return standAt(w, Form::Girl, p.x, p.z, p.feet, feet);
}

Vec3 eyePosition(const World& w) { return {w.player.x, w.player.feet + body(w.form).eye, w.player.z}; }

double gameMinutes(const World& w) {
    return kStartMinute + static_cast<double>(w.tick) / kTickRate * kGameMinutesPerSecond;
}

std::string clockText(const World& w) {
    const int minute = static_cast<int>(gameMinutes(w)) % 1440;
    char buf[32];
    std::snprintf(buf, sizeof buf, "%d:%02d %s", (minute / 60 + 11) % 12 + 1, minute % 60, minute < 720 ? "am" : "pm");
    return buf;
}

uint64_t fingerprint(const World& w) {
    Fnv f;
    f.add(w.seed);
    f.add(w.tick);
    f.add(w.player.x);
    f.add(w.player.z);
    f.add(w.player.feet);
    f.add(w.player.yaw);
    f.add(w.player.pitch);
    f.add(w.form);
    for (bool g : w.gateOpen) f.add(g);
    f.add(w.candy);
    for (bool k : w.knocked) f.add(k);
    f.add(w.answered);
    f.add(w.answeredTick);
    f.add(w.answer);
    f.add(w.asking);
    f.add(w.jokes);
    f.add(w.costume);
    f.add(w.bagDown);
    f.add(w.bag.x);
    f.add(w.bag.y);
    f.add(w.bag.z);
    f.add(w.spiritsSeen);
    for (bool l : w.lit) f.add(l);
    f.add(w.apples);
    f.add(w.rosemary);
    f.add(w.cinnamon);
    f.add(w.stirs);
    f.add(w.cider);
    f.add(w.offered);
    f.add(w.smoke);
    f.add(w.smokeX);
    f.add(w.smokeZ);
    f.add(w.homeTick);
    f.add(w.pages);
    f.add(w.secrets);
    f.add(w.deed);
    f.add(w.deedArg);
    f.add(w.deedTick);
    return f.h;
}

}  // namespace familiar
