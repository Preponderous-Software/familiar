// The autopilot plays the night from a new game: a ghost costume on Nana's
// walk, every lit porch with candy from west to east, a joke at the Okafors'
// and at Ms. Bright's, and home to Nana's porch. It ends on the pinned
// fingerprint on every compiler, and as the World alone decides what it does,
// it plays the same from any point (see also tests/test_saves.cpp).
#include <cinttypes>
#include <cstdio>
#include <string>

#include "check.h"
#include "sim/autopilot.h"
#include "sim/doors.h"
#include "sim/world.h"

using namespace familiar;
namespace T = familiar::town;

int main(int, char**) {
    World w = newWorld(1);
    CHECK(!playthroughDone(w));
    CHECK(autopilotTarget(w) == T::Garcia);  // the westernmost lit porch with candy

    int64_t stuck = 0, longestStuck = 0;
    float lastX = w.player.x, lastZ = w.player.z;
    int knocks = 0;
    while (!playthroughDone(w) && w.tick < kPlaythroughLimit) {
        const int before = w.answered;
        tick(w, autopilot(w));
        if (w.answered != before && w.answered >= 0) ++knocks;
        // It never stands still for long: every wait is a knock or a joke.
        if (w.player.x == lastX && w.player.z == lastZ) longestStuck = ++stuck > longestStuck ? stuck : longestStuck;
        else stuck = 0;
        lastX = w.player.x;
        lastZ = w.player.z;
    }
    CHECK(playthroughDone(w));
    CHECK(longestStuck < kTickRate);
    CHECK(w.costume == kAutopilotCostume);
    CHECK(w.form == Form::Girl);
    for (int h = 0; h < T::HouseCount; ++h) {
        const auto id = static_cast<T::HouseId>(h);
        CHECK(w.knocked[static_cast<size_t>(h)] == (h != T::Nana && neighbour(id).candy > 0));
    }
    CHECK(w.jokes == 2);  // the Okafors and Ms. Bright asked for a trick
    // 3 + 3 + 1 + 2 + (2 + 1 + 1 for the ghost) + 2 + 1 + 2.
    CHECK(w.candy == 18);
    CHECK(gameMinutes(w) < kPorchLightsOff);  // all before the porch lights go off
    CHECK(knocks >= 8);
    CHECK(autopilotTarget(w) == -1);
    CHECK(std::string(autopilotDoing(w)) == "home on Nana's porch");

    const uint64_t fp = fingerprint(w);
    std::printf("playthrough ends at %s (tick %" PRId64 ") with %d candy, fingerprint %016" PRIx64 "\n",
                clockText(w).c_str(), w.tick, w.candy, fp);
    CHECK(fp == kPlaythroughFingerprint);

    // A cat in the middle of the night is turned back into Wren first.
    {
        World c = newWorld(1);
        Input q;
        q.yaw = c.player.yaw;
        q.toggleForm = true;
        tick(c, q);
        CHECK(c.form == Form::Cat);
        CHECK(autopilot(c).toggleForm);
        int64_t limit = kPlaythroughLimit;
        while (!playthroughDone(c) && --limit > 0) tick(c, autopilot(c));
        CHECK(playthroughDone(c));
        CHECK(c.candy == 18);
    }

    // The bag dropped on the street by a cat who wandered off: Wren turns back,
    // fetches it, and the night goes on to the same end.
    {
        World c = newWorld(1);
        Input in;
        in.yaw = kPi;  // south, out to the street
        in.toggleForm = true;
        tick(c, in);
        CHECK(c.bagDown);
        in.toggleForm = false;
        in.forward = 1;
        for (int t = 0; t < 2 * kTickRate; ++t) tick(c, in);
        CHECK(c.form == Form::Cat && c.bagDown);
        bool fetched = false;
        int64_t limit = kPlaythroughLimit;
        while (!playthroughDone(c) && --limit > 0) {
            tick(c, autopilot(c));
            if (!c.bagDown && c.form == Form::Girl) fetched = true;
        }
        CHECK(fetched);
        CHECK(playthroughDone(c));
        CHECK(c.candy == 18);
    }

    TEST_MAIN_END();
}
