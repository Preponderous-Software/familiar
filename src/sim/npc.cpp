#include "sim/npc.h"

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

Walker nana(const World&) {
    Walker n;
    const Vec2 d = T::doorstep(T::Nana);
    n.at = {d.x - 1.2f, T::kPorchHeight, d.z + 0.6f};
    n.yaw = kPi;
    n.height = 1.62f;
    n.shown = true;
    return n;
}

}  // namespace familiar
