// Uploads Hollowell to Strata and keeps it in step with the world: the terrain
// tile, the town (lit), what glows (unlit), the creek, the gates while shut,
// and the neighbour standing in the doorway after a knock.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "bridge/geometry.h"
#include "sim/world.h"
#include "strata/strata.hpp"

namespace familiar::bridge {

class Scene {
public:
    explicit Scene(strata::Engine& engine);
    ~Scene();
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    void update(const World& w);
    size_t meshCount() const { return meshes_; }

private:
    uint32_t upload(const MeshData& m, bool unlit = false, float alpha = 1.0f);
    void remove(uint32_t& id);

    strata::Engine& engine_;
    uint32_t town_ = 0, glow_ = 0, water_ = 0;
    std::array<uint32_t, town::GateCount> gate_{};
    std::array<uint32_t, town::HouseCount> neighbour_{};
    int shown_ = -1;  // the neighbour in a doorway, or -1
    std::array<uint32_t, 7> children_{};
    uint32_t nana_ = 0;
    std::array<uint32_t, 4> spirits_{};  // seen only by the cat (sim/cat.h)
    uint32_t bag_ = 0;                   // the candy bag, while it is on the ground
    void place(uint32_t id, Vec3 at, float yaw, bool shown);
    size_t meshes_ = 0;
};

}  // namespace familiar::bridge
