// The NEXT marker's way (#25, #26): followed step by step, the marker gets
// Wren (or the cat) to the place NEXT names, round house walls, fences and the
// creek, and never points into something she cannot walk through. On Smoke's
// trail it leads on toward the oak, not back to the herb bed.
#include <cmath>
#include <cstdio>

#include "check.h"
#include "sim/craft.h"
#include "sim/town.h"
#include "sim/ways.h"
#include "sim/world.h"

using namespace familiar;
namespace T = familiar::town;
namespace W = familiar::ways;

namespace {

void standAt(World& w, Vec2 p) {
    w.player.x = p.x;
    w.player.z = p.z;
    float feet = 0;
    if (familiar::standAt(w, w.form, p.x, p.z, 10.0f, feet)) w.player.feet = feet;
}

float flat(Vec2 a, Vec3 b) { return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z)); }

// Walks wherever the marker points, as a player holding W toward it would,
// until within `near` of `stand`. Every aim off the goal must be a straight
// walk from where she is. True if she got there, and how many ticks it took.
bool follow(World& w, Vec2 stand, Vec3 goal, float near, int limit, int* ticks = nullptr, int* detours = nullptr) {
    for (int t = 0; t < limit; ++t) {
        const Vec2 p{w.player.x, w.player.z};
        if (W::dist(p, stand) < near) {
            if (ticks) *ticks = t;
            return true;
        }
        const W::Wayfinding f = W::wayfind(w, stand, goal);
        Vec2 to = stand;
        if (!f.direct) {
            to = {f.aim.x, f.aim.z};
            if (detours) ++*detours;
            if (!W::straightWalk(w, w.form, p, w.player.feet, to)) {
                std::fprintf(stderr, "aim (%.1f, %.1f) is not a straight walk from (%.1f, %.1f)\n", to.x, to.z, p.x, p.z);
                return false;
            }
            if (f.left + 0.01f < W::dist(p, stand)) return false;  // a way is never shorter than a straight line
        }
        Input in;
        in.forward = 1;
        in.hurry = true;
        in.yaw = detAtan2(to.x - p.x, -(to.z - p.z));
        tick(w, in);
    }
    return false;
}

// The same, for whatever NEXT points at (the trail moves on as she walks).
bool followNext(World& w, int limit, bool (*done)(const World&)) {
    for (int t = 0; t < limit; ++t) {
        if (done(w)) return true;
        const NextStep n = tonightStep(w);
        if (!n.goal) return false;
        const Vec2 p{w.player.x, w.player.z};
        const W::Wayfinding f = W::wayfind(w, n.stand, n.at);
        const Vec2 to = f.direct ? n.stand : Vec2{f.aim.x, f.aim.z};
        Input in;
        in.forward = 1;
        in.hurry = true;
        in.yaw = detAtan2(to.x - p.x, -(to.z - p.z));
        tick(w, in);
    }
    return done(w);
}

}  // namespace

int main() {
    // ---- In the open, the marker points straight at the goal.
    {
        World w = newWorld(1);
        standAt(w, {-40.0f, -2.5f});
        const Vec3 door{-30.0f, 1.65f, -11.0f};
        const W::Wayfinding f = W::wayfind(w, T::doorstep(T::Hale), door);
        CHECK(f.direct && f.aim.x == door.x && f.aim.z == door.z);
    }

    // ---- Beside the Whitfields' house (#25): the straight line to the door is through its
    // side wall, so the marker leads round by the front, and following it gets her to the porch.
    {
        World w = newWorld(1);
        w.costume = Costume::Ghost;
        standAt(w, {42.0f, 15.0f});
        const Vec2 stand = T::doorstep(T::Whitfield);
        CHECK(!W::straightWalk(w, w.form, {w.player.x, w.player.z}, w.player.feet, stand));
        const W::Wayfinding f = W::wayfind(w, stand, {stand.x, 1.6f, stand.z});
        CHECK(!f.direct);
        CHECK(f.left > W::dist({w.player.x, w.player.z}, stand));
        int detours = 0;
        CHECK(follow(w, stand, {stand.x, 1.6f, stand.z}, 0.6f, 30 * 30, nullptr, &detours));
        CHECK(detours > 0);
    }

    // ---- Across the creek from the orchard, after dark (#25): the marker goes by the bridge.
    {
        World w = newWorld(1);
        w.costume = Costume::Ghost;
        standAt(w, {44.0f, 14.0f});
        Vec2 tree{};
        for (const T::Tree& t : T::trees())
            if (t.apple) {
                tree = {t.at.x, t.at.z - 1.4f};
                break;
            }
        CHECK(tree.x > 60.0f);  // across the creek
        CHECK(!W::straightWalk(w, w.form, {w.player.x, w.player.z}, w.player.feet, tree));
        const W::Wayfinding f = W::wayfind(w, tree, {tree.x, 1.8f, tree.z});
        CHECK(!f.direct);
        CHECK(std::fabs(f.aim.z) < 6.0f);  // toward the street and its bridge, not straight over the water
        CHECK(follow(w, tree, {tree.x, 1.8f, tree.z}, 1.0f, 30 * 60));
        CHECK(w.player.x > T::creekX(w.player.z));
    }

    // ---- Every door on the street, from Nana's front walk: following the marker gets there.
    for (int h = 1; h < T::HouseCount; ++h) {
        World w = newWorld(1);
        w.costume = Costume::Ghost;
        const auto id = static_cast<T::HouseId>(h);
        const Vec2 stand = T::doorstep(id);
        const Vec2 d = T::doorAt(id);
        int ticks = 0;
        CHECK(follow(w, stand, {d.x, 1.65f, d.z}, 0.6f, 30 * 120, &ticks));
        if (h == T::Whitfield) CHECK(ticks > 0);
    }

    // ---- Into Nana's back yard for the pot, from the street: up the side passage, not into her wall.
    {
        World w = newWorld(1);
        w.costume = Costume::Ghost;
        standAt(w, {-66.0f, -2.5f});
        const Vec2 stand{T::kFirePit.x, T::kFirePit.z + 1.5f};
        CHECK(!W::straightWalk(w, w.form, {w.player.x, w.player.z}, w.player.feet, stand));
        CHECK(follow(w, stand, {T::kFirePit.x, 0.6f, T::kFirePit.z}, 0.6f, 30 * 60));
    }

    // ---- "Come closer to knock" is not offered through a house's wall (#25).
    {
        World w = newWorld(1);
        w.costume = Costume::Ghost;
        w.lit[NanaWindow] = true;
        const Vec2 d = T::doorAt(T::Whitfield);  // a south house: its door faces north
        standAt(w, {d.x, d.z - 5.0f});
        w.player.yaw = kPi;  // facing south, at the door
        CHECK(reachHint(w).find("Whitfields'") != std::string::npos);
        standAt(w, {41.5f, 14.0f});  // beside the house, facing the door through its side wall
        w.player.yaw = detAtan2(d.x - w.player.x, -(d.z - w.player.z));
        CHECK(reachHint(w).find("knock") == std::string::npos);
    }

    // ---- NEXT names where to stand for its place: a door's doorstep, a lantern's spot.
    {
        World w = newWorld(1);
        w.costume = Costume::Ghost;
        NextStep n = tonightStep(w);
        CHECK(n.goal && n.stand.x == lantern(NanaWindow).stand.x && n.stand.z == lantern(NanaWindow).stand.z);
        w.lit[NanaWindow] = true;
        w.knocked[T::Garcia] = true;
        standAt(w, {-30.0f, 0.0f});
        n = tonightStep(w);
        CHECK(n.goal && n.stand.x == T::doorstep(T::Bright).x && n.stand.z == T::doorstep(T::Bright).z);
    }

    // ---- Smoke's trail (#26): from Nana's herb bed, the cat's marker leads on along the
    // paw prints, never back to the bed, and following it finds Smoke under the oak.
    {
        World w = newWorld(1);
        Input q;
        q.yaw = w.player.yaw;
        q.toggleForm = true;
        tick(w, q);
        CHECK(w.form == Form::Cat);
        standAt(w, T::kHerbBed);
        const NextStep n = tonightStep(w);
        CHECK(n.goal);
        CHECK(n.text.find("creek") != std::string::npos);  // a cue on reaching the bed
        CHECK(flat(T::kHerbBed, n.at) > 5.0f);           // not the bed itself
        // A little way along, it still points on, away from the bed.
        standAt(w, {-50.0f, -30.1f});
        const NextStep m = tonightStep(w);
        CHECK(flat(T::kHerbBed, m.at) > flat(T::kHerbBed, {w.player.x, 0.0f, w.player.z}));
        // From the herb bed to Smoke, following only the marker.
        standAt(w, T::kHerbBed);
        CHECK(followNext(w, 30 * 240, [](const World& x) { return x.smoke == SmokeState::Following; }));
        // Far from the trail, it names the nearest prints, without the cue.
        World far = newWorld(1);
        tick(far, q);
        standAt(far, {-66.0f, -2.5f});
        const NextStep f = tonightStep(far);
        CHECK(f.text.find("herb bed") != std::string::npos);
    }

    TEST_MAIN_END();
}
