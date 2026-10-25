#include "bridge/scene.h"

#include "sim/doors.h"

namespace familiar::bridge {

namespace T = familiar::town;

namespace {

// Each neighbour's clothes and hair, for the figure in the doorway.
struct Look {
    float height;
    Rgb clothes, top, skin;
};
const Look kLooks[T::HouseCount] = {
    {1.62f, {0.25f, 0.10f, 0.30f}, {0.80f, 0.80f, 0.80f}, {0.62f, 0.45f, 0.36f}},  // Nana
    {1.70f, {0.70f, 0.40f, 0.05f}, {0.03f, 0.02f, 0.02f}, {0.22f, 0.13f, 0.08f}},  // Mrs. Okafor
    {1.78f, {0.18f, 0.20f, 0.22f}, {0.60f, 0.60f, 0.62f}, {0.62f, 0.45f, 0.36f}},  // Mr. Hale
    {1.72f, {0.10f, 0.25f, 0.45f}, {0.80f, 0.70f, 0.40f}, {0.70f, 0.52f, 0.42f}},  // Mrs. Lindqvist
    {1.80f, {0.85f, 0.85f, 0.85f}, {0.25f, 0.18f, 0.12f}, {0.62f, 0.45f, 0.36f}},  // Dr. Pell
    {1.75f, {0.35f, 0.08f, 0.06f}, {0.05f, 0.04f, 0.03f}, {0.45f, 0.30f, 0.20f}},  // Mr. Garcia
    {1.65f, {0.10f, 0.35f, 0.25f}, {0.35f, 0.15f, 0.05f}, {0.62f, 0.45f, 0.36f}},  // Ms. Bright
    {1.85f, {0.30f, 0.30f, 0.12f}, {0.50f, 0.35f, 0.15f}, {0.68f, 0.50f, 0.40f}},  // Mr. Novak
    {1.72f, {0.20f, 0.30f, 0.20f}, {0.05f, 0.05f, 0.05f}, {0.48f, 0.33f, 0.22f}},  // Mr. Abbas
    {1.68f, {0.05f, 0.05f, 0.05f}, {0.55f, 0.10f, 0.05f}, {0.62f, 0.45f, 0.36f}},  // Mrs. Whitfield
};

}  // namespace

Scene::Scene(strata::Engine& engine) : engine_(engine) {
    const TerrainData t = buildTerrain();
    StrataTerrainTile tile{};
    tile.origin[0] = t.origin[0];
    tile.origin[1] = t.origin[1];
    tile.origin[2] = t.origin[2];
    tile.cell_size = t.cell;
    tile.samples_x = t.samples;
    tile.samples_z = t.samples;
    tile.heights = t.heights.data();
    tile.colors = t.colors.data();
    tile.color[3] = 1;
    engine_.setTerrain(&tile);
    town_ = upload(buildTown());
    glow_ = upload(buildGlow(), true);
    water_ = upload(buildWater());
}

void Scene::update(const World& w) {
    for (int g = 0; g < T::GateCount; ++g) {
        const bool shut = !w.gateOpen[static_cast<size_t>(g)];
        if (shut && !gate_[static_cast<size_t>(g)]) gate_[static_cast<size_t>(g)] = upload(buildGate(static_cast<T::Gate>(g)));
        if (!shut) remove(gate_[static_cast<size_t>(g)]);
    }
    // The neighbour stands in the doorway while their answer shows.
    const int at = answerText(w).empty() ? -1 : w.answered;
    if (at != shown_) {
        if (shown_ >= 0) remove(neighbour_[static_cast<size_t>(shown_)]);
        shown_ = at;
        if (at >= 0) {
            const Look& l = kLooks[at];
            const auto id = static_cast<T::HouseId>(at);
            neighbour_[static_cast<size_t>(at)] = upload(buildFigure(l.height, l.clothes, l.top, l.skin));
            const Vec2 d = T::doorAt(id);
            const bool north = T::kHouses[at].north;
            // Facing out of the door: a north house's door faces +z (yaw pi).
            const float c = north ? -1.0f : 1.0f;
            const float model[16] = {c, 0, 0, 0, 0, 1, 0, 0, 0, 0, c, 0, d.x, T::kPorchHeight, d.z + (north ? 0.35f : -0.35f), 1};
            engine_.setMeshTransform(neighbour_[static_cast<size_t>(at)], model);
        }
    }
}

void Scene::remove(uint32_t& id) {
    if (!id) return;
    engine_.removeMesh(id);
    id = 0;
    --meshes_;
}

Scene::~Scene() {
    try {
        for (uint32_t& id : gate_) remove(id);
        for (uint32_t& id : neighbour_) remove(id);
        remove(town_);
        remove(glow_);
        remove(water_);
        engine_.setTerrain(nullptr);
    } catch (const strata::Error&) {
    }
}

uint32_t Scene::upload(const MeshData& m, bool unlit) {
    if (m.empty()) return 0;
    StrataMesh mesh{};
    mesh.positions = m.positions.data();
    mesh.normals = m.normals.data();
    mesh.colors = m.colors.data();
    mesh.indices = m.indices.data();
    mesh.vertex_count = static_cast<int32_t>(m.vertexCount());
    mesh.index_count = static_cast<int32_t>(m.indices.size());
    mesh.color[3] = 1;
    ++meshes_;
    if (!unlit) return engine_.addMesh(mesh);
    StrataTexturedMesh tm{};
    tm.mesh = mesh;
    tm.flags = STRATA_MESH_UNLIT;
    return engine_.addTexturedMesh(tm);
}

}  // namespace familiar::bridge
