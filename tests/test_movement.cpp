// Walking and climbing: Wren walks and hurries at her speeds, slides along
// walls, steps up onto porches but not over fences; the cat climbs fences,
// sheds and roofs by way of something lower, slips through the hedge gap Wren
// cannot, and drops back to the ground; nobody wades the creek, and the
// bridge crosses it.
#include <cmath>
#include <cstdio>

#include "check.h"
#include "sim/town.h"
#include "sim/world.h"

using namespace familiar;
namespace T = familiar::town;

namespace {

// Walks toward (x, z) for up to `seconds`; true if it got within 0.2 m.
bool walkTo(World& w, float x, float z, float seconds, bool hurry = false) {
    for (int i = 0; i < static_cast<int>(seconds * kTickRate); ++i) {
        const float dx = x - w.player.x, dz = z - w.player.z;
        if (dx * dx + dz * dz < 0.04f) return true;
        Input in;
        in.forward = 1;
        in.hurry = hurry;
        in.yaw = detAtan2(dx, -dz);
        tick(w, in);
    }
    const float dx = x - w.player.x, dz = z - w.player.z;
    return dx * dx + dz * dz < 0.04f;
}

World at(Form form, float x, float z) {
    World w = newWorld(1);
    w.form = form;
    w.player.x = x;
    w.player.z = z;
    float feet = 0;
    standAt(w, form, x, z, 0.0f, feet);
    w.player.feet = feet;
    return w;
}

}  // namespace

int main(int, char**) {
    // A new game starts on Nana's front walk, on the ground, facing the street.
    {
        const World w = newWorld(1);
        CHECK(w.player.x == T::kStart.x && w.player.z == T::kStart.z);
        CHECK(w.player.feet == 0.0f);
        CHECK(clockText(w) == "5:20 pm");
    }

    // Walking speed: ten seconds at 1.5 m/s along the empty street.
    {
        World w = at(Form::Girl, -70.0f, 0.0f);
        Input in;
        in.forward = 1;
        in.yaw = kPi / 2;  // east
        for (int i = 0; i < 10 * kTickRate; ++i) tick(w, in);
        CHECK(std::fabs(w.player.x - (-55.0f)) < 0.01f);
        in.hurry = true;
        for (int i = 0; i < 10 * kTickRate; ++i) tick(w, in);
        CHECK(std::fabs(w.player.x - (-19.0f)) < 0.01f);
        CHECK(clockText(w) == "5:26 pm");  // twenty seconds is six and two-thirds game minutes
    }

    // Wren steps up onto a porch, but a picket fence stops her.
    {
        World w = at(Form::Girl, -48.0f, -7.0f);
        CHECK(walkTo(w, -48.0f, -9.5f, 5));
        CHECK(w.player.feet == T::kPorchHeight);
        World v = at(Form::Girl, -45.0f, -3.0f);
        CHECK(!walkTo(v, -45.0f, -8.0f, 6));
        CHECK(v.player.z > -T::kFenceZ + 0.2f);
    }

    // Into a wall at an angle, she slides along it.
    {
        World w = at(Form::Girl, -40.0f, -8.0f);
        Input in;
        in.forward = 1;
        in.yaw = -0.5f;  // north-north-west, into the Okafors' side wall... and along it
        for (int i = 0; i < 4 * kTickRate; ++i) tick(w, in);
        CHECK(w.player.x < -40.5f);
    }

    // The cat hops a picket fence; Wren cannot.
    {
        World c = at(Form::Cat, -45.0f, -3.0f);
        CHECK(walkTo(c, -45.0f, -8.0f, 6));
        CHECK(c.player.feet == 0.0f);  // and down again on the far side
    }

    // Mr. Hale's fence is too tall to jump from the ground, but not from the woodpile.
    {
        World d = at(Form::Cat, -24.0f, -30.5f);  // beside the woodpile, on the ground
        CHECK(!walkTo(d, -27.0f, -30.5f, 6));
        CHECK(d.player.x > T::kHaleYard.x1);
        // From the woodpile's top, over the fence and down into the yard.
        World e = at(Form::Cat, -23.7f, -26.8f);
        e.player.feet = T::kWoodpileHeight;
        CHECK(walkTo(e, -28.0f, -26.8f, 6));
        CHECK(e.player.feet == 0.0f);
        CHECK(T::kHaleYard.contains(e.player.x, e.player.z));
        // The gate is shut until it is opened.
        World g = at(Form::Girl, -38.0f, T::kHaleGateZ);
        CHECK(!walkTo(g, -30.0f, T::kHaleGateZ, 8));
        g.gateOpen[T::HaleGate] = true;
        CHECK(walkTo(g, -30.0f, T::kHaleGateZ, 8));
    }

    // Up the rain barrel, onto Nana's shed, onto her roof, to the ridge.
    {
        World c = at(Form::Cat, -56.5f, -18.6f);
        CHECK(walkTo(c, -59.3f, -18.6f, 6));
        CHECK(c.player.feet == T::kShedHeight);
        CHECK(walkTo(c, -63.0f, -18.6f, 6));
        CHECK(c.player.feet > T::kHouses[T::Nana].eave);
        CHECK(walkTo(c, -63.0f, -15.5f, 6));
        CHECK(std::fabs(c.player.feet - T::kHouses[T::Nana].ridge) < 0.15f);
        // Off the back of the roof, down to the garden.
        CHECK(walkTo(c, -63.0f, -22.0f, 6));
        CHECK(c.player.feet == 0.0f);
        // Wren cannot climb the barrel.
        World g = at(Form::Girl, -56.5f, -18.6f);
        CHECK(!walkTo(g, -59.3f, -18.6f, 6));
    }

    // The hedge gap fits a cat and not a girl.
    {
        World c = at(Form::Cat, 44.0f, T::kHedgeGapZ);
        CHECK(walkTo(c, 40.0f, T::kHedgeGapZ, 6));
        World g = at(Form::Girl, 44.0f, T::kHedgeGapZ);
        CHECK(!walkTo(g, 40.0f, T::kHedgeGapZ, 6));
        CHECK(g.player.x > 42.0f);
    }

    // The creek stops anyone wading; the bridge crosses it.
    {
        World w = at(Form::Girl, 44.0f, 10.0f);
        CHECK(!walkTo(w, 60.0f, 10.0f, 20));
        CHECK(w.player.x < T::creekX(10.0f));
        World b = at(Form::Girl, 44.0f, 0.0f);
        CHECK(walkTo(b, 61.0f, 0.0f, 20));
        CHECK(walkTo(b, T::kHill.x - 5, T::kHill.z + 3, 120));
        CHECK(b.player.feet > 12.0f);  // up Lantern Hill
    }
    TEST_MAIN_END();
}
