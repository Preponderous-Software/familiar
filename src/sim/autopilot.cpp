#include "sim/autopilot.h"

#include "sim/cat.h"
#include "sim/doors.h"
#include "sim/town.h"

namespace familiar {

namespace T = familiar::town;

namespace {

constexpr float kLane = 3.0f;       // |z| it walks Elm Street at, inside the road
constexpr float kYard = 4.5f;       // |z| beyond which it is on a front walk, not the street
constexpr float kOnWalk = 0.2f;     // how close in x counts as on a house's front walk
constexpr float kArrive = 0.1f;     // how close counts as there
constexpr float kSlowWithin = 0.6f; // walks (not hurries) this close to where it is going

float sign(float v) { return v < 0 ? -1.0f : 1.0f; }

float dist2(Vec2 a, Vec2 b) { return (a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z); }

Vec2 here(const World& w) { return {w.player.x, w.player.z}; }

// Whether a house gives candy at its door tonight, and has not yet.
bool worthKnocking(const World& w, int h) {
    const auto id = static_cast<T::HouseId>(h);
    return h != T::Nana && !w.knocked[static_cast<size_t>(h)] && neighbour(id).candy > 0 && porchLightOn(w, id);
}

// Where to walk next on the way to `to` (a doorstep): out along the front walk
// it is on, along Elm Street to the right house's walk, then up to the door.
Vec2 nextPoint(const World& w, Vec2 to) {
    const Vec2 p = here(w);
    const bool onItsWalk = p.x - to.x < kOnWalk && to.x - p.x < kOnWalk && sign(p.z) == sign(to.z);
    if (onItsWalk) return to;
    if (p.z > kYard || p.z < -kYard) return {p.x, sign(p.z) * kLane};  // back out to the street
    return {to.x, sign(to.z) * kLane};
}

// Walks toward `to`, hurrying until it is close.
void walkTo(const World& w, Vec2 to, Input& in) {
    const Vec2 p = here(w), next = nextPoint(w, to);
    const float d2 = dist2(p, next);
    if (d2 < kArrive * kArrive * 0.25f) return;
    in.forward = 1;
    in.hurry = d2 > kSlowWithin * kSlowWithin;
    in.yaw = detAtan2(next.x - p.x, -(next.z - p.z));
    in.pitch = 0;
}

bool at(const World& w, Vec2 spot) { return dist2(here(w), spot) < kArrive * kArrive; }

// The yaw that faces a house's door from its doorstep.
float facingDoor(int h) {
    const auto id = static_cast<T::HouseId>(h);
    const Vec2 d = T::doorstep(id), door = T::doorAt(id);
    return detAtan2(door.x - d.x, -(door.z - d.z));
}

}  // namespace

int autopilotTarget(const World& w) {
    // West to east along the street, whichever side the house is on.
    int best = -1;
    for (int h = 0; h < T::HouseCount; ++h)
        if (worthKnocking(w, h) && (best < 0 || T::kHouses[static_cast<size_t>(h)].x < T::kHouses[static_cast<size_t>(best)].x))
            best = h;
    return best;
}

bool playthroughDone(const World& w) {
    for (int h = 0; h < T::HouseCount; ++h) {
        const auto id = static_cast<T::HouseId>(h);
        if (h != T::Nana && neighbour(id).candy > 0 && !w.knocked[static_cast<size_t>(h)]) return false;
    }
    return w.form == Form::Girl && at(w, T::doorstep(T::Nana));
}

const char* autopilotDoing(const World& w) {
    if (w.form == Form::Cat) return "turning back into Wren";
    if (w.costume == Costume::None && canChooseCostume(w)) return "choosing a costume";
    if (w.bagDown) return "fetching the candy bag";
    if (w.asking >= 0) return "telling a joke";
    const int h = autopilotTarget(w);
    if (h < 0) return playthroughDone(w) ? "home on Nana's porch" : "walking home to Nana's";
    static const char* const kAt[T::HouseCount] = {
        "", "knocking at the Okafors'", "", "knocking at the Lindqvists'", "knocking at Dr. Pell's",
        "knocking at the Garcias'", "knocking at Ms. Bright's", "knocking at the Novaks'", "knocking at Abbas's Market",
        "knocking at the Whitfields'"};
    return kAt[h];
}

Input autopilot(const World& w) {
    Input in;
    in.yaw = w.player.yaw;
    in.pitch = 0;
    if (w.form == Form::Cat) {  // the night's knocking is Wren's
        in.toggleForm = true;
        return in;
    }
    if (w.costume == Costume::None && canChooseCostume(w)) {
        in.costume = static_cast<int32_t>(kAutopilotCostume);
        return in;
    }
    if (w.bagDown) {  // the candy bag first: no door gives candy without it
        if (nearBag(w)) {
            in.use = true;
            return in;
        }
        walkTo(w, {w.bag.x, w.bag.z}, in);
        return in;
    }
    if (w.asking >= 0 && atDoor(w) == w.asking) {
        in.trick = true;
        return in;
    }
    const int h = autopilotTarget(w);
    const Vec2 to = T::doorstep(static_cast<T::HouseId>(h < 0 ? T::Nana : h));
    if (!at(w, to)) {
        walkTo(w, to, in);
        return in;
    }
    if (h < 0) return in;  // home
    in.yaw = facingDoor(h);
    in.use = true;
    return in;
}

}  // namespace familiar
