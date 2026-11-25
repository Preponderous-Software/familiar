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

}  // namespace familiar::ways
