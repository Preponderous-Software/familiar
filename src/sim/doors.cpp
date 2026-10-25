#include "sim/doors.h"

#include <cmath>

namespace familiar {

namespace T = familiar::town;

namespace {

const Neighbour kNeighbours[T::HouseCount] = {
    {"Nana", 0, ""},
    {"Mrs. Okafor", 2, "\"Trick or treat! Oh, look at you. Two for you, and don't tell your Nana.\""},
    {"Mr. Hale", 0, ""},
    {"Mrs. Lindqvist", 3, "\"Full-size bars this year. Halloween comes once!\""},
    {"Dr. Pell", 1, "\"A toothbrush for you... and, all right, one candy. Brush after.\""},
    {"Mr. Garcia", 3, "\"Take three, mija. It's cold out there.\""},
    {"Ms. Bright", 2, "\"Wren! Two for you. Is your homework done? Never mind, it's Halloween.\""},
    {"Mr. Novak", 2, "\"Mind the spider! He's friendly. Two each.\""},
    {"Mr. Abbas", 1, "\"Happy Halloween! One from the jar. The shop is open till eleven.\""},
    {"Mrs. Whitfield", 2, "\"You made it past the skeletons! Brave girl. Two for you.\""},
};

}  // namespace

const Neighbour& neighbour(T::HouseId h) { return kNeighbours[h]; }

bool porchLightOn(const World& w, T::HouseId h) {
    if (h == T::Nana) return true;
    if (h == T::Hale) return false;  // dark since last winter
    const int minute = static_cast<int>(gameMinutes(w));
    if (h == T::Market) return minute < 23 * 60;
    return minute < kPorchLightsOff;
}

int atDoor(const World& w) {
    for (int i = 0; i < T::HouseCount; ++i) {
        const Vec2 d = T::doorstep(static_cast<T::HouseId>(i));
        const float dx = w.player.x - d.x, dz = w.player.z - d.z;
        if (dx * dx + dz * dz < kDoorReach * kDoorReach) return i;
    }
    return -1;
}

std::string usePrompt(const World& w) {
    const int h = atDoor(w);
    if (h < 0) return "";
    const auto id = static_cast<T::HouseId>(h);
    if (id == T::Nana) return "Nana's door. Go and knock on the neighbours'";
    if (!porchLightOn(w, id)) return id == T::Hale ? "Mr. Hale's porch light is off" : "The porch light is off";
    if (w.knocked[h]) return "You have been here tonight";
    return w.form == Form::Cat ? "E  meow at the door" : "E  knock: trick or treat!";
}

std::string answerText(const World& w) {
    if (w.answered < 0) return "";
    if (static_cast<float>(w.tick - w.answeredTick) / kTickRate > kAnswerSeconds) return "";
    const Neighbour& n = neighbour(static_cast<T::HouseId>(w.answered));
    if (w.kitty) return std::string(n.who) + ": \"Well, hello, puss! A treat for the kitty.\" (A cat can't carry candy.)";
    return std::string(n.who) + ": " + n.says;
}

std::string nextStep(const World& w) {
    int open = 0;
    for (int i = 0; i < T::HouseCount; ++i)
        if (!w.knocked[i] && i != T::Nana && porchLightOn(w, static_cast<T::HouseId>(i))) ++open;
    if (w.form == Form::Cat) return "As a cat: climb fences and roofs. Q to be Wren";
    if (open == 0) return "Every lit porch visited. Explore: the hill, the churchyard, the creek";
    return "Knock (E) at doors with a porch light on";
}

void doorsTick(World& w, const Input& in) {
    if (!in.use) return;
    const int h = atDoor(w);
    if (h < 0 || h == T::Nana || w.knocked[h] || !porchLightOn(w, static_cast<T::HouseId>(h))) return;
    w.answered = h;
    w.answeredTick = w.tick;
    w.kitty = w.form == Form::Cat;
    if (w.kitty) return;  // a cat can come back as Wren
    w.knocked[h] = true;
    w.candy += neighbour(static_cast<T::HouseId>(h)).candy;
}

}  // namespace familiar
