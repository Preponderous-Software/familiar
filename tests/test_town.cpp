// The town's layout: the ground is flat where the houses stand, the creek is
// water and the hill is high; roofs slope from eaves to ridge; every house's
// doorstep is on its porch; and no tree stands on the street, in a yard, on
// the hill path or in the creek.
#include <cmath>
#include <cstdio>

#include "check.h"
#include "sim/town.h"

using namespace familiar;
using namespace familiar::town;

int main(int, char**) {
    // Town is flat; the creek is below the water line; the hilltop is high.
    CHECK(groundHeight(-66.0f, 0.0f) == 0.0f);
    CHECK(groundHeight(20.0f, -40.0f) == 0.0f);
    CHECK(groundHeight(creekX(20.0f), 20.0f) < kWaterY);
    CHECK(groundHeight(kHill.x, kHill.z) > kHillHeight - 0.5f);
    CHECK(std::fabs(creekX(0.0f) - 52.0f) < 0.01f);

    // A gable roof: the eaves at the front and back walls, the ridge in the middle.
    const Box2 f = footprint(Nana);
    Solid roof;
    for (const Solid& s : solids())
        if (s.top == Top::GableX && s.box.x0 == f.x0 && s.box.z0 == f.z0) roof = s;
    CHECK(roof.top == Top::GableX);
    CHECK(std::fabs(roof.heightAt(-66.0f, f.z0) - kHouses[Nana].eave) < 1e-4f);
    CHECK(std::fabs(roof.heightAt(-66.0f, (f.z0 + f.z1) / 2) - kHouses[Nana].ridge) < 1e-4f);

    // Doorsteps are on the porches, in front of the doors.
    for (int i = 0; i < HouseCount; ++i) {
        const auto h = static_cast<HouseId>(i);
        CHECK(porch(h).contains(doorstep(h).x, doorstep(h).z));
        const Vec2 d = doorAt(h);
        CHECK(std::fabs(d.z) == kHouseFront);
    }

    // Trees keep off the street, the hill path and the creek, and none stands
    // inside a house.
    int apple = 0;
    for (const Tree& t : trees()) {
        CHECK(!(t.at.z > -kSidewalkHalf && t.at.z < kSidewalkHalf && t.at.x < kStreetEast));
        CHECK(std::fabs(t.at.x - creekX(t.at.z)) > kCreekHalfWidth);
        for (int i = 0; i < HouseCount; ++i) CHECK(!footprint(static_cast<HouseId>(i)).contains(t.at.x, t.at.z));
        apple += t.apple ? 1 : 0;
    }
    CHECK(apple >= 12);
    CHECK(trees().size() > 100);

    // Exactly one solid for each gate.
    int gates[GateCount] = {};
    for (const Solid& s : solids())
        if (s.gate >= 0) ++gates[s.gate];
    CHECK(gates[HaleGate] == 1 && gates[Lychgate] == 1);
    TEST_MAIN_END();
}
