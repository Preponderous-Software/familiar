#include "bridge/geometry.h"

#include <cmath>

namespace familiar::bridge {

namespace T = familiar::town;

namespace {

Vec3 cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }

Vec3 normalize(Vec3 v) {
    const float l = length(v);
    return l > 0 ? v * (1.0f / l) : Vec3{0, 1, 0};
}

constexpr Rgb kTrim{0.75f, 0.73f, 0.68f};
constexpr Rgb kRoof{0.09f, 0.08f, 0.08f};
constexpr Rgb kPorchWood{0.25f, 0.17f, 0.10f};
constexpr Rgb kPicket{0.72f, 0.70f, 0.66f};
constexpr Rgb kBoard{0.24f, 0.17f, 0.11f};
constexpr Rgb kStone{0.30f, 0.29f, 0.27f};
constexpr Rgb kStoneDark{0.18f, 0.18f, 0.18f};
constexpr Rgb kHedge{0.03f, 0.09f, 0.03f};
constexpr Rgb kBark{0.12f, 0.08f, 0.06f};
constexpr Rgb kLeafRed{0.40f, 0.07f, 0.02f};
constexpr Rgb kLeafOrange{0.55f, 0.20f, 0.02f};
constexpr Rgb kLeafGold{0.55f, 0.36f, 0.04f};
constexpr Rgb kPine{0.02f, 0.07f, 0.03f};
constexpr Rgb kPumpkin{0.70f, 0.22f, 0.02f};
constexpr Rgb kStem{0.10f, 0.12f, 0.03f};
constexpr Rgb kWater{0.03f, 0.06f, 0.08f};
constexpr Rgb kLampPost{0.05f, 0.05f, 0.05f};
constexpr Rgb kWindowGlow{1.0f, 0.72f, 0.32f};
constexpr Rgb kFaceGlow{1.0f, 0.55f, 0.08f};
constexpr Rgb kHay{0.45f, 0.35f, 0.12f};

}  // namespace

void MeshData::triangle(Vec3 a, Vec3 b, Vec3 c, Rgb color) {
    const Vec3 n = normalize(cross(b - a, c - a));
    const auto base = static_cast<uint32_t>(vertexCount());
    for (Vec3 p : {a, b, c}) {
        positions.insert(positions.end(), {p.x, p.y, p.z});
        normals.insert(normals.end(), {n.x, n.y, n.z});
        colors.insert(colors.end(), {color.r, color.g, color.b});
    }
    indices.insert(indices.end(), {base, base + 1, base + 2});
}

void MeshData::quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Rgb color) {
    const Vec3 n = normalize(cross(b - a, c - a));
    const auto base = static_cast<uint32_t>(vertexCount());
    for (Vec3 p : {a, b, c, d}) {
        positions.insert(positions.end(), {p.x, p.y, p.z});
        normals.insert(normals.end(), {n.x, n.y, n.z});
        colors.insert(colors.end(), {color.r, color.g, color.b});
    }
    indices.insert(indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

void MeshData::box(Vec3 lo, Vec3 hi, Rgb color) {
    orientedBox(lo, {hi.x - lo.x, 0, 0}, {0, hi.y - lo.y, 0}, {0, 0, hi.z - lo.z}, color);
}

void MeshData::orientedBox(Vec3 o, Vec3 ex, Vec3 ey, Vec3 ez, Rgb color) {
    auto p = [&](int a, int b, int c) {
        return o + ex * static_cast<float>(a) + ey * static_cast<float>(b) + ez * static_cast<float>(c);
    };
    quad(p(0, 0, 1), p(1, 0, 1), p(1, 1, 1), p(0, 1, 1), color);  // +ez
    quad(p(1, 0, 0), p(0, 0, 0), p(0, 1, 0), p(1, 1, 0), color);  // -ez
    quad(p(1, 0, 1), p(1, 0, 0), p(1, 1, 0), p(1, 1, 1), color);  // +ex
    quad(p(0, 0, 0), p(0, 0, 1), p(0, 1, 1), p(0, 1, 0), color);  // -ex
    quad(p(0, 1, 1), p(1, 1, 1), p(1, 1, 0), p(0, 1, 0), color);  // +ey
    quad(p(0, 0, 0), p(1, 0, 0), p(1, 0, 1), p(0, 0, 1), color);  // -ey
}

void MeshData::cylinder(Vec3 base, float radiusBottom, float radiusTop, float height, int segments, Rgb color) {
    const float twoPi = 2.0f * kPi;
    for (int i = 0; i < segments; ++i) {
        const float a0 = twoPi * static_cast<float>(i) / static_cast<float>(segments);
        const float a1 = twoPi * static_cast<float>(i + 1) / static_cast<float>(segments);
        const float c0 = std::cos(a0), s0 = std::sin(a0), c1 = std::cos(a1), s1 = std::sin(a1);
        const Vec3 b0{base.x + radiusBottom * c0, base.y, base.z + radiusBottom * s0};
        const Vec3 b1{base.x + radiusBottom * c1, base.y, base.z + radiusBottom * s1};
        const Vec3 t0{base.x + radiusTop * c0, base.y + height, base.z + radiusTop * s0};
        const Vec3 t1{base.x + radiusTop * c1, base.y + height, base.z + radiusTop * s1};
        // Angles run from +x toward +z, which is clockwise seen from above,
        // so the outward face is b1, b0, t0, t1.
        quad(b1, b0, t0, t1, color);
        triangle({base.x, base.y + height, base.z}, t1, t0, color);
    }
}

void MeshData::blob(Vec3 c, float radius, float squash, Rgb color) {
    const int rings = 5, segments = 9;
    auto at = [&](int ring, int seg) {
        const float phi = kPi * static_cast<float>(ring) / static_cast<float>(rings);           // 0 top .. pi bottom
        const float theta = 2.0f * kPi * static_cast<float>(seg) / static_cast<float>(segments);
        return Vec3{c.x + radius * std::sin(phi) * std::cos(theta), c.y + radius * squash * std::cos(phi),
                    c.z + radius * std::sin(phi) * std::sin(theta)};
    };
    for (int r = 0; r < rings; ++r)
        for (int s = 0; s < segments; ++s) {
            const Vec3 a = at(r, s), b = at(r, s + 1), d = at(r + 1, s), e = at(r + 1, s + 1);
            // Seen from outside, (r, s) -> (r+1, s) -> (r+1, s+1) runs counter-clockwise.
            if (r > 0) triangle(a, b, d, color);
            if (r < rings - 1) triangle(b, e, d, color);
        }
}


namespace {

Rgb rgb(const float c[3]) { return {c[0], c[1], c[2]}; }

// Plain walls, a gable roof with eaves, a door, trim.
void house(MeshData& m, T::HouseId id) {
    const T::House& h = T::kHouses[id];
    const Box2 f = T::footprint(id);
    const Rgb wall = rgb(h.wall);
    m.box({f.x0, -0.3f, f.z0}, {f.x1, h.eave, f.z1}, wall);
    const float o = 0.4f;
    if (id == T::Market) {
        // A flat roof behind a parapet, and a striped awning over the storefront.
        m.box({f.x0 - 0.1f, h.eave, f.z0 - 0.1f}, {f.x1 + 0.1f, h.eave + 0.6f, f.z1 + 0.1f}, wall);
        for (int i = 0; i < 8; ++i) {
            const float x0 = f.x0 + 1.0f + 1.25f * static_cast<float>(i);
            m.orientedBox({x0, 2.6f, f.z0 - 1.6f}, {1.25f, 0, 0}, {0, 0.6f, 1.6f}, {0, 0.06f, 0},
                          i % 2 ? Rgb{0.55f, 0.05f, 0.03f} : Rgb{0.70f, 0.66f, 0.55f});
        }
    } else {
        const float zm = (f.z0 + f.z1) / 2, ridge = h.ridge;
        const float ex0 = f.x0 - o, ex1 = f.x1 + o, ez0 = f.z0 - o, ez1 = f.z1 + o, eave = h.eave - 0.15f;
        m.quad({ex1, eave, ez0}, {ex0, eave, ez0}, {ex0, ridge, zm}, {ex1, ridge, zm}, kRoof);
        m.quad({ex0, eave, ez1}, {ex1, eave, ez1}, {ex1, ridge, zm}, {ex0, ridge, zm}, kRoof);
        m.triangle({f.x0, h.eave, f.z0}, {f.x0, h.eave, f.z1}, {f.x0, ridge - 0.05f, zm}, wall);
        m.triangle({f.x1, h.eave, f.z1}, {f.x1, h.eave, f.z0}, {f.x1, ridge - 0.05f, zm}, wall);
        // A chimney.
        m.box({f.x1 - 1.6f, h.eave, zm - 0.5f}, {f.x1 - 0.8f, ridge + 0.8f, zm + 0.4f}, Rgb{0.25f, 0.08f, 0.05f});
    }
    // The door, just proud of the front wall.
    const Vec2 d = T::doorAt(id);
    const float s = h.north ? 1.0f : -1.0f;  // +z out of a north house's front
    const float dz = d.z + s * 0.03f, dw = 0.5f, dh = 2.1f;
    const Rgb door = rgb(h.door);
    if (h.north) m.quad({d.x - dw, T::kPorchHeight, dz}, {d.x + dw, T::kPorchHeight, dz}, {d.x + dw, dh, dz}, {d.x - dw, dh, dz}, door);
    else m.quad({d.x + dw, T::kPorchHeight, dz}, {d.x - dw, T::kPorchHeight, dz}, {d.x - dw, dh, dz}, {d.x + dw, dh, dz}, door);
    // The porch: a deck, two posts and a roof.
    const Box2 p = T::porch(id);
    m.box({p.x0, -0.2f, p.z0}, {p.x1, T::kPorchHeight, p.z1}, kPorchWood);
    const float pz = h.north ? p.z1 - 0.15f : p.z0 + 0.15f;
    for (float x : {p.x0 + 0.15f, p.x1 - 0.15f}) m.box({x - 0.08f, T::kPorchHeight, pz - 0.08f}, {x + 0.08f, 2.6f, pz + 0.08f}, kTrim);
    m.box({p.x0 - 0.1f, 2.6f, p.z0 - 0.1f}, {p.x1 + 0.1f, 2.75f, p.z1 + 0.1f}, kRoof);
    // A front step.
    const float sz = h.north ? p.z1 : p.z0 - 0.5f;
    m.box({d.x - 0.9f, -0.2f, sz}, {d.x + 0.9f, 0.22f, sz + 0.5f}, kPorchWood);
    // The porch light's fixture, beside the door.
    const Vec3 l = T::porchLight(id);
    m.box({l.x - 0.09f, l.y - 0.14f, l.z - 0.09f}, {l.x + 0.09f, l.y + 0.14f, l.z + 0.09f}, kLampPost);
}

void fenceRun(MeshData& m, float x0, float x1, float z, float gate0, float gate1) {
    // Pickets every 0.25 m and two rails, leaving the gate open.
    for (float x = x0; x <= x1; x += 0.25f) {
        if (x > gate0 && x < gate1) continue;
        m.box({x - 0.04f, 0, z - 0.02f}, {x + 0.04f, T::kPicketHeight, z + 0.02f}, kPicket);
    }
    for (float y : {0.3f, 0.7f}) {
        m.box({x0, y, z - 0.04f}, {gate0, y + 0.06f, z + 0.04f}, kPicket);
        m.box({gate1, y, z - 0.04f}, {x1, y + 0.06f, z + 0.04f}, kPicket);
    }
}

void tree(MeshData& m, const T::Tree& t, int i) {
    const float g = T::groundHeight(t.at.x, t.at.z);
    if (t.apple) {
        m.cylinder({t.at.x, g - 0.2f, t.at.z}, t.trunk, t.trunk * 0.7f, t.height + 0.2f, 6, kBark);
        m.blob({t.at.x, g + t.height + 0.9f, t.at.z}, 1.8f, 0.7f, Rgb{0.10f, 0.14f, 0.03f});
        for (int k = 0; k < 5; ++k) {  // apples
            const float a = 1.3f * static_cast<float>(k) + static_cast<float>(i);
            m.blob({t.at.x + 1.3f * std::cos(a), g + t.height + 0.5f + 0.2f * static_cast<float>(k % 2), t.at.z + 1.3f * std::sin(a)},
                   0.09f, 1.0f, Rgb{0.45f, 0.03f, 0.02f});
        }
        return;
    }
    const bool pine = (i % 3) == 0 && (t.at.z < -66 || t.at.x > 86);
    if (pine) {
        m.cylinder({t.at.x, g - 0.2f, t.at.z}, t.trunk, t.trunk * 0.6f, 1.6f, 6, kBark);
        const float h = t.height + 3.0f;
        m.cylinder({t.at.x, g + 1.2f, t.at.z}, 2.2f, 0.05f, h, 8, kPine);
        return;
    }
    m.cylinder({t.at.x, g - 0.2f, t.at.z}, t.trunk, t.trunk * 0.6f, t.height + 0.2f, 7, kBark);
    const Rgb leaf = i % 3 == 0 ? kLeafRed : i % 3 == 1 ? kLeafOrange : kLeafGold;
    const float r = 2.2f + t.trunk * 2.5f;
    m.blob({t.at.x, g + t.height + r * 0.5f, t.at.z}, r, 0.8f, leaf);
    m.blob({t.at.x + r * 0.5f, g + t.height + r * 0.2f, t.at.z - r * 0.3f}, r * 0.6f, 0.8f, leaf);
}

void churchyard(MeshData& m) {
    const Box2& c = T::kChurchyard;
    const float h = T::kChurchyardWall;
    m.box({c.x0 - 0.25f, -0.2f, c.z0 - 0.25f}, {c.x1, h, c.z0 + 0.25f}, kStone);
    m.box({c.x0 - 0.25f, -0.2f, c.z0}, {c.x0 + 0.25f, h, c.z1 + 0.25f}, kStone);
    m.box({c.x0, -0.2f, c.z1 - 0.25f}, {T::kLychgateX0, h, c.z1 + 0.25f}, kStone);
    m.box({T::kLychgateX1, -0.2f, c.z1 - 0.25f}, {c.x1, h, c.z1 + 0.25f}, kStone);
    // The hedge, with its gap.
    const float g0 = T::kHedgeGapZ - T::kHedgeGap / 2, g1 = T::kHedgeGapZ + T::kHedgeGap / 2;
    m.box({c.x1 - 0.4f, -0.2f, c.z0}, {c.x1 + 0.4f, 2.6f, g0}, kHedge);
    m.box({c.x1 - 0.4f, -0.2f, g1}, {c.x1 + 0.4f, 2.6f, c.z1}, kHedge);
    // The lychgate: posts and a little roof.
    for (float x : {T::kLychgateX0 - 0.3f, T::kLychgateX1})
        m.box({x, -0.2f, c.z1 - 0.9f}, {x + 0.3f, 2.6f, c.z1 + 0.9f}, kBoard);
    const float zm = c.z1, x0 = T::kLychgateX0 - 0.6f, x1 = T::kLychgateX1 + 0.6f;
    m.quad({x1, 2.5f, zm - 1.2f}, {x0, 2.5f, zm - 1.2f}, {x0, 3.6f, zm}, {x1, 3.6f, zm}, kRoof);
    m.quad({x0, 2.5f, zm + 1.2f}, {x1, 2.5f, zm + 1.2f}, {x1, 3.6f, zm}, {x0, 3.6f, zm}, kRoof);
    // The church: stone walls, a steep roof, a square tower with a spire.
    const Box2& k = T::kChurch;
    m.box({k.x0, -0.2f, k.z0}, {k.x1, 6.0f, k.z1}, kStone);
    const float km = (k.z0 + k.z1) / 2;
    m.quad({k.x1 + 0.3f, 5.8f, k.z0 - 0.3f}, {k.x0 - 0.3f, 5.8f, k.z0 - 0.3f}, {k.x0 - 0.3f, 10.5f, km}, {k.x1 + 0.3f, 10.5f, km}, kStoneDark);
    m.quad({k.x0 - 0.3f, 5.8f, k.z1 + 0.3f}, {k.x1 + 0.3f, 5.8f, k.z1 + 0.3f}, {k.x1 + 0.3f, 10.5f, km}, {k.x0 - 0.3f, 10.5f, km}, kStoneDark);
    m.triangle({k.x0, 6.0f, k.z0}, {k.x0, 6.0f, k.z1}, {k.x0, 10.4f, km}, kStone);
    m.triangle({k.x1, 6.0f, k.z1}, {k.x1, 6.0f, k.z0}, {k.x1, 10.4f, km}, kStone);
    const Box2& t = T::kTower;
    m.box({t.x0, -0.2f, t.z0}, {t.x1, 17.0f, t.z1}, kStone);
    const Vec3 tc{(t.x0 + t.x1) / 2, 17.0f, (t.z0 + t.z1) / 2};
    m.cylinder(tc, 3.0f, 0.05f, 7.0f, 4, kStoneDark);
    // Headstones (the same rows as the solids in town.cpp).
    for (int row = 0; row < 4; ++row)
        for (int i = 0; i < 7; ++i) {
            const float x = 13.5f + 3.8f * static_cast<float>(i) + (row % 2 ? 1.2f : 0.0f);
            const float z = -44.5f + 3.2f * static_cast<float>(row);
            if (x > T::kLychgateX0 - 1.5f && x < T::kLychgateX1 + 1.5f) continue;
            m.box({x - 0.45f, -0.1f, z - 0.12f}, {x + 0.45f, 0.7f, z + 0.12f}, (row + i) % 3 ? kStone : kStoneDark);
            m.cylinder({x, 0.7f, z - 0.12f}, 0.45f, 0.45f, 0.0f, 6, kStone);
            m.box({x - 0.32f, 0.7f, z - 0.12f}, {x + 0.32f, 0.85f, z + 0.12f}, (row + i) % 3 ? kStone : kStoneDark);
        }
}

}  // namespace

std::vector<Vec3> pumpkins(T::HouseId h) {
    if (h == T::Hale) return {};
    const T::House& d = T::kHouses[h];
    const Box2 p = T::porch(h);
    const float z = d.north ? p.z1 - 0.35f : p.z0 + 0.35f;
    return {{d.x - 1.5f, T::kPorchHeight, z}, {d.x + 1.6f, T::kPorchHeight, z}};
}

TerrainData buildTerrain() {
    TerrainData t;
    t.samples = T::kSamples;
    t.origin[0] = T::kMin;
    t.origin[1] = 0;
    t.origin[2] = T::kMin;
    t.cell = T::kCell;
    const size_t n = static_cast<size_t>(t.samples) * static_cast<size_t>(t.samples);
    t.heights.resize(n);
    t.colors.resize(n * 3);
    for (int iz = 0; iz < t.samples; ++iz)
        for (int ix = 0; ix < t.samples; ++ix) {
            const float x = T::kMin + T::kCell * static_cast<float>(ix), z = T::kMin + T::kCell * static_cast<float>(iz);
            const float h = T::groundHeight(x, z);
            const size_t i = static_cast<size_t>(iz) * static_cast<size_t>(t.samples) + static_cast<size_t>(ix);
            t.heights[i] = h;
            // Late-October grass, leaf litter toward the woods, mud at the creek.
            const float n1 = 0.5f + 0.5f * std::sin(x * 0.37f + std::cos(z * 0.23f) * 3.0f);
            Rgb c{0.10f + 0.06f * n1, 0.13f + 0.03f * n1, 0.04f};
            const bool woods = z < -66 || x > 86 || (x > T::creekX(z) + 6 && z < -20);
            if (woods) c = {0.16f + 0.08f * n1, 0.08f + 0.03f * n1, 0.03f};
            const float mud = clampf((0.0f - h) / 1.0f, 0.0f, 1.0f);
            c = {c.r + (0.10f - c.r) * mud, c.g + (0.08f - c.g) * mud, c.b + (0.06f - c.b) * mud};
            if (x > T::kStreetWest && x < T::kStreetEast) {
                const float az = std::fabs(z);
                if (az < T::kRoadHalf) c = {0.06f, 0.06f, 0.065f};                   // asphalt
                else if (az < T::kSidewalkHalf) c = {0.28f, 0.27f, 0.25f};          // sidewalk
            }
            // Front walks from the sidewalk to each porch, and the church lane.
            for (int k = 0; k < T::HouseCount; ++k) {
                const T::House& hd = T::kHouses[k];
                const float az = hd.north ? -z : z;
                if (std::fabs(x - hd.x) < 0.7f && az >= T::kSidewalkHalf && az < T::kHouseFront) c = {0.28f, 0.27f, 0.25f};
            }
            if (x > T::kLychgateX0 && x < T::kLychgateX1 && z < -T::kSidewalkHalf && z > -38.0f) c = {0.22f, 0.20f, 0.17f};
            // The hill path: from the bridge's east end up to the circle.
            {
                const float ax = 60.0f, az = -2.0f, bx = T::kHill.x - 4.0f, bz = T::kHill.z + 5.0f;
                const float tt = clampf(((x - ax) * (bx - ax) + (z - az) * (bz - az)) / ((bx - ax) * (bx - ax) + (bz - az) * (bz - az)), 0.0f, 1.0f);
                const float dx = x - (ax + (bx - ax) * tt), dz = z - (az + (bz - az) * tt);
                if (dx * dx + dz * dz < 1.0f) c = {0.20f, 0.15f, 0.09f};
            }
            t.colors[i * 3 + 0] = c.r;
            t.colors[i * 3 + 1] = c.g;
            t.colors[i * 3 + 2] = c.b;
        }
    return t;
}

MeshData buildTown() {
    MeshData m;
    for (int i = 0; i < T::HouseCount; ++i) {
        const auto id = static_cast<T::HouseId>(i);
        house(m, id);
        const T::House& h = T::kHouses[i];
        if (h.fence) {
            const float fz = h.north ? -T::kFenceZ : T::kFenceZ;
            fenceRun(m, h.x - h.width / 2 - 0.5f, h.x + h.width / 2 + 0.5f, fz, h.x - 0.75f, h.x + 0.75f);
        }
        for (Vec3 p : pumpkins(id)) {
            m.blob({p.x, p.y + 0.2f, p.z}, 0.26f, 0.8f, kPumpkin);
            m.box({p.x - 0.03f, p.y + 0.38f, p.z - 0.03f}, {p.x + 0.03f, p.y + 0.5f, p.z + 0.03f}, kStem);
        }
    }
    // Nana's shed, rain barrel, fire pit with its iron pot, and herb bed.
    const Box2& s = T::kNanaShed;
    m.box({s.x0, -0.2f, s.z0}, {s.x1, T::kShedHeight, s.z1}, kBoard);
    m.box({s.x0 - 0.1f, T::kShedHeight, s.z0 - 0.1f}, {s.x1 + 0.1f, T::kShedHeight + 0.08f, s.z1 + 0.1f}, kRoof);
    m.cylinder({T::kRainBarrel.x, -0.1f, T::kRainBarrel.z}, 0.4f, 0.4f, T::kRainBarrelHeight + 0.1f, 10, Rgb{0.14f, 0.18f, 0.20f});
    m.cylinder({T::kFirePit.x, -0.1f, T::kFirePit.z}, 0.7f, 0.65f, 0.4f, 10, kStone);
    m.blob({T::kFirePit.x, 0.75f, T::kFirePit.z}, 0.42f, 0.75f, Rgb{0.04f, 0.04f, 0.04f});
    for (float dx : {-0.45f, 0.45f}) m.box({T::kFirePit.x + dx - 0.03f, 0, T::kFirePit.z - 0.03f}, {T::kFirePit.x + dx + 0.03f, 1.3f, T::kFirePit.z + 0.03f}, kLampPost);
    m.box({T::kFirePit.x - 0.5f, 1.25f, T::kFirePit.z - 0.03f}, {T::kFirePit.x + 0.5f, 1.31f, T::kFirePit.z + 0.03f}, kLampPost);
    m.box({T::kHerbBed.x - 1.5f, -0.1f, T::kHerbBed.z - 0.8f}, {T::kHerbBed.x + 1.5f, 0.25f, T::kHerbBed.z + 0.8f}, kBoard);
    for (int k = 0; k < 6; ++k)
        m.blob({T::kHerbBed.x - 1.1f + 0.45f * static_cast<float>(k), 0.35f, T::kHerbBed.z + (k % 2 ? 0.3f : -0.3f)}, 0.22f, 0.8f, Rgb{0.08f, 0.14f, 0.07f});

    // Mr. Hale's yard: board fence, posts, and the woodpile outside it.
    const Box2& y = T::kHaleYard;
    const float hf = T::kHaleFence;
    const Box2 hale = T::footprint(T::Hale);
    m.box({y.x0, -0.1f, y.z0 - 0.06f}, {y.x1, hf, y.z0 + 0.06f}, kBoard);
    m.box({y.x1 - 0.06f, -0.1f, y.z0}, {y.x1 + 0.06f, hf, y.z1}, kBoard);
    m.box({y.x0 - 0.06f, -0.1f, y.z0}, {y.x0 + 0.06f, hf, T::kHaleGateZ - 0.8f}, kBoard);
    m.box({y.x0 - 0.06f, -0.1f, T::kHaleGateZ + 0.8f}, {y.x0 + 0.06f, hf, y.z1}, kBoard);
    m.box({y.x0, -0.1f, y.z1 - 0.06f}, {hale.x0 + 0.1f, hf, y.z1 + 0.06f}, kBoard);
    m.box({hale.x1 - 0.1f, -0.1f, y.z1 - 0.06f}, {y.x1, hf, y.z1 + 0.06f}, kBoard);
    const Box2& wp = T::kWoodpile;
    for (int k = 0; k < 4; ++k) {
        const float zz = wp.z0 + 0.1f + 0.6f * static_cast<float>(k);
        m.box({wp.x0, 0, zz}, {wp.x1, T::kWoodpileHeight, zz + 0.55f}, k % 2 ? Rgb{0.30f, 0.20f, 0.11f} : Rgb{0.24f, 0.16f, 0.09f});
    }

    churchyard(m);

    // The bandstand: a raised octagon, posts, a pointed roof.
    const Vec2 b = T::kBandstand;
    m.cylinder({b.x, -0.2f, b.z}, T::kBandstandRadius, T::kBandstandRadius, T::kBandstandHeight + 0.2f, 8, kTrim);
    for (int k = 0; k < 8; ++k) {
        const float a = 2 * kPi * (static_cast<float>(k) + 0.5f) / 8;
        const float px = b.x + (T::kBandstandRadius - 0.3f) * std::cos(a), pz = b.z + (T::kBandstandRadius - 0.3f) * std::sin(a);
        m.box({px - 0.1f, T::kBandstandHeight, pz - 0.1f}, {px + 0.1f, 3.6f, pz + 0.1f}, kTrim);
    }
    m.cylinder({b.x, 3.6f, b.z}, T::kBandstandRadius + 0.4f, 0.1f, 2.0f, 8, Rgb{0.30f, 0.06f, 0.05f});
    m.box({b.x - 1.0f, -0.1f, b.z + T::kBandstandRadius - 0.4f}, {b.x + 1.0f, 0.45f, b.z + T::kBandstandRadius + 0.6f}, kTrim);

    // The bridge: deck, rails and posts.
    const Box2& br = T::kBridge;
    m.box({br.x0, -0.4f, br.z0}, {br.x1, T::kBridgeDeck, br.z1}, kBoard);
    for (float z : {br.z0, br.z1}) {
        m.box({br.x0, T::kBridgeRail - 0.1f, z - 0.07f}, {br.x1, T::kBridgeRail, z + 0.07f}, kBoard);
        for (float x = br.x0; x <= br.x1 + 0.01f; x += 1.625f)
            m.box({x - 0.07f, -0.4f, z - 0.07f}, {x + 0.07f, T::kBridgeRail, z + 0.07f}, kBoard);
    }

    // Streetlamps.
    for (Vec2 p : T::kStreetlamps) {
        m.cylinder({p.x, -0.1f, p.z}, 0.12f, 0.08f, T::kStreetlampHeight, 6, kLampPost);
        m.box({p.x - 0.25f, T::kStreetlampHeight - 0.05f, p.z - 0.25f}, {p.x + 0.25f, T::kStreetlampHeight + 0.05f, p.z + 0.25f}, kLampPost);
    }

    // Lantern Hill's ring of stones.
    for (int i = 0; i < T::kStoneCount; ++i) {
        const float a = 2 * kPi * static_cast<float>(i) / T::kStoneCount;
        const float px = T::kHill.x + T::kCircleRadius * detCos(a), pz = T::kHill.z + T::kCircleRadius * detSin(a);
        const float g = T::groundHeight(px, pz);
        m.cylinder({px, g - 0.3f, pz}, 0.45f, 0.32f, 2.0f + 0.3f * static_cast<float>(i % 3), 6, kStone);
    }

    // The scarecrow: a post, a crossbar, a coat and a pumpkin head.
    const Vec2 sc = T::kScarecrow;
    const float sg = T::groundHeight(sc.x, sc.z);
    m.box({sc.x - 0.07f, sg - 0.2f, sc.z - 0.07f}, {sc.x + 0.07f, sg + 1.9f, sc.z + 0.07f}, kBark);
    m.box({sc.x - 0.9f, sg + 1.45f, sc.z - 0.05f}, {sc.x + 0.9f, sg + 1.55f, sc.z + 0.05f}, kBark);
    m.box({sc.x - 0.35f, sg + 0.8f, sc.z - 0.15f}, {sc.x + 0.35f, sg + 1.6f, sc.z + 0.15f}, Rgb{0.18f, 0.12f, 0.20f});
    m.blob({sc.x, sg + 1.9f, sc.z}, 0.28f, 0.8f, kPumpkin);
    m.cylinder({sc.x, sg + 2.05f, sc.z}, 0.4f, 0.02f, 0.45f, 8, kHay);

    for (size_t i = 0; i < T::trees().size(); ++i) tree(m, T::trees()[i], static_cast<int>(i));
    return m;
}

MeshData buildGlow() {
    MeshData m;
    // Lit windows: two either side of the door downstairs, two up in the gable.
    for (int i = 0; i < T::HouseCount; ++i) {
        const auto id = static_cast<T::HouseId>(i);
        const T::House& h = T::kHouses[i];
        const Vec2 d = T::doorAt(id);
        const float zf = h.north ? d.z + 0.04f : d.z - 0.04f;
        Rgb glow = kWindowGlow;
        if (id == T::Hale) glow = {0.10f, 0.14f, 0.25f};  // one lamp, far inside
        if (id == T::Whitfield) glow = {0.60f, 0.10f, 0.45f};
        auto window = [&](float cx, float y0, float w, float hgt) {
            if (h.north) m.quad({cx - w, y0, zf}, {cx + w, y0, zf}, {cx + w, y0 + hgt, zf}, {cx - w, y0 + hgt, zf}, glow);
            else m.quad({cx + w, y0, zf}, {cx - w, y0, zf}, {cx - w, y0 + hgt, zf}, {cx + w, y0 + hgt, zf}, glow);
        };
        if (id == T::Market) {
            window(d.x - 3.2f, 0.6f, 1.8f, 1.7f);
            window(d.x + 3.2f, 0.6f, 1.8f, 1.7f);
            continue;
        }
        window(d.x - 2.6f, 1.0f, 0.5f, 1.2f);
        window(d.x + 2.6f, 1.0f, 0.5f, 1.2f);
        window(d.x - 1.2f, h.eave + 0.3f, 0.4f, 0.8f);
        window(d.x + 1.2f, h.eave + 0.3f, 0.4f, 0.8f);
        // Jack-o'-lantern faces: two eyes and a mouth on the street side.
        const float out = h.north ? 1.0f : -1.0f;
        for (Vec3 p : pumpkins(id)) {
            const float fz = p.z + out * 0.255f, cy = p.y + 0.22f;
            auto tri = [&](Vec3 a, Vec3 b, Vec3 c) {
                if (h.north) m.triangle(a, b, c, kFaceGlow);
                else m.triangle(a, c, b, kFaceGlow);
            };
            tri({p.x - 0.13f, cy + 0.02f, fz}, {p.x - 0.03f, cy + 0.02f, fz}, {p.x - 0.08f, cy + 0.11f, fz});
            tri({p.x + 0.03f, cy + 0.02f, fz}, {p.x + 0.13f, cy + 0.02f, fz}, {p.x + 0.08f, cy + 0.11f, fz});
            tri({p.x - 0.14f, cy - 0.08f, fz}, {p.x + 0.14f, cy - 0.08f, fz}, {p.x, cy - 0.02f, fz});
        }
    }
    // Streetlamp heads.
    for (Vec2 p : T::kStreetlamps)
        m.box({p.x - 0.18f, T::kStreetlampHeight - 0.35f, p.z - 0.18f}, {p.x + 0.18f, T::kStreetlampHeight - 0.05f, p.z + 0.18f},
              Rgb{1.0f, 0.85f, 0.55f});
    // The church clock, on the tower's south face.
    const Box2& t = T::kTower;
    const float cx = (t.x0 + t.x1) / 2, cz = t.z1 + 0.03f;
    m.quad({cx - 0.8f, 12.5f, cz}, {cx + 0.8f, 12.5f, cz}, {cx + 0.8f, 14.1f, cz}, {cx - 0.8f, 14.1f, cz}, Rgb{0.85f, 0.80f, 0.60f});
    return m;
}

MeshData buildWater() {
    MeshData m;
    for (float z = T::kMin; z < T::kMax; z += 4.0f) {
        const float a0 = T::creekX(z), a1 = T::creekX(z + 4.0f), w = T::kCreekHalfWidth + 0.8f;
        m.quad({a0 - w, T::kWaterY, z}, {a1 - w, T::kWaterY, z + 4.0f}, {a1 + w, T::kWaterY, z + 4.0f}, {a0 + w, T::kWaterY, z}, kWater);
    }
    return m;
}

MeshData buildGate(T::Gate g) {
    MeshData m;
    if (g == T::HaleGate) {
        const float x = T::kHaleYard.x0;
        m.box({x - 0.05f, -0.1f, T::kHaleGateZ - 0.8f}, {x + 0.05f, T::kHaleFence - 0.05f, T::kHaleGateZ + 0.8f}, Rgb{0.30f, 0.21f, 0.12f});
    } else {
        const float z = T::kChurchyard.z1;
        m.box({T::kLychgateX0, 0, z - 0.06f}, {T::kLychgateX1, 1.3f, z + 0.06f}, Rgb{0.20f, 0.13f, 0.08f});
    }
    return m;
}

MeshData buildFigure(float height, Rgb clothes, Rgb top, Rgb skin) {
    MeshData m;
    const float k = height / 1.7f;
    m.cylinder({0, 0, 0}, 0.17f * k, 0.2f * k, 0.85f * k, 10, clothes);
    m.cylinder({0, 0.85f * k, 0}, 0.21f * k, 0.17f * k, 0.6f * k, 10, clothes);
    m.blob({0, 1.55f * k, 0}, 0.13f * k, 1.15f, skin);
    m.blob({0, 1.63f * k, 0.02f * k}, 0.135f * k, 0.75f, top);
    return m;
}

MeshData buildCostumed(int look, float height) {
    const float k = height / 1.3f;
    const Rgb skin{0.62f, 0.45f, 0.36f};
    MeshData m;
    switch (look) {
        case 1: {  // a ghost: a sheet with two dark eyes
            const Rgb sheet{0.85f, 0.85f, 0.88f};
            m.cylinder({0, 0, 0}, 0.32f * k, 0.2f * k, 1.0f * k, 12, sheet);
            m.blob({0, 1.05f * k, 0}, 0.22f * k, 1.1f, sheet);
            for (float x : {-0.07f, 0.07f}) m.box({x * k - 0.03f * k, 1.08f * k, -0.215f * k}, {x * k + 0.03f * k, 1.16f * k, -0.2f * k}, Rgb{0.02f, 0.02f, 0.02f});
            break;
        }
        case 2: {  // a pumpkin: round and orange, a green stalk hat
            m.cylinder({0, 0, 0}, 0.08f * k, 0.08f * k, 0.45f * k, 6, Rgb{0.08f, 0.10f, 0.05f});
            m.blob({0, 0.75f * k, 0}, 0.36f * k, 0.85f, kPumpkin);
            m.blob({0, 1.18f * k, 0}, 0.13f * k, 1.1f, skin);
            m.cylinder({0, 1.3f * k, 0}, 0.05f * k, 0.03f * k, 0.12f * k, 6, kStem);
            break;
        }
        case 3: {  // a skeleton: black, with white bones and a white skull
            const Rgb bone{0.85f, 0.83f, 0.78f};
            m.cylinder({0, 0, 0}, 0.13f * k, 0.16f * k, 0.6f * k, 8, Rgb{0.02f, 0.02f, 0.02f});
            m.cylinder({0, 0.6f * k, 0}, 0.17f * k, 0.14f * k, 0.45f * k, 8, Rgb{0.02f, 0.02f, 0.02f});
            for (int r = 0; r < 4; ++r) m.box({-0.14f * k, (0.66f + 0.09f * static_cast<float>(r)) * k, -0.17f * k}, {0.14f * k, (0.69f + 0.09f * static_cast<float>(r)) * k, -0.13f * k}, bone);
            m.blob({0, 1.17f * k, 0}, 0.13f * k, 1.1f, bone);
            break;
        }
        case 4: {  // a costume witch: black dress, green face, a tall pointed hat
            m.cylinder({0, 0, 0}, 0.25f * k, 0.13f * k, 1.05f * k, 10, Rgb{0.03f, 0.02f, 0.04f});
            m.blob({0, 1.15f * k, 0}, 0.12f * k, 1.1f, Rgb{0.15f, 0.40f, 0.08f});
            m.cylinder({0, 1.22f * k, 0}, 0.26f * k, 0.26f * k, 0.02f * k, 10, Rgb{0.02f, 0.02f, 0.02f});
            m.cylinder({0, 1.24f * k, 0}, 0.13f * k, 0.01f, 0.4f * k, 8, Rgb{0.02f, 0.02f, 0.02f});
            break;
        }
        default:  // a grown-up walking with them
            return buildFigure(height, Rgb{0.20f, 0.17f, 0.12f}, Rgb{0.15f, 0.10f, 0.06f}, skin);
    }
    // A candy bag in one hand.
    m.box({0.2f * k, 0.45f * k, -0.05f * k}, {0.34f * k, 0.65f * k, 0.08f * k}, kPumpkin);
    return m;
}

MeshData buildSpirit(int who, float height) {
    // Moonlight colours, kept plain: the town's own dead, as they were, not
    // anything frightening.
    const float k = height / 1.7f;
    const Rgb coat{0.62f, 0.72f, 0.85f}, pale{0.80f, 0.86f, 0.95f}, dim{0.45f, 0.55f, 0.72f};
    MeshData m;
    m.cylinder({0, 0, 0}, 0.24f * k, 0.19f * k, 0.9f * k, 12, coat);  // a long coat or skirt
    m.cylinder({0, 0.9f * k, 0}, 0.21f * k, 0.16f * k, 0.55f * k, 12, coat);
    m.blob({0, 1.55f * k, 0}, 0.13f * k, 1.15f, pale);
    switch (who) {
        case 0:  // the reader: a shawl
            m.cylinder({0, 1.3f * k, 0}, 0.25f * k, 0.15f * k, 0.18f * k, 12, dim);
            break;
        case 1:  // the farmer: a flat cap
            m.cylinder({0, 1.64f * k, -0.02f * k}, 0.15f * k, 0.13f * k, 0.06f * k, 10, dim);
            m.box({-0.1f * k, 1.64f * k, -0.24f * k}, {0.1f * k, 1.67f * k, -0.1f * k}, dim);
            break;
        case 2:  // the fiddler: his fiddle case under his arm
            m.orientedBox({0.2f * k, 0.95f * k, -0.25f * k}, {0.1f * k, 0, 0}, {0, 0.22f * k, 0}, {0, 0.05f * k, 0.6f * k}, dim);
            break;
        default:  // the lamplighter: a peaked cap and his long pole
            m.cylinder({0, 1.63f * k, 0}, 0.15f * k, 0.15f * k, 0.08f * k, 10, dim);
            m.box({0.26f * k, 0.0f, -0.32f * k}, {0.3f * k, 2.3f * k, -0.28f * k}, dim);
            break;
    }
    return m;
}

MeshData buildBag() {
    MeshData m;
    const Rgb orange{0.70f, 0.22f, 0.02f}, black{0.03f, 0.03f, 0.03f};
    m.box({-0.16f, 0.0f, -0.1f}, {0.16f, 0.34f, 0.1f}, orange);
    // A black jack-o'-lantern face on each broad side, and the handles.
    for (float z : {-0.101f, 0.101f}) m.box({-0.07f, 0.15f, z - 0.001f}, {0.07f, 0.2f, z + 0.001f}, black);
    m.box({-0.12f, 0.34f, -0.02f}, {-0.09f, 0.46f, 0.02f}, orange);
    m.box({0.09f, 0.34f, -0.02f}, {0.12f, 0.46f, 0.02f}, orange);
    m.box({-0.12f, 0.43f, -0.02f}, {0.12f, 0.46f, 0.02f}, orange);
    return m;
}

}  // namespace familiar::bridge
