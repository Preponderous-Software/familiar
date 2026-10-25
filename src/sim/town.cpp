#include "sim/town.h"

#include <cstdint>
#include <initializer_list>
#include <utility>

namespace familiar::town {

namespace {

float smoothstep(float e0, float e1, float x) {
    const float t = clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3 - 2 * t);
}

// A small deterministic hash for placing trees, 0..1.
float hash01(uint32_t a, uint32_t b) {
    uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    h *= 0x297A2D39u;
    h ^= h >> 15;
    return static_cast<float>(h >> 8) / 16777216.0f;
}

Solid box(Box2 b, float top) {
    Solid s;
    s.box = b;
    s.height = top;
    return s;
}

Solid circle(Vec2 c, float r, float top) {
    Solid s;
    s.shape = Shape::Circle;
    s.centre = c;
    s.radius = r;
    s.height = top;
    return s;
}

// A fence or wall from (x0, z0) to (x1, z1) along one axis, `half` thick each
// side, leaving out the openings [a, b) given along its length.
void wall(std::vector<Solid>& out, float x0, float z0, float x1, float z1, float half, float top,
          std::initializer_list<std::pair<float, float>> openings = {}) {
    const bool alongX = z0 == z1;
    float from = alongX ? x0 : z0;
    const float to = alongX ? x1 : z1;
    auto piece = [&](float a, float b) {
        if (b - a <= 0.01f) return;
        out.push_back(alongX ? box({a, z0 - half, b, z0 + half}, top) : box({x0 - half, a, x0 + half, b}, top));
    };
    for (const auto& o : openings) {
        piece(from, o.first);
        from = o.second;
    }
    piece(from, to);
}

std::vector<Solid> build() {
    std::vector<Solid> out;
    for (int i = 0; i < HouseCount; ++i) {
        const auto h = static_cast<HouseId>(i);
        const House& hd = kHouses[i];
        // The house: walls to the eaves, a gable roof with its ridge along the street.
        Solid s = box(footprint(h), hd.eave);
        s.top = Top::GableX;
        s.ridge = hd.ridge;
        if (h == Market) s.top = Top::Flat;  // the market has a flat roof behind a parapet
        out.push_back(s);
        out.push_back(box(porch(h), kPorchHeight));
        if (hd.fence) {
            // The picket fence along the front, with the front walk's gate open.
            const float fz = hd.north ? -kFenceZ : kFenceZ;
            wall(out, hd.x - hd.width / 2 - 0.5f, fz, hd.x + hd.width / 2 + 0.5f, fz, 0.05f, kPicketHeight,
                 {{hd.x - 0.75f, hd.x + 0.75f}});
        }
    }
    // Nana's shed and rain barrel: the way onto her roof (shed 2.3 from the barrel at 1.0).
    out.push_back(box(kNanaShed, kShedHeight));
    out.push_back(circle(kRainBarrel, 0.4f, kRainBarrelHeight));
    // The fire pit's ring of stones, low enough to step over? No: knee-high, walked around.
    out.push_back(circle(kFirePit, 0.7f, 0.55f));

    // Mr. Hale's yard: board fence on three sides, the gate in the west side.
    const Box2& y = kHaleYard;
    wall(out, y.x0, y.z0, y.x1, y.z0, 0.06f, kHaleFence);                                 // north
    wall(out, y.x1, y.z0, y.x1, y.z1, 0.06f, kHaleFence);                                 // east
    wall(out, y.x0, y.z0, y.x0, y.z1, 0.06f, kHaleFence, {{kHaleGateZ - 0.8f, kHaleGateZ + 0.8f}});  // west
    Solid gate = box({y.x0 - 0.06f, kHaleGateZ - 0.8f, y.x0 + 0.06f, kHaleGateZ + 0.8f}, kHaleFence);
    gate.gate = HaleGate;
    out.push_back(gate);
    // Short lengths from the side fences to the back corners of his house.
    const Box2 hf = footprint(Hale);
    wall(out, y.x0, y.z1, hf.x0 + 0.1f, y.z1, 0.06f, kHaleFence);
    wall(out, hf.x1 - 0.1f, y.z1, y.x1, y.z1, 0.06f, kHaleFence);
    out.push_back(box(kWoodpile, kWoodpileHeight));

    // The churchyard: wall on the west, north and south (with the lychgate),
    // hedge on the east with its gap.
    const Box2& c = kChurchyard;
    wall(out, c.x0, c.z0, c.x1, c.z0, 0.25f, kChurchyardWall);                                  // north
    wall(out, c.x0, c.z0, c.x0, c.z1, 0.25f, kChurchyardWall);                                  // west
    wall(out, c.x0, c.z1, c.x1, c.z1, 0.25f, kChurchyardWall, {{kLychgateX0, kLychgateX1}});   // south
    wall(out, c.x1, c.z0, c.x1, c.z1, 0.4f, 2.6f, {{kHedgeGapZ - kHedgeGap / 2, kHedgeGapZ + kHedgeGap / 2}});  // hedge
    Solid lych = box({kLychgateX0, c.z1 - 0.12f, kLychgateX1, c.z1 + 0.12f}, kChurchyardWall);
    lych.gate = Lychgate;
    out.push_back(lych);
    // The lychgate's posts.
    out.push_back(box({kLychgateX0 - 0.3f, c.z1 - 0.9f, kLychgateX0, c.z1 + 0.9f}, 2.6f));
    out.push_back(box({kLychgateX1, c.z1 - 0.9f, kLychgateX1 + 0.3f, c.z1 + 0.9f}, 2.6f));
    // The church and its tower.
    Solid church = box(kChurch, 6.0f);
    church.top = Top::GableX;
    church.ridge = 10.5f;
    out.push_back(church);
    out.push_back(box(kTower, 17.0f));
    // Headstones in rows: knee-high, so a girl walks round them and a cat hops up.
    for (int row = 0; row < 4; ++row)
        for (int k = 0; k < 7; ++k) {
            const float x = 13.5f + 3.8f * static_cast<float>(k) + (row % 2 ? 1.2f : 0.0f);
            const float z = -44.5f + 3.2f * static_cast<float>(row);
            if (x > kLychgateX0 - 1.5f && x < kLychgateX1 + 1.5f) continue;  // the path from the gate
            out.push_back(box({x - 0.45f, z - 0.12f, x + 0.45f, z + 0.12f}, 0.85f));
        }

    // The bandstand: a raised octagon (a circle here), with a step on the south side.
    out.push_back(circle(kBandstand, kBandstandRadius, kBandstandHeight));
    out.push_back(box({kBandstand.x - 1.0f, kBandstand.z + kBandstandRadius - 0.4f, kBandstand.x + 1.0f,
                       kBandstand.z + kBandstandRadius + 0.6f}, 0.45f));

    // The bridge: a deck over the creek, rails along both sides.
    out.push_back(box(kBridge, kBridgeDeck));
    out.push_back(box({kBridge.x0, kBridge.z0 - 0.08f, kBridge.x1, kBridge.z0 + 0.08f}, kBridgeRail));
    out.push_back(box({kBridge.x0, kBridge.z1 - 0.08f, kBridge.x1, kBridge.z1 + 0.08f}, kBridgeRail));

    // Streetlamp posts.
    for (Vec2 p : kStreetlamps) out.push_back(circle(p, 0.12f, kStreetlampHeight));

    // The standing stones on Lantern Hill.
    for (int i = 0; i < kStoneCount; ++i) {
        const float a = 2 * kPi * static_cast<float>(i) / kStoneCount;
        const Vec2 p{kHill.x + kCircleRadius * detCos(a), kHill.z + kCircleRadius * detSin(a)};
        out.push_back(circle(p, 0.45f, groundHeight(p.x, p.z) + 1.7f));
    }
    // The scarecrow's post.
    out.push_back(circle(kScarecrow, 0.15f, 2.2f));

    for (const Tree& t : trees()) out.push_back(circle(t.at, t.trunk, groundHeight(t.at.x, t.at.z) + t.height));
    return out;
}

// Whether a tree at p would stand in the way of something.
bool keepClear(Vec2 p) {
    if (p.z > -kSidewalkHalf - 9 && p.z < kSidewalkHalf + 9 && p.x < kStreetEast + 14) return true;  // street and front yards
    if (p.x > kChurchyard.x0 - 3 && p.x < kChurchyard.x1 + 3 && p.z > kChurchyard.z0 - 3 && p.z < kChurchyard.z1 + 3) return true;
    const float hx = p.x - kHill.x, hz = p.z - kHill.z;
    if (hx * hx + hz * hz < (kCircleRadius + 5) * (kCircleRadius + 5)) return true;
    const float cx = creekX(p.z);
    if (p.x > cx - kCreekHalfWidth - 2 && p.x < cx + kCreekHalfWidth + 2) return true;
    // The hill path, from the bridge's east end up to the circle.
    const Vec2 a{60.0f, -2.0f}, b{kHill.x - 4.0f, kHill.z + 5.0f};
    const float t = clampf(((p.x - a.x) * (b.x - a.x) + (p.z - a.z) * (b.z - a.z)) /
                               ((b.x - a.x) * (b.x - a.x) + (b.z - a.z) * (b.z - a.z)), 0.0f, 1.0f);
    const float dx = p.x - (a.x + (b.x - a.x) * t), dz = p.z - (a.z + (b.z - a.z) * t);
    if (dx * dx + dz * dz < 9.0f) return true;
    for (Vec2 q : {kFairyRing, kScarecrow, kOldOak}) {
        const float ex = p.x - q.x, ez = p.z - q.z;
        if (ex * ex + ez * ez < 16.0f) return true;
    }
    for (int i = 0; i < HouseCount; ++i) {
        const Box2 f = footprint(static_cast<HouseId>(i));
        if (p.x > f.x0 - 4 && p.x < f.x1 + 4 && p.z > f.z0 - 14 && p.z < f.z1 + 14 && (kHouses[i].north ? p.z < f.z1 : p.z > f.z0))
            return true;
    }
    if (p.x > kHaleYard.x0 - 3 && p.x < kHaleYard.x1 + 3 && p.z > kHaleYard.z0 - 3 && p.z < kHaleYard.z1) return true;
    return false;
}

std::vector<Tree> buildTrees() {
    std::vector<Tree> out;
    // The orchard: rows of apple trees.
    for (int r = 0; r < 4; ++r)
        for (int k = 0; k < 4; ++k) {
            const Vec2 p{kOrchard.x0 + 2.0f + 5.0f * static_cast<float>(k), kOrchard.z0 + 2.0f + 6.0f * static_cast<float>(r)};
            const float ex = p.x - kScarecrow.x, ez = p.z - kScarecrow.z;
            if (ex * ex + ez * ez < 6.0f) continue;
            out.push_back({p, 0.22f, 1.6f, true});
        }
    out.push_back({kOldOak, 0.9f, 6.0f, false});
    out.push_back({kSwapTree, 0.6f, 4.5f, false});
    // Street trees between the houses.
    for (float x : {-75.0f, -57.0f, -39.0f, -21.0f, -3.0f, 27.0f})
        for (float z : {-7.5f, 7.5f})
            if (!(x == -3.0f && z > 0)) out.push_back({{x, z}, 0.35f, 3.6f, false});
    // The woods: north of town and east of the creek, thinning toward town.
    for (int i = 0; i < 900 && out.size() < 230; ++i) {
        const float x = kMin + 6 + (kMax - kMin - 12) * hash01(static_cast<uint32_t>(i), 1);
        const float z = kMin + 6 + (kMax - kMin - 12) * hash01(static_cast<uint32_t>(i), 2);
        const Vec2 p{x, z};
        const bool woods = z < -68 || x > 86 || (x > creekX(z) + 6 && z < -20) || x < -92 || z > 44;
        if (!woods || keepClear(p)) continue;
        if (p.x > kOrchard.x0 - 3 && p.x < kOrchard.x1 + 3 && p.z > kOrchard.z0 - 3 && p.z < kOrchard.z1 + 3) continue;
        bool crowded = false;
        for (const Tree& t : out) {
            const float dx = t.at.x - x, dz = t.at.z - z;
            if (dx * dx + dz * dz < 20.0f) crowded = true;
        }
        if (crowded) continue;
        out.push_back({p, 0.3f + 0.15f * hash01(static_cast<uint32_t>(i), 3), 3.5f + 3.0f * hash01(static_cast<uint32_t>(i), 4), false});
    }
    return out;
}

}  // namespace

// Siding colours are linear RGB: barn red, cream, slate blue, mustard, white,
// sage, grey, the market's brick, and a dark plum for the haunted house.
const std::array<House, HouseCount> kHouses = {{
    {"Nana's house", -66.0f, true, 9.0f, 9.0f, 3.2f, 5.4f, {0.12f, 0.16f, 0.10f}, {0.30f, 0.05f, 0.25f}, true},
    {"The Okafors'", -48.0f, true, 9.0f, 9.0f, 3.2f, 5.6f, {0.62f, 0.55f, 0.40f}, {0.05f, 0.18f, 0.08f}, true},
    {"Mr. Hale's", -30.0f, true, 9.0f, 9.0f, 3.2f, 5.4f, {0.30f, 0.33f, 0.36f}, {0.10f, 0.08f, 0.06f}, true},
    {"The Lindqvists'", -12.0f, true, 9.0f, 9.0f, 3.4f, 5.8f, {0.62f, 0.45f, 0.08f}, {0.35f, 0.04f, 0.03f}, true},
    {"Dr. Pell's", 34.0f, true, 9.0f, 9.0f, 3.2f, 5.4f, {0.70f, 0.70f, 0.66f}, {0.03f, 0.08f, 0.25f}, true},
    {"The Garcias'", -54.0f, false, 9.0f, 9.0f, 3.2f, 5.6f, {0.40f, 0.08f, 0.05f}, {0.65f, 0.55f, 0.35f}, true},
    {"Ms. Bright's", -36.0f, false, 8.0f, 9.0f, 3.2f, 5.2f, {0.20f, 0.30f, 0.45f}, {0.70f, 0.45f, 0.05f}, true},
    {"The Novaks'", -18.0f, false, 9.0f, 9.0f, 3.2f, 5.6f, {0.35f, 0.42f, 0.30f}, {0.06f, 0.06f, 0.06f}, true},
    {"Abbas's Market", 20.0f, false, 12.0f, 10.0f, 4.0f, 4.0f, {0.28f, 0.09f, 0.05f}, {0.25f, 0.30f, 0.20f}, false},
    {"The Whitfields'", 36.0f, false, 9.0f, 9.0f, 3.4f, 6.0f, {0.10f, 0.05f, 0.10f}, {0.30f, 0.30f, 0.25f}, true},
}};

const std::array<Vec2, kStreetlampCount> kStreetlamps = {{
    {-78.0f, -5.0f}, {-58.0f, 5.0f}, {-40.0f, -5.0f}, {-22.0f, 5.0f}, {-2.0f, -5.0f}, {14.0f, 5.0f}, {28.0f, -5.0f},
}};

float creekX(float z) { return 52.0f + 2.5f * detSin(z * 0.035f) + 1.5f * detSin(z * 0.011f + 1.0f) - 1.5f * detSin(1.0f); }

float groundHeight(float x, float z) {
    // Town is flat; beyond it the ground rolls, and rises to hills on the
    // west, the north and the far east.
    float roll = 0.35f * detSin(x * 0.07f) * detCos(z * 0.06f) + 0.25f * detSin((x - z) * 0.03f);
    const float inTown = smoothstep(-96.0f, -88.0f, x) * (1 - smoothstep(44.0f, 47.0f, x)) * smoothstep(-74.0f, -68.0f, z) *
                         (1 - smoothstep(42.0f, 50.0f, z));
    float h = roll * (1 - inTown);
    h += smoothstep(-92.0f, -110.0f, x) * 8.0f;
    h += smoothstep(-76.0f, -110.0f, z) * 6.0f * (1 - inTown);
    h += smoothstep(92.0f, 110.0f, x) * 7.0f;
    // Lantern Hill: a smooth dome.
    const float dx = x - kHill.x, dz = z - kHill.z;
    const float d = std::sqrt(dx * dx + dz * dz) / kHillRadius;
    if (d < 1) {
        const float k = 0.5f + 0.5f * detCos(d * kPi);
        h += kHillHeight * k * k * (3 - 2 * k) * 0.999f;
    }
    // The creek's channel: banks fall to the bed within a metre and a half.
    const float off = std::fabs(x - creekX(z));
    const float bank = 1 - smoothstep(kCreekHalfWidth, kCreekHalfWidth + 1.5f, off);
    return h + (kCreekBedY - h) * bank;
}

float Solid::heightAt(float x, float z) const {
    if (top == Top::Flat || shape == Shape::Circle) return height;
    const float px = clampf(x, box.x0, box.x1), pz = clampf(z, box.z0, box.z1);
    float u;
    if (top == Top::GableX) {
        const float mid = (box.z0 + box.z1) / 2, half = (box.z1 - box.z0) / 2;
        u = 1 - std::fabs(pz - mid) / half;
    } else {
        const float mid = (box.x0 + box.x1) / 2, half = (box.x1 - box.x0) / 2;
        u = 1 - std::fabs(px - mid) / half;
    }
    return height + (ridge - height) * u;
}

bool Solid::overlaps(float x, float z, float r) const {
    if (shape == Shape::Circle) {
        const float dx = x - centre.x, dz = z - centre.z, rr = r + radius;
        return dx * dx + dz * dz < rr * rr;
    }
    const float cx = clampf(x, box.x0, box.x1), cz = clampf(z, box.z0, box.z1);
    const float dx = x - cx, dz = z - cz;
    return dx * dx + dz * dz < r * r;
}

Box2 footprint(HouseId h) {
    const House& d = kHouses[h];
    const float x0 = d.x - d.width / 2, x1 = d.x + d.width / 2;
    return d.north ? Box2{x0, -kHouseFront - d.depth, x1, -kHouseFront} : Box2{x0, kHouseFront, x1, kHouseFront + d.depth};
}

Box2 porch(HouseId h) {
    const House& d = kHouses[h];
    const float half = h == Market ? 3.0f : 2.6f;
    return d.north ? Box2{d.x - half, -kHouseFront, d.x + half, -kHouseFront + kPorchDepth}
                   : Box2{d.x - half, kHouseFront - kPorchDepth, d.x + half, kHouseFront};
}

Vec2 doorAt(HouseId h) { return {kHouses[h].x, kHouses[h].north ? -kHouseFront : kHouseFront}; }

Vec2 doorstep(HouseId h) {
    const Vec2 d = doorAt(h);
    return {d.x, kHouses[h].north ? d.z + 1.2f : d.z - 1.2f};
}

Vec3 porchLight(HouseId h) {
    const Vec2 d = doorAt(h);
    return {d.x + 1.0f, 2.3f, kHouses[h].north ? d.z + 0.15f : d.z - 0.15f};
}

const std::vector<Tree>& trees() {
    static const std::vector<Tree> t = buildTrees();
    return t;
}

const std::vector<Solid>& solids() {
    static const std::vector<Solid> s = build();
    return s;
}

}  // namespace familiar::town
