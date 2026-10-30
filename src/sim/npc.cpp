#include "sim/npc.h"

#include <cmath>

#include "sim/doors.h"
#include "sim/town.h"

namespace familiar {

namespace T = familiar::town;

namespace {

// Each child walks a loop: east along one sidewalk, back west along the other.
struct Loop {
    float speed;   // metres per second
    float offset;  // metres along the loop at the start
    float lane;    // |z| of the sidewalk they keep to
    int look;
    float height;
};
const Loop kLoops[kChildCount] = {
    {0.9f, 0.0f, 4.7f, 1, 1.25f},   {0.9f, 1.4f, 4.4f, 3, 1.35f},  {0.8f, 60.0f, 4.8f, 2, 1.1f},
    {1.0f, 130.0f, 4.6f, 4, 1.4f},  {0.85f, 190.0f, 4.7f, 1, 1.2f}, {0.85f, 191.5f, 4.5f, 0, 1.75f},
    {0.95f, 250.0f, 4.6f, 3, 1.3f},
};
constexpr float kWest = -80.0f, kEast = 40.0f;

}  // namespace

std::vector<Walker> children(const World& w) {
    std::vector<Walker> out(kChildCount);
    const float minute = static_cast<float>(gameMinutes(w));
    const bool out_ = minute >= 17 * 60 + 35 && minute < kPorchLightsOff + 5;
    const float seconds = static_cast<float>(w.tick) / kTickRate;
    const float span = kEast - kWest, loop = 2 * span + 2 * 9.4f;
    for (int i = 0; i < kChildCount; ++i) {
        const Loop& l = kLoops[i];
        Walker& c = out[static_cast<size_t>(i)];
        c.look = l.look;
        c.height = l.height;
        c.shown = out_;
        float s = l.offset + l.speed * seconds;
        s = s - loop * static_cast<float>(static_cast<int>(s / loop));
        if (s < span) {  // east along the north sidewalk
            c.at = {kWest + s, 0, -l.lane};
            c.yaw = kPi / 2;
        } else if (s < span + 9.4f) {  // across at the east end
            c.at = {kEast, 0, -l.lane + (s - span) * (2 * l.lane / 9.4f)};
            c.yaw = kPi;
        } else if (s < 2 * span + 9.4f) {  // west along the south sidewalk
            c.at = {kEast - (s - span - 9.4f), 0, l.lane};
            c.yaw = -kPi / 2;
        } else {  // back across at the west end
            c.at = {kWest, 0, l.lane - (s - 2 * span - 9.4f) * (2 * l.lane / 9.4f)};
            c.yaw = 0;
        }
        // A little bob in the step.
        c.at.y = 0.03f * detSin(seconds * 7.0f + static_cast<float>(i));
    }
    return out;
}

namespace {

constexpr int kNanaLeaves = 22 * 60 + 30;  // half past ten
constexpr float kNanaSpeed = 1.2f;         // metres per second of play

// Nana's way to the stones: down her front walk, east along Elm Street, over
// the bridge and up the hill path, to her place inside the circle.
const Vec2 kNanaPath[] = {
    {-67.2f, -9.2f}, {-66.0f, -7.0f}, {-66.0f, -2.6f}, {42.0f, -2.6f}, {45.0f, 0.0f},
    {59.5f, 0.0f},   {61.0f, -3.0f},  {76.0f, -57.0f}, {78.8f, -60.8f},
};
constexpr int kNanaPoints = sizeof kNanaPath / sizeof kNanaPath[0];

float segment(int i) {
    const float dx = kNanaPath[i + 1].x - kNanaPath[i].x, dz = kNanaPath[i + 1].z - kNanaPath[i].z;
    return std::sqrt(dx * dx + dz * dz);
}

float pathLength() {
    float l = 0;
    for (int i = 0; i + 1 < kNanaPoints; ++i) l += segment(i);
    return l;
}

// Seconds of play since Nana set off (negative before).
float sinceLeaving(const World& w) {
    const double minutes = gameMinutes(w) - kNanaLeaves;
    return static_cast<float>(minutes / kGameMinutesPerSecond);
}

float feetAt(float x, float z) {
    if (T::kBridge.contains(x, z)) return T::kBridgeDeck;
    return T::groundHeight(x, z);
}

}  // namespace

Walker nana(const World& w) {
    Walker n;
    n.height = 1.62f;
    n.shown = true;
    const float t = sinceLeaving(w);
    if (t <= 0) {
        const Vec2 d = T::doorstep(T::Nana);
        n.at = {d.x - 1.2f, T::kPorchHeight, d.z + 0.6f};
        n.yaw = kPi;
        return n;
    }
    float s = t * kNanaSpeed;
    for (int i = 0; i + 1 < kNanaPoints; ++i) {
        const float l = segment(i);
        const Vec2 a = kNanaPath[i], b = kNanaPath[i + 1];
        if (s <= l || i + 2 == kNanaPoints) {
            const float k = clampf(s / l, 0.0f, 1.0f);
            const float x = a.x + (b.x - a.x) * k, z = a.z + (b.z - a.z) * k;
            n.at = {x, feetAt(x, z) + 0.03f * detSin(t * 6.0f) * (k < 1 ? 1.0f : 0.0f), z};
            n.yaw = detAtan2(b.x - a.x, -(b.z - a.z));
            if (k >= 1) n.yaw = detAtan2(T::kHill.x - x, -(T::kHill.z - z));  // at the stones, facing the middle
            return n;
        }
        s -= l;
    }
    return n;
}

bool nanaArrived(const World& w) { return sinceLeaving(w) * kNanaSpeed >= pathLength(); }

int nanaArrivalMinute() {
    return kNanaLeaves + static_cast<int>(pathLength() / kNanaSpeed * kGameMinutesPerSecond) + 1;
}

}  // namespace familiar
