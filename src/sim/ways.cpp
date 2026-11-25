#include "sim/ways.h"

namespace familiar::ways {

namespace T = familiar::town;

const Vec2 kNodes[NodeCount] = {
    {-66.0f, -2.5f}, {-55.5f, -2.5f}, {-54.0f, -2.5f}, {-48.0f, -2.5f}, {-41.0f, -2.5f}, {-36.0f, -2.5f}, {-30.0f, -2.5f},
    {-18.0f, -2.5f}, {-12.0f, -2.5f}, {20.0f, -2.5f},  {34.0f, -2.5f},  {36.0f, -2.5f},  {44.0f, -2.5f},
    {-48.0f, -9.8f}, {-30.0f, -9.8f}, {-12.0f, -9.8f}, {34.0f, -9.8f},  {-54.0f, 9.8f},  {-36.0f, 9.8f},  {-18.0f, 9.8f},
    {20.0f, 9.8f},   {36.0f, 9.8f},
    {-66.0f, -7.2f}, {-68.2f, -9.9f}, {-55.5f, -23.0f}, {-62.5f, -22.8f}, {-68.0f, -24.0f},
    {-49.9f, -9.6f},
    {-41.0f, -26.0f}, {-38.0f, -26.0f}, {-34.0f, -26.0f}, {-30.0f, -24.0f}, {-30.0f, -21.6f},
    {-38.0f, -34.0f}, {-22.0f, -34.0f}, {-22.0f, -26.8f}, {-28.0f, -26.8f}, {-34.9f, -26.0f},
    {20.0f, -6.0f},  {23.5f, -28.8f}, {23.5f, -31.5f},
    {44.0f, -28.5f}, {44.0f, -40.0f}, {39.0f, -40.0f}, {39.0f, -31.5f},
    {45.0f, 0.0f},   {60.0f, 0.0f},   {61.0f, -3.0f},  {65.0f, 8.6f},  {75.0f, -55.0f}, {73.9f, -56.9f}, {80.4f, -59.9f},
    {65.5f, -19.0f}, {58.6f, -19.0f}, {59.2f, -23.6f},
};

const Way kWays[] = {
    // Elm Street.
    {SNana, SPassage, kBoth}, {SPassage, SGarcia, kBoth}, {SGarcia, SOkafor, kBoth}, {SOkafor, SHalePassage, kBoth},
    {SHalePassage, SBright, kBoth}, {SBright, SHale, kBoth}, {SHale, SNovak, kBoth}, {SNovak, SLindqvist, kBoth},
    {SLindqvist, SMarket, kBoth}, {SMarket, SPell, kBoth}, {SPell, SWhitfield, kBoth}, {SWhitfield, SEast, kBoth},
    // Up the front walks.
    {SOkafor, DOkafor, kBoth}, {SHale, DHale, kBoth}, {SLindqvist, DLindqvist, kBoth}, {SPell, DPell, kBoth},
    {SGarcia, DGarcia, kBoth}, {SBright, DBright, kBoth}, {SNovak, DNovak, kBoth}, {SMarket, DMarket, kBoth},
    {SWhitfield, DWhitfield, kBoth},
    {SNana, Start, kBoth}, {Start, NanaWindow_, kBoth}, {DOkafor, OkaforLantern, kBoth},
    // Nana's back yard, up the side passage between her house and the Okafors'.
    {SPassage, Passage, kBoth}, {Passage, HerbBed, kBoth}, {HerbBed, Pot, kBoth},
    // Mr. Hale's: the passage, his gate (open), his yard.
    {SHalePassage, HaleBack, kBoth}, {HaleBack, HaleGateOut, kBoth}, {HaleGateOut, HaleGateIn, kBoth, T::HaleGate},
    {HaleGateIn, HaleYard, kBoth}, {HaleYard, HaleStep_, kBoth},
    {HaleGateOut, CatHaleNW, kCat}, {CatHaleNW, CatHaleNE, kCat}, {CatHaleNE, CatWoodpile, kCat},
    {CatWoodpile, CatInYard, kCat, -1, true}, {CatInYard, CatHaleLatch, kBoth}, {CatHaleLatch, HaleGateIn, kBoth},
    // The churchyard.
    {SMarket, Green, kBoth}, {Green, Lychgate, kBoth}, {Lychgate, LychInside, kBoth, T::Lychgate},
    {Lychgate, CatPellBack, kCat}, {CatPellBack, CatHedgeOut, kCat}, {CatHedgeOut, CatHedgeIn, kCat},
    {CatHedgeIn, CatChurchyard, kCat}, {CatChurchyard, LychInside, kCat},
    // Over the bridge, the orchard, the hill.
    {SEast, BridgeWest, kBoth}, {BridgeWest, BridgeEast, kBoth}, {BridgeEast, HillFoot, kBoth}, {HillFoot, Orchard, kBoth},
    {HillFoot, HillPath, kBoth}, {HillPath, StonesGap, kBoth}, {StonesGap, Circle, kBoth},
    {HillPath, CatWoods, kCat}, {CatWoods, CatCreekside, kCat}, {CatCreekside, CatOak, kCat},
};
const int kWayCount = sizeof kWays / sizeof kWays[0];

int formBit(Form f) { return f == Form::Cat ? kCat : kGirl; }

bool usable(const World& w, const Way& e) {
    return (e.forms & formBit(w.form)) != 0 && (e.gate < 0 || w.gateOpen[static_cast<size_t>(e.gate)]);
}

bool straightWalk(const World& w, Form form, Vec2 from, float feet, Vec2 to) {
    const float d = dist(from, to);
    const int n = static_cast<int>(d / kSample) + 1;
    for (int i = 1; i <= n; ++i) {
        const float k = static_cast<float>(i) / static_cast<float>(n);
        float out = 0;
        if (!standAt(w, form, from.x + (to.x - from.x) * k, from.z + (to.z - from.z) * k, feet, out)) return false;
        feet = out;
    }
    return true;
}

Routes routesTo(const World& w, int goal) {
    Routes r;
    bool done[NodeCount] = {};
    for (int i = 0; i < NodeCount; ++i) {
        r.far[i] = 1e30f;
        r.next[i] = -1;
    }
    r.far[goal] = 0;
    for (;;) {
        int u = -1;
        for (int i = 0; i < NodeCount; ++i)
            if (!done[i] && r.far[i] < 1e29f && (u < 0 || r.far[i] < r.far[u])) u = i;
        if (u < 0) break;
        done[u] = true;
        for (const Way& e : kWays) {
            if (!usable(w, e)) continue;
            // Walking from v to u along this way.
            int v = -1;
            if (e.b == u) v = e.a;
            else if (e.a == u && !e.oneWay) v = e.b;
            if (v < 0 || done[v]) continue;
            const float d = r.far[u] + dist(kNodes[v], kNodes[u]);
            if (d < r.far[v]) {
                r.far[v] = d;
                r.next[v] = u;
            }
        }
    }
    return r;
}

int nearestPlace(Vec2 s) {
    int best = 0;
    for (int i = 1; i < NodeCount; ++i)
        if (dist2(kNodes[i], s) < dist2(kNodes[best], s)) best = i;
    return best;
}

namespace {

constexpr float kSeedWithin = 30.0f;  // places this near the goal may be the last before it
constexpr float kJoinWithin = 40.0f;  // the marker joins the ways at a place no further off than this
constexpr int kJoinTries = 24;        // straight walks tried, least left to walk first, to join them
constexpr float kAimUp = 1.0f;        // the marker floats over a place
constexpr float kAimStep = 2.0f;      // then looks on along the way in steps this long
constexpr float kAimAhead = 30.0f;    // no further on than this

float groundAt(Vec2 p) {
    const float g = T::groundHeight(p.x, p.z);
    return T::kBridge.contains(p.x, p.z) && g < T::kBridgeDeck ? T::kBridgeDeck : g;
}

// Whether the body could walk straight from place i to `to`, starting on the ground there.
bool walkFrom(const World& w, int i, Vec2 to) {
    const Vec2 from = kNodes[i];
    float feet = 0;
    if (!standAt(w, w.form, from.x, from.z, groundAt(from), feet)) return false;
    return straightWalk(w, w.form, from, feet, to);
}

// The shortest way from every place to a spot that is not one of them: the
// places it can be walked to straight from start the search, at their distance.
Routes routesToSpot(const World& w, Vec2 s) {
    Routes r;
    bool done[NodeCount] = {};
    bool seeded = false;
    for (int i = 0; i < NodeCount; ++i) {
        r.far[i] = 1e30f;
        r.next[i] = -1;
        const float d = dist(kNodes[i], s);
        if (d < kSeedWithin && walkFrom(w, i, s)) {
            r.far[i] = d;
            seeded = true;
        }
    }
    if (!seeded) {  // nothing walks straight there: the nearest place will do
        const int n = nearestPlace(s);
        r.far[n] = dist(kNodes[n], s);
    }
    for (;;) {
        int u = -1;
        for (int i = 0; i < NodeCount; ++i)
            if (!done[i] && r.far[i] < 1e29f && (u < 0 || r.far[i] < r.far[u])) u = i;
        if (u < 0) break;
        done[u] = true;
        for (const Way& e : kWays) {
            if (!usable(w, e)) continue;
            int v = -1;
            if (e.b == u) v = e.a;
            else if (e.a == u && !e.oneWay) v = e.b;
            if (v < 0 || done[v]) continue;
            const float d = r.far[u] + dist(kNodes[v], kNodes[u]);
            if (d < r.far[v]) {
                r.far[v] = d;
                r.next[v] = u;
            }
        }
    }
    return r;
}

}  // namespace

Wayfinding wayfind(const World& w, Vec2 stand, Vec3 goal) {
    const Vec2 p{w.player.x, w.player.z};
    const float feet = w.player.feet;
    const Wayfinding straight{goal, dist(p, stand), true};
    if (straightWalk(w, w.form, p, feet, stand)) return straight;
    const Routes r = routesToSpot(w, stand);
    // Every stretch of the shortest ways there, the last ones (a place to
    // the spot) included: where it passes nearest, and what it leaves to walk.
    auto end = [&](int a) { return r.next[a] >= 0 ? kNodes[r.next[a]] : stand; };
    auto endLeft = [&](int a) { return r.next[a] >= 0 ? r.far[r.next[a]] : 0.0f; };
    struct Join {
        int a = -1;
        Vec2 on{};
        float cost = 0;
    };
    Join joins[NodeCount];
    int count = 0;
    for (int a = 0; a < NodeCount; ++a) {
        if (r.far[a] > 1e29f) continue;
        const Vec2 A = kNodes[a], B = end(a);
        const float len2 = dist2(A, B);
        const float t = len2 > 0 ? clampf(((p.x - A.x) * (B.x - A.x) + (p.z - A.z) * (B.z - A.z)) / len2, 0.0f, 1.0f) : 0.0f;
        const Vec2 on{A.x + (B.x - A.x) * t, A.z + (B.z - A.z) * t};
        const float off = dist(p, on);
        if (off > kJoinWithin) continue;
        joins[count++] = {a, on, off + dist(on, B) + endLeft(a)};
    }
    // Join the ways where that leaves the least to walk, of the points it can walk straight to.
    int join = -1;
    bool tried[NodeCount] = {};
    for (int t = 0; t < kJoinTries && join < 0; ++t) {
        int best = -1;
        for (int i = 0; i < count; ++i)
            if (!tried[i] && (best < 0 || joins[i].cost < joins[best].cost)) best = i;
        if (best < 0) break;
        tried[best] = true;
        if (straightWalk(w, w.form, p, feet, joins[best].on)) join = best;
    }
    if (join < 0) return straight;  // off the ways: the goal is the best it knows
    // Then on along the way, as far as it is still a straight walk from here.
    int a = joins[join].a;
    Vec2 at = joins[join].on, next = end(a);
    float ahead = 0;
    for (;;) {
        const float d = dist(at, next);
        const float step = d < kAimStep ? d : kAimStep;
        const Vec2 q = d > 0 ? Vec2{at.x + (next.x - at.x) * step / d, at.z + (next.z - at.z) * step / d} : next;
        if (ahead + step > kAimAhead || !straightWalk(w, w.form, p, feet, q)) break;
        at = q;
        ahead += step;
        if (dist2(at, next) < 1e-6f) {
            if (a < 0) return straight;  // as far as the spot itself
            a = r.next[a];
            next = a >= 0 ? kNodes[a] : stand;
        }
    }
    const float left = dist(p, at) + dist(at, next) + (a >= 0 ? r.far[a] : 0.0f);
    return {{at.x, groundAt(at) + kAimUp, at.z}, left, false};
}

}  // namespace familiar::ways
