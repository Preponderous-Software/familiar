#include "sim/autopilot.h"

#include <cmath>

#include "sim/cat.h"
#include "sim/craft.h"
#include "sim/doors.h"
#include "sim/npc.h"
#include "sim/town.h"
#include "sim/ways.h"

namespace familiar {

namespace T = familiar::town;

using namespace familiar::ways;

namespace {

constexpr float kArrive = 0.1f;      // how close counts as there
constexpr float kSlowWithin = 0.6f;  // walks (not hurries) this close to where it is going
constexpr float kLookAhead = 0.6f;   // steers for a point this far along the way
constexpr float kDirect = 12.0f;     // walks straight to a loose goal (the bag) within this, if nothing is in the way
constexpr float kJoin = 8.0f;        // joins a stretch of the way no further off than this

Vec2 here(const World& w) { return {w.player.x, w.player.z}; }

bool at(const World& w, Vec2 spot) { return dist2(here(w), spot) < kArrive * kArrive; }

// Walks toward `to` (no further than it), hurrying while `left` is long.
void stepToward(const World& w, Vec2 to, float left, Input& in) {
    const Vec2 p = here(w);
    if (dist2(p, to) < kArrive * kArrive * 0.25f) return;
    in.forward = 1;
    in.hurry = left > kSlowWithin;
    in.yaw = detAtan2(to.x - p.x, -(to.z - p.z));
    in.pitch = 0;
}

// Along the way from `p` (on the stretch toward place `n`) `ahead` metres, or to the goal.
Vec2 along(const Routes& r, Vec2 p, int n, float ahead) {
    while (n >= 0) {
        const float d = dist(p, kNodes[n]);
        if (d >= ahead) {
            const float k = ahead / d;
            return {p.x + (kNodes[n].x - p.x) * k, p.z + (kNodes[n].z - p.z) * k};
        }
        ahead -= d;
        p = kNodes[n];
        n = r.next[n];
    }
    return p;
}

// Walks to place `goal` along the shortest way there: of every stretch of it
// that it can walk straight onto (near enough, with nothing in the way), the
// one that leaves the least to walk, steering for a point a little along it.
void walkTo(const World& w, int goal, Input& in) {
    const Vec2 p = here(w), g = kNodes[goal];
    if (dist2(p, g) < kArrive * kArrive * 0.25f) return;
    const Routes r = routesTo(w, goal);
    struct Pick {
        float off, left;  // how far off the stretch, and how far to the goal by it
        Vec2 aim;
    };
    Pick picks[NodeCount];
    int count = 0, nearest = -1;
    for (int a = 0; a < NodeCount; ++a) {
        const int b = r.next[a];
        if (b < 0) continue;
        const Vec2 A = kNodes[a], B = kNodes[b];
        const float len2 = dist2(A, B);
        float t = len2 > 0 ? ((p.x - A.x) * (B.x - A.x) + (p.z - A.z) * (B.z - A.z)) / len2 : 0.0f;
        t = clampf(t, 0.0f, 1.0f);
        const Vec2 on{A.x + (B.x - A.x) * t, A.z + (B.z - A.z) * t};
        const Pick k{dist(p, on), dist(on, B) + r.far[b], along(r, on, b, kLookAhead)};
        if (nearest < 0 || k.off < picks[nearest].off) nearest = count;
        picks[count++] = k;
    }
    if (count == 0) {  // nowhere on the ways leads there: straight for it
        stepToward(w, g, dist(p, g), in);
        return;
    }
    bool tried[NodeCount] = {};
    for (int tries = 0; tries < 8; ++tries) {
        int best = -1;
        for (int i = 0; i < count; ++i) {
            if (tried[i] || picks[i].off > kJoin) continue;
            if (best < 0 || picks[i].off + picks[i].left < picks[best].off + picks[best].left) best = i;
        }
        if (best < 0) break;
        tried[best] = true;
        if (straightWalk(w, w.form, p, w.player.feet, picks[best].aim)) {
            stepToward(w, picks[best].aim, picks[best].off + picks[best].left, in);
            return;
        }
    }
    stepToward(w, picks[nearest].aim, picks[nearest].off + picks[nearest].left, in);
}

// Walks to a spot that is not one of the places (the bag, Smoke): straight
// there when it is near and nothing is in the way, else by the place nearest it.
void walkToSpot(const World& w, Vec2 s, Input& in) {
    const Vec2 p = here(w);
    if (dist2(p, s) < kDirect * kDirect && straightWalk(w, w.form, p, w.player.feet, s)) {
        stepToward(w, s, dist(p, s), in);
        return;
    }
    walkTo(w, nearestPlace(s), in);
}

float facing(Vec2 from, Vec2 to) { return detAtan2(to.x - from.x, -(to.z - from.z)); }

// ---- what the night needs next

enum class Job {
    Costume, Joke, Light, Knock, Buy, Lychgate, Apples, HaleGate, Rosemary, Cider, FindSmoke, LeadSmoke, Offer, Midnight, Done
};

struct Plan {
    Job job;
    int arg = -1;  // the lantern, or the house
};

// Whether a house gives candy at its door tonight, and has not yet.
bool worthKnocking(const World& w, int h) {
    const auto id = static_cast<T::HouseId>(h);
    return h != T::Nana && !w.knocked[static_cast<size_t>(h)] && neighbour(id).candy > 0 && porchLightOn(w, id);
}

Plan plan(const World& w) {
    if (w.smoke == SmokeState::Home) return {Job::Done};
    if (w.form == Form::Girl && w.asking >= 0 && atDoor(w) == w.asking) return {Job::Joke};
    if (w.costume == Costume::None && canChooseCostume(w)) return {Job::Costume};
    if (!w.lit[NanaWindow]) return {Job::Light, NanaWindow};
    if (w.knocked[T::Okafor] && !w.lit[OkaforPorch]) return {Job::Light, OkaforPorch};
    if (w.knocked[T::Market] && !w.cinnamon && !w.cider && w.stirs == 0) return {Job::Buy};
    const int h = autopilotTarget(w);
    if (h >= 0) return {Job::Knock, h};
    if (!w.gateOpen[T::Lychgate]) return {Job::Lychgate};
    if (!w.lit[LychLantern]) return {Job::Light, LychLantern};
    if (w.apples < kApplesWanted && !w.cider && w.stirs == 0) return {Job::Apples};
    if (!w.gateOpen[T::HaleGate]) return {Job::HaleGate};
    if (!w.lit[HaleStep]) return {Job::Light, HaleStep};
    if (!w.rosemary && !w.cider && w.stirs == 0) return {Job::Rosemary};
    if (!w.cider) return {Job::Cider};
    if (w.smoke == SmokeState::Lost || w.smoke == SmokeState::Waiting) return {Job::FindSmoke};
    if (w.smoke == SmokeState::Following) return {Job::LeadSmoke};
    if (!w.offered) return {Job::Offer};
    return {Job::Midnight};
}

bool catsJob(Job j) { return j == Job::Lychgate || j == Job::HaleGate || j == Job::FindSmoke || j == Job::LeadSmoke; }

// Where Wren leaves her bag and becomes the cat for a cat's job, and where the cat goes.
int changesAt(Job j) { return j == Job::Lychgate ? Lychgate : j == Job::HaleGate ? HaleGateOut : StonesGap; }
int catGoesTo(Job j) { return j == Job::Lychgate ? LychInside : j == Job::HaleGate ? CatHaleLatch : j == Job::FindSmoke ? CatOak : Circle; }

int lanternPlace(int l) {
    switch (l) {
        case NanaWindow: return NanaWindow_;
        case OkaforPorch: return OkaforLantern;
        case HaleStep: return HaleStep_;
        default: return Lychgate;
    }
}

int doorstepPlace(int h) {
    static const int kAt[T::HouseCount] = {Start, DOkafor, DHale, DLindqvist, DPell, DGarcia, DBright, DNovak, DMarket, DWhitfield};
    return kAt[h];
}

// Stands at `place` and presses E there, facing `face`.
Input useAt(const World& w, int place, Vec2 face) {
    Input in;
    in.yaw = w.player.yaw;
    in.pitch = 0;
    if (!at(w, kNodes[place])) {
        walkTo(w, place, in);
        return in;
    }
    in.yaw = facing(here(w), face);
    in.use = true;
    return in;
}

// Smoke, for the cat to find: where her trail ends, or wherever she waits.
bool smokeAway(const World& w) {
    return w.smoke == SmokeState::Waiting && dist2({w.smokeX, w.smokeZ}, kSmokeStart) > 1.0f;
}

}  // namespace

int autopilotWayCount() { return kWayCount; }

bool autopilotWayWalkable(int i) {
    const Way& e = kWays[i];
    World w = newWorld(1);
    w.gateOpen[T::HaleGate] = w.gateOpen[T::Lychgate] = true;
    const Form form = e.forms & kGirl ? Form::Girl : Form::Cat;
    for (int dir = 0; dir < (e.oneWay ? 1 : 2); ++dir) {
        const Vec2 from = kNodes[dir ? e.b : e.a], to = kNodes[dir ? e.a : e.b];
        float feet = 0;  // every place is on the ground, or a porch
        if (!standAt(w, form, from.x, from.z, T::groundHeight(from.x, from.z), feet)) return false;
        if (!straightWalk(w, form, from, feet, to)) return false;
    }
    return true;
}

int autopilotTarget(const World& w) {
    // West to east along the street, whichever side the house is on.
    int best = -1;
    for (int h = 0; h < T::HouseCount; ++h)
        if (worthKnocking(w, h) && (best < 0 || T::kHouses[static_cast<size_t>(h)].x < T::kHouses[static_cast<size_t>(best)].x))
            best = h;
    return best;
}

bool playthroughDone(const World& w) { return w.smoke == SmokeState::Home; }

const char* autopilotDoing(const World& w) {
    const Plan p = plan(w);
    const bool cat = w.form == Form::Cat;
    if (p.job == Job::Done) return "Smoke has gone home";
    if (catsJob(p.job)) {
        if (!cat && w.bagDown) return "fetching the candy bag";
        switch (p.job) {
            case Job::Lychgate: return cat ? "slipping into the churchyard to draw the bolt" : "going to the lychgate to be a cat";
            case Job::HaleGate: return cat ? "climbing in to Mr. Hale's latch" : "going to Mr. Hale's gate to be a cat";
            case Job::FindSmoke: return cat ? "finding Smoke" : "going up Lantern Hill to be a cat";
            default: return "leading Smoke to the stones";
        }
    }
    if (cat) return "turning back into Wren";
    if (w.bagDown) return "fetching the candy bag";
    switch (p.job) {
        case Job::Costume: return "choosing a costume";
        case Job::Joke: return "telling a joke";
        case Job::Light: {
            static const char* const kLight[kLanternCount] = {"lighting Nana's window lantern", "lighting Grandpa Ike's lantern",
                                                              "lighting June Hale's lantern", "lighting the lychgate lantern"};
            return kLight[p.arg];
        }
        case Job::Knock: {
            static const char* const kAt[T::HouseCount] = {
                "", "knocking at the Okafors'", "knocking at Mr. Hale's", "knocking at the Lindqvists'", "knocking at Dr. Pell's",
                "knocking at the Garcias'", "knocking at Ms. Bright's", "knocking at the Novaks'", "knocking at Abbas's Market",
                "knocking at the Whitfields'"};
            return kAt[p.arg];
        }
        case Job::Buy: return "buying a cinnamon stick";
        case Job::Apples: return "picking apples in the orchard";
        case Job::Rosemary: return "picking rosemary";
        case Job::Cider: return "brewing Nana's cider";
        case Job::Offer: return nanaArrived(w) ? "making the offering with Nana" : "waiting for Nana at the stones";
        case Job::Midnight: return "waiting with Nana and Smoke for midnight";
        default: break;
    }
    return "";
}

Input autopilot(const World& w) {
    Input in;
    in.yaw = w.player.yaw;
    in.pitch = 0;
    const Plan p = plan(w);
    if (p.job == Job::Done) return in;

    if (catsJob(p.job)) {
        if (w.form == Form::Girl) {
            if (w.bagDown) {  // take the bag along, to leave it where the cat sets off
                if (nearBag(w)) in.use = true;
                else walkToSpot(w, {w.bag.x, w.bag.z}, in);
                return in;
            }
            const int spot = changesAt(p.job);
            if (!at(w, kNodes[spot])) {
                walkTo(w, spot, in);
                return in;
            }
            in.toggleForm = true;
            return in;
        }
        if (p.job == Job::FindSmoke && smokeAway(w)) {
            walkToSpot(w, {w.smokeX, w.smokeZ}, in);
            return in;
        }
        const int goal = catGoesTo(p.job);
        if (!at(w, kNodes[goal])) {
            walkTo(w, goal, in);
            return in;
        }
        if (latchHere(w) >= 0) in.use = true;  // nose up the latch, draw the bolt
        return in;
    }

    // Everything else is Wren's, with her bag.
    if (w.form == Form::Cat) {
        if (canBecomeGirl(w)) in.toggleForm = true;
        else if (w.bagDown) walkToSpot(w, {w.bag.x, w.bag.z}, in);  // down off whatever the cat is on
        return in;
    }
    if (w.bagDown) {
        if (nearBag(w)) in.use = true;
        else walkToSpot(w, {w.bag.x, w.bag.z}, in);
        return in;
    }
    switch (p.job) {
        case Job::Costume: in.costume = static_cast<int32_t>(kAutopilotCostume); return in;
        case Job::Joke: in.trick = true; return in;
        case Job::Light: {
            const Vec3 l = lantern(p.arg).at;
            return useAt(w, lanternPlace(p.arg), {l.x, l.z});
        }
        case Job::Knock: return useAt(w, doorstepPlace(p.arg), T::doorAt(static_cast<T::HouseId>(p.arg)));
        case Job::Buy: {
            Input b = useAt(w, DMarket, T::doorAt(T::Market));
            if (b.use && haggling(w)) {  // short of candy: a joke will do
                b.use = false;
                b.trick = true;
            }
            return b;
        }
        case Job::Apples: {
            const Vec2 s = spotStand(4);
            return useAt(w, Orchard, {s.x, s.z + 1.4f});
        }
        case Job::Rosemary: return useAt(w, HerbBed, T::kHerbBed);
        case Job::Cider: return useAt(w, Pot, T::kFirePit);
        case Job::Offer:
        case Job::Midnight: {
            Input o = useAt(w, Circle, T::kHill);
            o.use = o.use && p.job == Job::Offer && nanaArrived(w);  // with Nana, then wait for midnight
            return o;
        }
        default: break;
    }
    return in;
}

}  // namespace familiar
