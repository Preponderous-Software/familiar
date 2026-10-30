#include "audio/cues.h"

#include <cmath>

#include "sim/craft.h"
#include "sim/town.h"

namespace familiar::audio {

namespace T = familiar::town;

namespace {

int bits(uint32_t v) {
    int n = 0;
    for (; v; v &= v - 1) ++n;
    return n;
}

int count(const std::array<bool, kLanternCount>& a) {
    int n = 0;
    for (bool b : a) n += b ? 1 : 0;
    return n;
}

// Whole hours of the clock since the night began (17 at 5 pm, 24 at midnight).
int hour(const World& w) { return static_cast<int>(gameMinutes(w)) / 60; }

constexpr int kDusk = 18 * 60;  // owls from about six (sunset is twenty to six)

}  // namespace

Ambience ambience(const World& w, Vec3 listener, float stepsPerSecond) {
    Ambience a;
    const float ground = T::groundHeight(listener.x, listener.z);
    a.wind = clampf(0.3f + ground / 25.0f, 0.0f, 0.9f);  // the hill top is windy

    // The creek: full at the water's edge, half at 8 m.
    const float off = std::fabs(listener.x - T::creekX(listener.z)) - T::kCreekHalfWidth;
    const float toWater = off > 0 ? off : 0.0f;
    a.creek = 1.0f / (1.0f + (toWater / 8.0f) * (toWater / 8.0f));

    // Leaves: everywhere a little (it is late October), more near the trees.
    float nearest = 1e9f;
    for (const T::Tree& t : T::trees()) {
        const float dx = t.at.x - listener.x, dz = t.at.z - listener.z;
        const float d2 = dx * dx + dz * dz;
        if (d2 < nearest) nearest = d2;
    }
    a.leaves = clampf(0.25f + 0.6f * (1.0f - std::sqrt(nearest) / 15.0f), 0.25f, 0.85f);

    // An owl now and then once it is dark, more often toward the woods.
    const int minute = static_cast<int>(gameMinutes(w));
    if (minute >= kDusk) {
        const bool woods = listener.z < -50 || listener.x > 62;
        a.owl = woods ? 1.0f : 0.45f;
    }

    // Smoke purrs when she is close.
    Vec3 at;
    float yaw = 0;
    if (w.smoke != SmokeState::Lost && w.smoke != SmokeState::Home && smokeShown(w, at, yaw)) {
        const float dx = at.x - w.player.x, dz = at.z - w.player.z;
        const float d = std::sqrt(dx * dx + dz * dz);
        a.purr = clampf(1.0f - (d - 1.0f) / 3.0f, 0.0f, 1.0f);
    }

    a.steps = stepsPerSecond;
    a.cat = w.form == Form::Cat;
    const Player& p = w.player;
    if (T::kBridge.contains(p.x, p.z) || p.feet > T::groundHeight(p.x, p.z) + 0.3f) a.ground = Ground::Wood;  // the bridge, a porch, a fence
    else if (p.z > -T::kRoadHalf && p.z < T::kRoadHalf && p.x > T::kStreetWest && p.x < T::kStreetEast) a.ground = Ground::Road;
    else a.ground = Ground::Leaves;
    return a;
}

std::vector<Cue> cues(const World& before, const World& after) {
    std::vector<Cue> out;
    // A load, a new night or a teleport is not something that happened.
    if (after.seed != before.seed || after.tick < before.tick || after.tick - before.tick > kTickRate) return out;

    if (after.answeredTick != before.answeredTick && after.answered >= 0) {
        out.push_back(Cue::Knock);
        if (after.answer == Answer::Kitty) out.push_back(Cue::Meow);
    }
    if (after.candy > before.candy) out.push_back(Cue::Candy);
    if (after.form != before.form) {
        out.push_back(after.form == Form::Cat ? Cue::Change : Cue::ChangeBack);
        if (after.form == Form::Cat) out.push_back(Cue::Meow);
    }
    if (after.bagDown != before.bagDown) out.push_back(Cue::Bag);
    for (size_t g = 0; g < after.gateOpen.size(); ++g)
        if (after.gateOpen[g] && !before.gateOpen[g]) out.push_back(Cue::Latch);
    if (count(after.lit) > count(before.lit)) out.push_back(Cue::Lantern);
    if (after.apples > before.apples || (after.rosemary && !before.rosemary)) out.push_back(Cue::Pick);
    if (after.cinnamon && !before.cinnamon) out.push_back(Cue::ShopBell);
    if (after.stirs > before.stirs) out.push_back(after.cider && !before.cider ? Cue::Brewed : Cue::Stir);
    if (bits(after.spiritsSeen) > bits(before.spiritsSeen)) out.push_back(Cue::Spirit);
    if (bits(after.pages) > bits(before.pages) || bits(after.secrets) > bits(before.secrets)) out.push_back(Cue::Page);
    if (after.smoke != before.smoke) {
        if (after.smoke == SmokeState::Following) out.push_back(Cue::Found);
        if (after.smoke == SmokeState::AtCircle) out.push_back(Cue::Settle);
        if (after.smoke == SmokeState::Home) out.push_back(Cue::Home);
    }
    if (after.offered && !before.offered) out.push_back(Cue::Offering);
    // The church clock strikes the hour.
    if (hour(after) > hour(before)) {
        const int strokes = (hour(after) + 11) % 12 + 1;
        for (int i = 0; i < strokes; ++i) out.push_back(Cue::Bell);
    }
    return out;
}

}  // namespace familiar::audio
