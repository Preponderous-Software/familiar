// Hollowell: its ground, its creek, and where everything in town stands. One
// fixed layout, shared by the rules (walking, climbing, collision) and the
// render bridge (meshes), so what the player sees is what they walk on.
//
// World frame (Strata's): +X east, +Y up, +Z south, in metres. Elm Street
// runs east-west along z = 0; north of it is negative z.
#pragma once

#include <array>
#include <vector>

#include "sim/math.h"

namespace familiar::town {

// The terrain grid the ground is sampled on.
constexpr float kMin = -110.0f;  // west and north edge
constexpr float kMax = 110.0f;   // east and south edge
constexpr float kCell = 1.0f;
constexpr int kSamples = 221;    // per side: (kMax - kMin) / kCell + 1
constexpr float kWalkLimit = 104.0f;

// The creek runs north to south east of town, meandering a little.
constexpr float kCreekHalfWidth = 3.2f;  // water's edge from the middle
constexpr float kWaterY = -1.0f;
constexpr float kCreekBedY = -1.8f;
float creekX(float z);  // the middle of the creek at z

// Elm Street: the road, and the sidewalks either side.
constexpr float kStreetWest = -86.0f, kStreetEast = 46.0f;
constexpr float kRoadHalf = 4.0f, kSidewalkHalf = 5.4f;

// Height of the bare ground at (x, z).
float groundHeight(float x, float z);

// Something solid: a box or an upright cylinder from the ground up to its top,
// which may be flat or a gable roof. A body may stand on a solid whose top it
// can climb to; anything taller stops it (see world.cpp).
enum class Shape { Box, Circle };
enum class Top { Flat, GableX, GableZ };  // GableX: the ridge runs along x
struct Solid {
    Shape shape = Shape::Box;
    Box2 box;            // Box: the footprint
    Vec2 centre;         // Circle: the centre and radius
    float radius = 0;
    Top top = Top::Flat;
    float height = 0;    // Flat: the top; gables: the eaves
    float ridge = 0;     // gables: the ridge
    int gate = -1;       // >= 0: one of the gates (Gate), solid only while shut
    // The height of the top at (x, z), clamped into the footprint.
    float heightAt(float x, float z) const;
    // Whether a circle at (x, z) of radius r overlaps it.
    bool overlaps(float x, float z, float r) const;
};

// Gates the cat can open from the far side (see MVP.md).
enum Gate : int { HaleGate = 0, Lychgate = 1, GateCount = 2 };

// Every solid in town, built once.
const std::vector<Solid>& solids();

// The houses on Elm Street, west to east on each side.
enum HouseId : int {
    Nana = 0, Okafor, Hale, Lindqvist, Pell,  // north side, doors facing south
    Garcia, Bright, Novak, Market, Whitfield, // south side, doors facing north
    HouseCount
};

struct House {
    const char* name;  // as the HUD names it
    float x;           // the middle of the frontage
    bool north;        // on the north side of the street
    float width;       // along the street
    float depth;
    float eave, ridge; // heights above the plinth
    float wall[3];     // linear RGB of the siding
    float door[3];
    bool fence;        // a picket fence along the front
};
extern const std::array<House, HouseCount> kHouses;

constexpr float kHouseFront = 11.0f;  // |z| of the front wall
constexpr float kPorchDepth = 2.5f;   // the porch, between the front wall and the steps
constexpr float kPorchHeight = 0.45f;
constexpr float kFenceZ = 5.7f;       // |z| of the front picket fences
constexpr float kPicketHeight = 0.9f;

Box2 footprint(HouseId h);
Box2 porch(HouseId h);
Vec2 doorAt(HouseId h);       // the middle of the front door, on the wall
Vec2 doorstep(HouseId h);     // where to stand to knock: on the porch, before the door
Vec3 porchLight(HouseId h);   // the lamp by the door

// Landmarks.
constexpr Vec2 kStart{-66.0f, -7.2f};   // Nana's front walk, facing the street
constexpr float kStartYaw = kPi;        // facing south
constexpr Vec2 kBandstand{0.0f, 18.0f};
constexpr float kBandstandRadius = 4.0f;
constexpr float kBandstandHeight = 0.9f;
constexpr Vec2 kSwapTree{-5.0f, 27.0f};  // the big maple on the green

// The churchyard: a stone wall on three sides, a hedge on the east, the
// lychgate in the south wall. The church stands at the north end.
constexpr Box2 kChurchyard{10.0f, -64.0f, 42.0f, -30.0f};
constexpr float kChurchyardWall = 2.2f;
constexpr float kLychgateX0 = 22.0f, kLychgateX1 = 25.0f;
constexpr float kHedgeGapZ = -40.0f;   // the cat-sized gap in the east hedge
constexpr float kHedgeGap = 0.46f;
constexpr Box2 kChurch{14.0f, -62.0f, 30.0f, -50.0f};
constexpr Box2 kTower{14.0f, -50.0f, 18.5f, -45.5f};

// Mr. Hale's back yard: a tall board fence on three sides, the back of his
// house on the fourth, a latched gate on the west side and a woodpile outside
// the east fence.
constexpr Box2 kHaleYard{-35.5f, -32.0f, -24.5f, -20.0f};
constexpr float kHaleFence = 1.9f;
constexpr float kHaleGateZ = -26.0f;
constexpr Box2 kWoodpile{-24.35f, -28.0f, -23.1f, -25.5f};
constexpr float kWoodpileHeight = 0.9f;

// Nana's back yard: the herb bed, the fire pit with its iron pot, the shed
// and the rain barrel that are the way up onto her roof.
constexpr Vec2 kHerbBed{-63.5f, -24.0f};
constexpr Vec2 kFirePit{-68.0f, -25.5f};
constexpr Box2 kNanaShed{-61.5f, -19.6f, -59.0f, -16.0f};
constexpr float kShedHeight = 2.3f;
constexpr Vec2 kRainBarrel{-58.4f, -18.6f};
constexpr float kRainBarrelHeight = 1.0f;

// The bridge carries Elm Street over the creek.
constexpr Box2 kBridge{45.5f, -1.6f, 58.5f, 1.6f};
constexpr float kBridgeDeck = 0.15f;
constexpr float kBridgeRail = 1.15f;

// The orchard across the creek: a grid of apple trees.
constexpr Box2 kOrchard{63.0f, 8.0f, 81.0f, 30.0f};
constexpr Vec2 kScarecrow{72.0f, 19.0f};

// Lantern Hill: a round hill north-east of town with a ring of stones on top.
constexpr Vec2 kHill{80.0f, -62.0f};
constexpr float kHillRadius = 34.0f;
constexpr float kHillHeight = 15.0f;
constexpr float kCircleRadius = 6.0f;
constexpr int kStoneCount = 9;

constexpr Vec2 kOldOak{61.0f, -28.0f};
constexpr Vec2 kFairyRing{96.0f, -14.0f};

// Trees: trunks are solid. Built once from the layout (see town.cpp).
struct Tree {
    Vec2 at;
    float trunk;   // radius
    float height;  // to the top of the trunk
    bool apple;
};
const std::vector<Tree>& trees();

// Streetlamps along Elm Street.
constexpr int kStreetlampCount = 7;
extern const std::array<Vec2, kStreetlampCount> kStreetlamps;
constexpr float kStreetlampHeight = 4.2f;

}  // namespace familiar::town
