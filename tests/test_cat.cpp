// The cat (M3): she climbs a fence, the woodpile, a shed and a roof that Wren
// cannot; fits through the hedge gap and cannot turn back into a girl inside
// it; cannot knock for candy or carry the bag, which stays where Wren changed
// until she picks it up (and Wren cannot trick-or-treat without it); noses up
// Mr. Hale's latch and the lychgate's bolt from the inside only, after which
// Wren walks through; and sees the spirits, which Wren never does. The spirits'
// words stay gentle (RESEARCH.md, sensitivity notes).
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>

#include "check.h"
#include "sim/cat.h"
#include "sim/doors.h"
#include "sim/world.h"

using namespace familiar;
namespace T = familiar::town;

namespace {

bool walkTo(World& w, float x, float z, float seconds) {
    for (int i = 0; i < static_cast<int>(seconds * kTickRate); ++i) {
        const float dx = x - w.player.x, dz = z - w.player.z;
        if (dx * dx + dz * dz < 0.04f) return true;
        Input in;
        in.forward = 1;
        in.yaw = detAtan2(dx, -dz);
        tick(w, in);
    }
    const float dx = x - w.player.x, dz = z - w.player.z;
    return dx * dx + dz * dz < 0.04f;
}

void place(World& w, float x, float z, float feet = 0.0f) {
    w.player.x = x;
    w.player.z = z;
    float top = 0;
    if (standAt(w, w.form, x, z, feet, top)) w.player.feet = top;
}

void press(World& w, bool use, bool toggle = false, bool trick = false) {
    Input in;
    in.yaw = w.player.yaw;
    in.use = use;
    in.toggleForm = toggle;
    in.trick = trick;
    tick(w, in);
}

void stand(World& w, T::HouseId h) {
    const Vec2 d = T::doorstep(h);
    place(w, d.x, d.z, 1.0f);
}

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

}  // namespace

int main(int, char**) {
    // Climbing: the cat goes up a picket fence, the woodpile, the rain barrel,
    // the shed and Nana's roof; Wren is stopped at each.
    {
        World c = newWorld(1);
        press(c, false, true);
        CHECK(c.form == Form::Cat);
        place(c, -45.0f, -3.0f);
        CHECK(walkTo(c, -45.0f, -5.7f, 4));  // onto the Okafors' front fence
        CHECK(std::fabs(c.player.feet - T::kPicketHeight) < 1e-4f);
        place(c, -22.5f, -26.8f);
        CHECK(walkTo(c, -23.7f, -26.8f, 4));  // up onto the woodpile
        CHECK(c.player.feet == T::kWoodpileHeight);
        place(c, -56.5f, -18.6f);
        CHECK(walkTo(c, -58.4f, -18.6f, 4));  // the rain barrel
        CHECK(c.player.feet == T::kRainBarrelHeight);
        CHECK(walkTo(c, -60.0f, -18.6f, 4));  // the shed
        CHECK(c.player.feet == T::kShedHeight);
        CHECK(walkTo(c, -63.0f, -18.6f, 4));  // onto Nana's roof
        CHECK(walkTo(c, -63.0f, -15.5f, 6));  // up to the ridge
        CHECK(c.player.feet > T::kHouses[T::Nana].eave);
        // Up on the roof she cannot become a girl.
        CHECK(!canBecomeGirl(c));
        press(c, false, true);
        CHECK(c.form == Form::Cat);

        World g = newWorld(1);
        place(g, -45.0f, -3.0f);
        CHECK(!walkTo(g, -45.0f, -5.7f, 4));
        place(g, -22.5f, -26.8f);
        CHECK(!walkTo(g, -23.7f, -26.8f, 4));
        CHECK(g.player.feet == 0.0f);
    }

    // The hedge gap: the cat fits, Wren does not, and inside the gap there is
    // no room to turn back into a girl.
    {
        World c = newWorld(1);
        press(c, false, true);
        place(c, 44.0f, T::kHedgeGapZ);
        CHECK(walkTo(c, 42.0f, T::kHedgeGapZ, 4));  // in the gap itself
        CHECK(!canBecomeGirl(c));
        press(c, false, true);
        CHECK(c.form == Form::Cat);
        CHECK(walkTo(c, 40.0f, T::kHedgeGapZ, 4));  // through, into the churchyard
        CHECK(canBecomeGirl(c));
        World g = newWorld(1);
        place(g, 44.0f, T::kHedgeGapZ);
        CHECK(!walkTo(g, 40.0f, T::kHedgeGapZ, 6));
    }

    // The bag: dropped where Wren changes, counted but not carried, and picked
    // up again by Wren. A cat at a door gets no candy; Wren with no bag cannot knock.
    {
        World w = newWorld(1);
        press(w, false);
        stand(w, T::Lindqvist);
        press(w, false, true);  // into a cat on the doorstep
        CHECK(w.form == Form::Cat && w.bagDown);
        CHECK(std::fabs(w.bag.x - w.player.x) < 1e-6f && std::fabs(w.bag.z - w.player.z) < 1e-6f);
        CHECK(useLabel(w) == "Meow");
        press(w, true);  // the cat "knocks"
        CHECK(w.candy == 0 && !w.knocked[T::Lindqvist] && w.answer == Answer::Kitty);
        // Walk off as a cat, turn back into Wren away from the bag: it stays.
        CHECK(walkTo(w, -12.0f, -3.0f, 8));
        press(w, false, true);
        CHECK(w.form == Form::Girl && w.bagDown);
        CHECK(!nearBag(w));
        CHECK(nextStep(w).find("candy bag") != std::string::npos);
        // Without her bag Wren cannot trick-or-treat at the Garcias'.
        stand(w, T::Garcia);
        CHECK(usePrompt(w).find("Fetch it") != std::string::npos);
        press(w, true);
        CHECK(w.candy == 0 && !w.knocked[T::Garcia] && w.answered != T::Garcia);
        // Back at the Lindqvists' step, E picks the bag up (and is not a knock).
        stand(w, T::Lindqvist);
        CHECK(nearBag(w) && useLabel(w) == "Pick up");
        CHECK(usePrompt(w).find("pick up") != std::string::npos);
        press(w, true);
        CHECK(!w.bagDown && w.candy == 0 && !w.knocked[T::Lindqvist]);
        press(w, true);  // now the knock
        CHECK(w.candy > 0 && w.knocked[T::Lindqvist]);
        // Changing back beside the bag picks it straight up.
        press(w, false, true);
        CHECK(w.bagDown);
        press(w, false, true);
        CHECK(w.form == Form::Girl && !w.bagDown);
    }

    // A neighbour asking for a trick gets no joke from a cat.
    {
        World w = newWorld(1);
        stand(w, T::Okafor);
        press(w, true);
        CHECK(w.asking == T::Okafor);
        press(w, false, true);  // turn into a cat at the door
        press(w, false, false, true);
        CHECK(w.asking < 0 && w.candy == 0 && !w.knocked[T::Okafor]);
    }

    // Mr. Hale's latch: shut to Wren outside; the cat climbs in from the
    // woodpile and noses it up from inside; then Wren walks through.
    {
        World w = newWorld(1);
        place(w, -37.0f, T::kHaleGateZ);
        CHECK(latchedOutside(w) == T::HaleGate);
        CHECK(usePrompt(w).find("latched") != std::string::npos);
        press(w, true);
        CHECK(!w.gateOpen[T::HaleGate]);
        CHECK(!walkTo(w, -30.0f, T::kHaleGateZ, 6));
        // The cat can't open it from outside either.
        press(w, false, true);
        place(w, -36.2f, T::kHaleGateZ);
        CHECK(latchHere(w) < 0);
        press(w, true);
        CHECK(!w.gateOpen[T::HaleGate]);
        // Over the fence from the woodpile, to the latch from inside.
        place(w, -22.5f, -26.8f);
        CHECK(walkTo(w, -23.7f, -26.8f, 4));
        CHECK(walkTo(w, -28.0f, -26.8f, 6));
        CHECK(T::kHaleYard.contains(w.player.x, w.player.z) && w.player.feet == 0.0f);
        CHECK(walkTo(w, -34.9f, T::kHaleGateZ, 8));
        CHECK(latchHere(w) == T::HaleGate && useLabel(w) == "Open");
        press(w, true);
        CHECK(w.gateOpen[T::HaleGate]);
        press(w, false, true);  // Wren again, inside the yard
        CHECK(w.form == Form::Girl);
        CHECK(walkTo(w, -38.0f, T::kHaleGateZ, 6));  // out through the open gate
    }

    // The lychgate: bolted; the cat goes through the hedge gap and works the
    // bolt from inside; then Wren walks in under the lychgate roof.
    {
        World w = newWorld(1);
        const float gx = (T::kLychgateX0 + T::kLychgateX1) / 2;
        place(w, gx, -28.5f);
        CHECK(latchedOutside(w) == T::Lychgate);
        CHECK(!walkTo(w, gx, -33.0f, 6));
        press(w, false, true);
        press(w, true);
        CHECK(!w.gateOpen[T::Lychgate]);
        place(w, 44.0f, T::kHedgeGapZ);
        CHECK(walkTo(w, 40.0f, T::kHedgeGapZ, 4));
        CHECK(walkTo(w, 36.0f, -33.0f, 8));
        CHECK(walkTo(w, gx, -31.0f, 10));
        CHECK(latchHere(w) == T::Lychgate);
        CHECK(usePrompt(w).find("bolt") != std::string::npos);
        press(w, true);
        CHECK(w.gateOpen[T::Lychgate]);
        CHECK(walkTo(w, gx, -28.0f, 4));
        press(w, false, true);
        CHECK(w.form == Form::Girl);
        CHECK(walkTo(w, gx, -33.0f, 6));  // Wren walks in
    }

    // Spirits: only a cat sees them, and the cat notes each one she meets.
    {
        World g = newWorld(1);
        place(g, 30.0f, -40.0f);
        for (const Walker& s : spirits(g)) CHECK(!s.shown);
        World c = g;
        press(c, false, true);
        for (const Walker& s : spirits(c)) CHECK(s.shown);
        CHECK(spiritsSeen(c) == 0);
        // Stand by the farmer: the cat sees him, and Wren in the same spot does not.
        place(c, 37.5f, -35.5f);
        press(c, false);
        CHECK(spiritNear(c) == Farmer);
        CHECK(c.spiritsSeen == (1u << Farmer));
        World g2 = newWorld(1);
        place(g2, 37.5f, -35.5f);
        press(g2, false);
        CHECK(spiritNear(g2) < 0 && g2.spiritsSeen == 0);
        // Spirits stay in the churchyard and on the hill path, and move with the clock.
        for (int t = 0; t < 600 * kTickRate; t += 15 * kTickRate) {
            World k = newWorld(1);
            k.tick = t;
            const std::vector<Walker> all = spirits(k);
            for (int i = 0; i < Lamplighter; ++i) CHECK(T::kChurchyard.contains(all[static_cast<size_t>(i)].at.x, all[static_cast<size_t>(i)].at.z));
            const Walker& l = all[Lamplighter];
            CHECK(l.at.x >= 59.9f && l.at.x <= T::kHill.x && l.at.z <= -1.9f && l.at.z >= T::kHill.z);
        }
        World a = newWorld(1), b = newWorld(1);
        b.tick = 20 * kTickRate;
        CHECK(spirits(a)[Reader].at.x != spirits(b)[Reader].at.x);
        CHECK(spirits(a)[Lamplighter].at.z != spirits(b)[Lamplighter].at.z);
        // Their words are gentle: no fright, no mockery (RESEARCH.md, sensitivity notes).
        for (int i = 0; i < SpiritCount; ++i) {
            const std::string text = lower(spiritText(i));
            CHECK(!text.empty());
            for (const char* word : {"scary", "spooky", "evil", "demon", "devil", "curse", "haunt", "boo", "blood", "creepy", "ghoul"})
                CHECK(text.find(word) == std::string::npos);
        }
    }

    // Same inputs, same world: the cat's run is deterministic.
    {
        auto run = [] {
            World w = newWorld(1);
            stand(w, T::Lindqvist);
            press(w, false, true);
            place(w, 44.0f, T::kHedgeGapZ);
            walkTo(w, 40.0f, T::kHedgeGapZ, 4);
            walkTo(w, 30.0f, -42.0f, 10);
            return fingerprint(w);
        };
        CHECK(run() == run());
    }
    TEST_MAIN_END();
}
