#include "bridge/scene.h"

#include "sim/cat.h"
#include "sim/craft.h"
#include "sim/doors.h"
#include "sim/npc.h"

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
    const std::vector<Walker> kids = children(World{});
    for (size_t i = 0; i < children_.size() && i < kids.size(); ++i) children_[i] = upload(buildCostumed(kids[i].look, kids[i].height));
    const Look& n = kLooks[T::Nana];
    nana_ = upload(buildFigure(n.height, n.clothes, n.top, n.skin));
    const std::vector<Walker> ss = spirits(World{});
    for (size_t i = 0; i < spirits_.size() && i < ss.size(); ++i)
        spirits_[i] = upload(buildSpirit(ss[i].look, ss[i].height), true, 0.42f);
    bag_ = upload(buildBag());
    smoke_ = upload(buildSmoke(), true, 0.55f);
}

// Turns a figure built facing -z to `yaw` and stands it at `at`; hidden ones go
// far below the ground.
void Scene::place(uint32_t id, Vec3 at, float yaw, bool shown) {
    if (!id) return;
    const float c = detCos(yaw), s = detSin(yaw);
    const float y = shown ? at.y : -500.0f;
    const float model[16] = {c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0, at.x, y, at.z, 1};
    engine_.setMeshTransform(id, model);
}

void Scene::update(const World& w) {
    const std::vector<Walker> kids = children(w);
    for (size_t i = 0; i < children_.size() && i < kids.size(); ++i) place(children_[i], kids[i].at, kids[i].yaw, kids[i].shown);
    const Walker n = nana(w);
    place(nana_, n.at, n.yaw, n.shown);
    const std::vector<Walker> ss = spirits(w);
    for (size_t i = 0; i < spirits_.size() && i < ss.size(); ++i) place(spirits_[i], ss[i].at, ss[i].yaw, ss[i].shown);
    place(bag_, w.bag, 0.4f, w.bagDown);
    Vec3 smokeAt;
    float smokeYaw = 0;
    const bool seen = smokeShown(w, smokeAt, smokeYaw);
    place(smoke_, smokeAt, smokeYaw, seen);
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
        for (uint32_t& id : children_) remove(id);
        remove(nana_);
        for (uint32_t& id : spirits_) remove(id);
        remove(bag_);
        remove(smoke_);
        remove(town_);
        remove(glow_);
        remove(water_);
        engine_.setTerrain(nullptr);
    } catch (const strata::Error&) {
    }
}

uint32_t Scene::upload(const MeshData& m, bool unlit, float alpha) {
    if (m.empty()) return 0;
    StrataMesh mesh{};
    mesh.positions = m.positions.data();
    mesh.normals = m.normals.data();
    mesh.colors = m.colors.data();
    mesh.indices = m.indices.data();
    mesh.vertex_count = static_cast<int32_t>(m.vertexCount());
    mesh.index_count = static_cast<int32_t>(m.indices.size());
    mesh.color[3] = alpha;
    ++meshes_;
    if (!unlit && alpha >= 1.0f) return engine_.addMesh(mesh);
    StrataTexturedMesh tm{};
    tm.mesh = mesh;
    tm.flags = (unlit ? STRATA_MESH_UNLIT : 0u) | (alpha < 1.0f ? STRATA_MESH_TRANSLUCENT : 0u);
    return engine_.addTexturedMesh(tm);
}

}  // namespace familiar::bridge
