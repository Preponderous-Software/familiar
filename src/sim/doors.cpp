#include "sim/doors.h"

#include <cmath>

namespace familiar {

namespace T = familiar::town;

namespace {

const Neighbour kNeighbours[T::HouseCount] = {
    {"Nana", 0, false, Costume::None, "", "", "", ""},
    {"Mrs. Okafor", 2, true, Costume::None, "\"Two for you, and don't tell your Nana.\"", "",
     "\"Trick or treat? In this house it's a trick first. Tell us a joke!\" (T: tell a joke, E: just the treat)",
     "\"Ha! Grandpa Ike would have loved that one. Three for you.\""},
    {"Mr. Hale", 0, false, Costume::None, "", "", "", ""},
    {"Mrs. Lindqvist", 3, false, Costume::Pumpkin, "\"Full-size bars this year. Halloween comes once!\"",
     "\"A pumpkin! Like the ones on my step. Take another.\"", "", ""},
    {"Dr. Pell", 1, false, Costume::None, "\"A toothbrush for you... and, all right, one candy. Brush after.\"", "", "", ""},
    {"Mr. Garcia", 2, false, Costume::Skeleton, "\"Two for you. It's cold out there.\"",
     "\"A skeleton! Nobody's scared of bones in this house. Take one more.\"", "", ""},
    {"Ms. Bright", 2, true, Costume::Ghost, "\"Wren! Two for you. Is your homework done? Never mind.\"",
     "\"A ghost! My favourite. An extra one for haunting me.\"",
     "\"Trick or treat? You know the rule in my class: a joke first.\" (T: tell a joke, E: just the treat)",
     "\"Oh, that's terrible. I love it. Three for you.\""},
    {"Mr. Novak", 2, false, Costume::None, "\"Mind the spider! He's friendly. Two each.\"", "", "", ""},
    {"Mr. Abbas", 1, false, Costume::None, "\"Happy Halloween! One from the jar. The shop's open till eleven.\"", "", "", ""},
    {"Mrs. Whitfield", 2, false, Costume::Witch, "\"You made it past the skeletons! Two for you.\"",
     "\"A witch! The best costume on the street. Have another.\"", "", ""},
};

const char* const kJokes[kJokeCount] = {
    "Why didn't the skeleton go to the party? He had no body to go with.",
    "What do you call a cat on the beach at Halloween? Sandy Claws.",
    "Why are ghosts bad liars? You can see right through them.",
    "What room does a ghost not need? A living room.",
    "What's a pumpkin's favourite sport? Squash.",
    "Why do witches wear name tags? So they know which witch is which.",
    "What do you get if you cross a vampire with a snowman? Frostbite.",
    "Why did the owl say 'tweet'? Because it didn't give a hoot.",
};

}  // namespace

const char* costumeName(Costume c) {
    switch (c) {
        case Costume::Ghost: return "a ghost";
        case Costume::Pumpkin: return "a pumpkin";
        case Costume::Skeleton: return "a skeleton";
        case Costume::Witch: return "a witch (the green-faced, pointy-hat kind)";
        case Costume::None: break;
    }
    return "no costume yet";
}

const char* costumeShort(Costume c) {
    static const char* const kNames[] = {"none yet", "ghost", "pumpkin", "skeleton", "witch"};
    const int i = static_cast<int>(c);
    return i >= 0 && i <= 4 ? kNames[i] : "";
}

const Neighbour& neighbour(T::HouseId h) { return kNeighbours[h]; }

const char* joke(int i) { return kJokes[((i % kJokeCount) + kJokeCount) % kJokeCount]; }

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

bool canChooseCostume(const World& w) {
    if (w.form != Form::Girl || w.candy > 0) return false;
    for (bool k : w.knocked)
        if (k) return false;
    const float dx = w.player.x - T::kStart.x, dz = w.player.z - T::kStart.z;
    return dx * dx + dz * dz < kCostumeReach * kCostumeReach;
}

int housesLeft(const World& w) {
    int n = 0;
    for (int i = 0; i < T::HouseCount; ++i)
        if (!w.knocked[i] && i != T::Nana && neighbour(static_cast<T::HouseId>(i)).candy > 0 && porchLightOn(w, static_cast<T::HouseId>(i)))
            ++n;
    return n;
}

std::string usePrompt(const World& w) {
    const int h = atDoor(w);
    if (h < 0) {
        if (canChooseCostume(w)) return "Choose a costume:  1 ghost   2 pumpkin   3 skeleton   4 witch";
        return "";
    }
    const auto id = static_cast<T::HouseId>(h);
    if (id == T::Nana) {
        if (canChooseCostume(w)) return "Choose a costume:  1 ghost   2 pumpkin   3 skeleton   4 witch";
        return "Nana's door. The neighbours' porches are lit";
    }
    if (w.asking == h) return "T  tell a joke     E  just the treat";
    if (!porchLightOn(w, id)) return id == T::Hale ? "Mr. Hale's porch light is off" : "The porch light is off now";
    if (w.knocked[h]) return "You have been here tonight";
    return w.form == Form::Cat ? "E  meow at the door" : "E  knock: trick or treat!";
}

std::string answerText(const World& w) {
    if (w.answered < 0) return "";
    if (static_cast<float>(w.tick - w.answeredTick) / kTickRate > kAnswerSeconds && w.asking < 0) return "";
    const Neighbour& n = neighbour(static_cast<T::HouseId>(w.answered));
    const std::string who = std::string(n.who) + ": ";
    switch (w.answer) {
        case Answer::Kitty: return who + "\"Well, hello, puss! A treat for the kitty.\" (A cat can't carry candy.)";
        case Answer::Ask: return who + n.ask;
        case Answer::Joke: return std::string("You: \"") + joke(w.jokes - 1) + "\"\n" + who + n.laugh;
        case Answer::Liked: return who + n.liked;
        case Answer::Treat: return who + n.treat;
        case Answer::None: break;
    }
    return "";
}

std::string nextStep(const World& w) {
    if (w.form == Form::Cat) return "As a cat: climb fences and roofs. Q to be Wren";
    if (canChooseCostume(w) && w.costume == Costume::None) return "Choose a costume (1-4) on Nana's walk";
    const int left = housesLeft(w);
    if (left == 0) {
        if (static_cast<int>(gameMinutes(w)) >= kPorchLightsOff) return "Trick-or-treating is over. Explore the night";
        return "Every lit porch visited. Explore: the hill, the churchyard, the creek";
    }
    return "Knock (E) where a porch light is on: " + std::to_string(left) + " to go";
}

void doorsTick(World& w, const Input& in) {
    if (in.costume > 0 && in.costume <= 4 && canChooseCostume(w)) w.costume = static_cast<Costume>(in.costume);
    const int h = atDoor(w);
    if (w.asking >= 0 && h != w.asking) w.asking = -1;  // walked away before the trick
    if (h < 0 || h == T::Nana) return;
    const auto id = static_cast<T::HouseId>(h);
    const Neighbour& n = neighbour(id);
    auto give = [&](int candy, Answer a) {
        w.knocked[h] = true;
        w.candy += candy;
        w.answer = a;
        w.answered = h;
        w.answeredTick = w.tick;
        w.asking = -1;
    };
    const int bonus = n.likes != Costume::None && n.likes == w.costume ? 1 : 0;
    if (w.asking == h) {
        if (in.trick) {
            ++w.jokes;
            give(n.candy + 1 + bonus, Answer::Joke);
        } else if (in.use) {
            give(n.candy + bonus, bonus ? Answer::Liked : Answer::Treat);
        }
        return;
    }
    if (!in.use || w.knocked[h] || !porchLightOn(w, id) || n.candy <= 0) return;
    w.answered = h;
    w.answeredTick = w.tick;
    if (w.form == Form::Cat) {
        w.answer = Answer::Kitty;
        return;
    }
    if (n.asksTrick) {
        w.asking = h;
        w.answer = Answer::Ask;
        return;
    }
    give(n.candy + bonus, bonus ? Answer::Liked : Answer::Treat);
}

}  // namespace familiar
