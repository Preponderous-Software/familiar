#include "sim/world.h"

#include <cmath>
#include <cstdio>

#include "sim/cat.h"
#include "sim/doors.h"

namespace familiar {

namespace {

constexpr float kMaxPitch = 1.45f;  // just short of straight up or down

// Wren walks and hurries as a twelve-year-old does; the cat is low, quick and
// jumps about a metre and a half onto things.
constexpr Body kGirl{0.3f, 0.5f, 1.45f, 1.5f, 3.6f};
constexpr Body kCat{0.15f, 1.6f, 0.28f, 2.2f, 5.5f};

// Moves the player by (dx, dz), sliding along whatever blocks the way: each
// axis is tried on its own, so walking into a fence at an angle follows it.
void move(World& w, float dx, float dz) {
    Player& p = w.player;
    float feet = 0;
    if (standAt(w, w.form, p.x + dx, p.z + dz, p.feet, feet)) {
        p.x += dx;
        p.z += dz;
    } else if (standAt(w, w.form, p.x + dx, p.z, p.feet, feet)) {
        p.x += dx;
    } else if (standAt(w, w.form, p.x, p.z + dz, p.feet, feet)) {
        p.z += dz;
    } else {
        return;
    }
    p.feet = feet;
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
    // The bag, the latches and the spirits come first: E used there is not a knock.
    Input rest = in;
    if (catTick(w, in, before)) rest.use = false;
    doorsTick(w, rest);
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
    return f.h;
}

}  // namespace familiar
