// Trick-or-treating: a costume is chosen on Nana's walk before the first
// knock; a lit porch answers once; the Okafors and Ms. Bright ask for a trick
// and give one more for a joke (or the plain treat for E); a liked costume
// earns one more; a cat gets a treat for the kitty and no candy; porch lights
// go off at 9 pm, except the market's; Mr. Hale's is off.
#include <cstdio>

#include "check.h"
#include "sim/doors.h"
#include "sim/npc.h"
#include "sim/world.h"

using namespace familiar;
namespace T = familiar::town;

namespace {

void stand(World& w, T::HouseId h) {
    const Vec2 d = T::doorstep(h);
    w.player.x = d.x;
    w.player.z = d.z;
    w.player.feet = T::kPorchHeight;
}

void press(World& w, bool use, bool trick = false, int costume = 0, bool toggle = false) {
    Input in;
    in.yaw = w.player.yaw;
    in.use = use;
    in.trick = trick;
    in.costume = costume;
    in.toggleForm = toggle;
    tick(w, in);
}

}  // namespace

int main(int, char**) {
    // A costume on Nana's walk, before any knock.
    World w = newWorld(1);
    CHECK(canChooseCostume(w));
    press(w, false, false, 2);
    CHECK(w.costume == Costume::Pumpkin);

    // The Okafors ask for a trick; a joke earns one more.
    stand(w, T::Okafor);
    press(w, true);
    CHECK(w.asking == T::Okafor && w.candy == 0 && w.answer == Answer::Ask);
    CHECK(answerText(w).find("joke") != std::string::npos);
    press(w, false, true);
    CHECK(w.candy == 3 && w.knocked[T::Okafor] && w.asking < 0 && w.answer == Answer::Joke);
    CHECK(answerText(w).find("skeleton") != std::string::npos);  // the first joke
    press(w, true);
    CHECK(w.candy == 3);  // once only
    CHECK(!canChooseCostume(w));

    // The Lindqvists love a pumpkin.
    stand(w, T::Lindqvist);
    press(w, true);
    CHECK(w.candy == 3 + 4 && w.answer == Answer::Liked);

    // Ms. Bright asks; E takes the plain treat.
    stand(w, T::Bright);
    press(w, true);
    CHECK(w.asking == T::Bright);
    press(w, true);
    CHECK(w.candy == 7 + 2 && w.answer == Answer::Treat);

    // Walking away from an asked-for trick forgets it.
    stand(w, T::Okafor);
    World v = newWorld(1);
    stand(v, T::Bright);
    press(v, true);
    CHECK(v.asking == T::Bright);
    v.player.x += 5;
    press(v, false);
    CHECK(v.asking < 0 && !v.knocked[T::Bright]);

    // A cat at the door: a treat for the kitty, no candy, and the house still owes Wren.
    World c = newWorld(1);
    press(c, false, false, 0, true);
    CHECK(c.form == Form::Cat);
    stand(c, T::Garcia);
    press(c, true);
    CHECK(c.candy == 0 && c.answer == Answer::Kitty && !c.knocked[T::Garcia]);

    // Mr. Hale's porch is dark; at 9 pm the rest go dark too, but the market stays open.
    World late = newWorld(1);
    CHECK(!porchLightOn(late, T::Hale));
    late.tick = static_cast<int64_t>((kPorchLightsOff - kStartMinute) / kGameMinutesPerSecond * kTickRate) + 1;
    CHECK(!porchLightOn(late, T::Novak) && porchLightOn(late, T::Market));
    stand(late, T::Novak);
    press(late, true);
    CHECK(late.candy == 0);
    stand(late, T::Market);
    press(late, true);
    CHECK(late.candy == 1);

    // Children are out once the porch lights are, and keep to the sidewalks.
    World early = newWorld(1);
    CHECK(!children(early)[0].shown);
    World dusk = newWorld(1);
    dusk.tick = 60 * 3 * kTickRate;  // 6:20 pm
    for (const Walker& k : children(dusk)) {
        CHECK(k.shown);
        CHECK(k.at.x >= -80.01f && k.at.x <= 40.01f);
        CHECK(k.at.z > -5.0f && k.at.z < 5.0f);
    }
    TEST_MAIN_END();
}
