// Triangle meshes for Hollowell, built from the layout in sim/town.h. No
// engine here: Scene (bridge/scene.h) uploads what this builds, and tests
// check it without a GPU. (MeshData and its shapes are Atman's.)
#pragma once

#include <cstdint>
#include <vector>

#include "sim/math.h"
#include "sim/town.h"

namespace familiar::bridge {

// Linear RGB.
struct Rgb {
    float r = 1, g = 1, b = 1;
};

// An indexed triangle list with a normal and a colour per vertex. Faces do not
// share vertices, so shading is flat. Triangles wind counter-clockwise seen
// from the side their normal points to (Strata culls back faces).
struct MeshData {
    std::vector<float> positions, normals, colors;
    std::vector<uint32_t> indices;

    size_t vertexCount() const { return positions.size() / 3; }
    bool empty() const { return indices.empty(); }

    void quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Rgb color);
    void triangle(Vec3 a, Vec3 b, Vec3 c, Rgb color);
    void box(Vec3 lo, Vec3 hi, Rgb color);
    void orientedBox(Vec3 o, Vec3 ex, Vec3 ey, Vec3 ez, Rgb color);
    void cylinder(Vec3 base, float radiusBottom, float radiusTop, float height, int segments, Rgb color);
    void blob(Vec3 centre, float radius, float squash, Rgb color);
};

struct TerrainData {
    int samples = 0;
    float origin[3] = {0, 0, 0};
    float cell = 1;
    std::vector<float> heights;
    std::vector<float> colors;
};

TerrainData buildTerrain();

// The town's lit geometry: houses, porches, fences, the church and its yard,
// the bandstand, the bridge, the trees, the hill's stones, the pumpkins.
MeshData buildTown();
// What glows: lit windows, jack-o'-lantern faces, lamp heads. Drawn unlit.
MeshData buildGlow();
// The creek's surface.
MeshData buildWater();
// A gate, shut (it is removed when opened).
MeshData buildGate(town::Gate g);

// A person standing at the origin facing -z, about `height` tall, in two
// colours (clothes, and hair or hat). The scene moves it with a transform.
MeshData buildFigure(float height, Rgb clothes, Rgb top, Rgb skin);

// A trick-or-treater in costume (look: 1 ghost, 2 pumpkin, 3 skeleton, 4
// witch; 0 a grown-up in a coat), standing at the origin facing -z.
MeshData buildCostumed(int look, float height);

// Where the jack-o'-lanterns sit on a house's porch (two each, none at Hale's).
std::vector<Vec3> pumpkins(town::HouseId h);

}  // namespace familiar::bridge
