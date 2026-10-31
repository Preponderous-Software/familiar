#include "sim/craft.h"

#include <cmath>
#include <cstdio>
#include <utility>

#include "sim/doors.h"
#include "sim/npc.h"

namespace familiar {

namespace T = familiar::town;

namespace {

// The family lanterns (CONCEPT.md, the family lanterns).
const LanternSpot kLanterns[kLanternCount] = {
    {"Nana's window lantern", "for Nana's mother, Ada", {-68.6f, 1.15f, -10.8f}, {-68.2f, -9.9f}},
    {"Grandpa Ike's lantern", "on the Okafors' porch", {-50.4f, T::kPorchHeight, -9.0f}, {-49.9f, -9.6f}},
    {"June Hale's lantern", "on Mr. Hale's back step", {-30.0f, 0.25f, -20.5f}, {-30.0f, -21.6f}},
    {"the lychgate lantern", "for everyone in the churchyard", {23.5f, 2.3f, -30.0f}, {23.5f, -28.8f}},
};

constexpr Vec2 kSmokeSit{80.6f, -61.2f};  // in the circle, beside Nana
constexpr float kSmokeSpeed = 6.0f;       // she keeps up with a hurrying cat
constexpr float kSmokeSettle = 1.5f;
constexpr float kSmokeLeaves = 0.8f;      // walking away at midnight, metres per second
constexpr float kSmokeGone = 14.0f;       // seconds until she is out of sight

float dist2(float ax, float az, float bx, float bz) {
    const float dx = ax - bx, dz = az - bz;
    return dx * dx + dz * dz;
}

bool near(const World& w, Vec2 p, float r) { return dist2(w.player.x, w.player.z, p.x, p.z) < r * r; }

enum class Act { None, Light, Stir, Herb, Apple, Buy, Haggle, Offer };

bool lanternReady(const World& w, int i) {
    switch (i) {
        // Mrs. Okafor asks Wren to light it when she answers the door. After 9 pm,
        // with the porch light off and nobody coming to the door, it is lit for
        // Grandpa Ike anyway: otherwise a player who never knocked there could
        // never light all four, and Smoke would never go home.
        case OkaforPorch: return w.knocked[T::Okafor] || !porchLightOn(w, T::Okafor);
        case HaleStep: return w.gateOpen[T::HaleGate];
        case LychLantern: return w.gateOpen[T::Lychgate];
        default: return true;
    }
}

bool canBrew(const World& w) { return !w.cider && (w.stirs > 0 || (w.apples >= kApplesWanted && w.rosemary && w.cinnamon)); }

// The deed E (or T, for Haggle) would do here.
Act actionHere(const World& w) {
    if (!carrying(w) || w.asking >= 0) return Act::None;
    const int l = lanternHere(w);
    if (l >= 0 && lanternReady(w, l)) return Act::Light;
    if (atPot(w) && canBrew(w)) return Act::Stir;
    if (atHerbBed(w) && !w.rosemary && !w.cider && w.stirs == 0) return Act::Herb;
    if (atAppleTree(w) && w.apples < kApplesWanted) return Act::Apple;
    if (atMarket(w) && shopOpen(w) && !w.cinnamon && !w.cider && w.stirs == 0)
        return w.candy >= kCinnamonPrice ? Act::Buy : Act::Haggle;
    if (atCircle(w) && !w.offered && w.cider && w.apples >= 1 && nanaArrived(w)) return Act::Offer;
    return Act::None;
}

void did(World& w, Deed d, int32_t arg = 0) {
    w.deed = d;
    w.deedArg = arg;
    w.deedTick = w.tick;
}

std::string minuteText(int minute) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%d:%02d", (minute / 60 + 11) % 12 + 1, minute % 60);
    return buf;
}

std::string missing(const World& w) {
    std::string out;
    auto add = [&](const std::string& s) { out += (out.empty() ? "" : ", ") + s; };
    if (w.apples < kApplesWanted) add(std::to_string(kApplesWanted - w.apples) + (kApplesWanted - w.apples == 1 ? " apple" : " apples") + " (the orchard)");
    if (!w.rosemary) add("rosemary (Nana's herb bed)");
    if (!w.cinnamon) add("a cinnamon stick (Abbas's Market)");
    return out;
}

void moveToward(float& x, float& z, float tx, float tz, float step, float stopAt) {
    const float dx = tx - x, dz = tz - z;
    const float d = std::sqrt(dx * dx + dz * dz);
    if (d <= stopAt) return;
    const float k = std::fmin(step, d - stopAt) / d;
    x += dx * k;
    z += dz * k;
}

void smokeTick(World& w) {
    const bool cat = w.form == Form::Cat;
    const bool close = dist2(w.player.x, w.player.z, w.smokeX, w.smokeZ) < kSmokeFind * kSmokeFind;
    switch (w.smoke) {
        case SmokeState::Lost:
        case SmokeState::Waiting:
            if (cat && close) {
                did(w, Deed::Found, w.smoke == SmokeState::Waiting ? 1 : 0);
                w.smoke = SmokeState::Following;
            }
            break;
        case SmokeState::Following:
            if (!cat) {
                w.smoke = SmokeState::Waiting;
                did(w, Deed::Waits);
                break;
            }
            moveToward(w.smokeX, w.smokeZ, w.player.x, w.player.z, kSmokeSpeed / kTickRate, kSmokeBehind);
            if (dist2(w.smokeX, w.smokeZ, T::kHill.x, T::kHill.z) < (T::kCircleRadius - 0.8f) * (T::kCircleRadius - 0.8f)) {
                w.smoke = SmokeState::AtCircle;
                did(w, Deed::Circle);
            }
            break;
        case SmokeState::AtCircle:
            moveToward(w.smokeX, w.smokeZ, kSmokeSit.x, kSmokeSit.z, kSmokeSettle / kTickRate, 0.0f);
            if (w.offered && lanternsLit(w) == kLanternCount && gameMinutes(w) >= kMidnight) {
                w.smoke = SmokeState::Home;
                w.homeTick = w.tick;
                did(w, Deed::Home);
            }
            break;
        case SmokeState::Home: break;
    }
}

}  // namespace

const LanternSpot& lantern(int i) { return kLanterns[((i % kLanternCount) + kLanternCount) % kLanternCount]; }

int lanternsLit(const World& w) {
    int n = 0;
    for (bool l : w.lit) n += l ? 1 : 0;
    return n;
}

const std::vector<Vec2>& smokeTrail() {
    static const std::vector<Vec2> t = {
        {-63.5f, -24.0f}, {-58.0f, -27.0f}, {-40.0f, -34.0f}, {-10.0f, -27.0f}, {6.0f, -24.0f},  {44.0f, -24.0f},
        {44.0f, -3.0f},   {46.0f, -0.5f},   {58.0f, -0.5f},   {60.0f, -4.0f},   {58.8f, -20.0f}, kSmokeStart,
    };
    return t;
}

bool smokeShown(const World& w, Vec3& at, float& yaw) {
    float x = w.smokeX, z = w.smokeZ;
    yaw = detAtan2(w.player.x - x, -(w.player.z - z));  // she looks at the cat
    if (w.smoke == SmokeState::Home) {
        const float t = static_cast<float>(w.tick - w.homeTick) / kTickRate;
        if (t > kSmokeGone) return false;
        // North-east, out between the stones, away from the town.
        x += 0.6f * kSmokeLeaves * t;
        z -= 0.8f * kSmokeLeaves * t;
        yaw = detAtan2(0.6f, 0.8f);
    } else if (w.form != Form::Cat && !(w.smoke == SmokeState::AtCircle && w.offered)) {
        return false;
    }
    at = {x, T::kBridge.contains(x, z) ? T::kBridgeDeck : T::groundHeight(x, z), z};
    return true;
}

bool nanaAtCircle(const World& w) { return nanaArrived(w); }
int nanaArrives() { return nanaArrivalMinute(); }

bool carrying(const World& w) { return w.form == Form::Girl && !w.bagDown; }  // the jar and basket go with the bag (M3)

int lanternHere(const World& w) {
    for (int i = 0; i < kLanternCount; ++i) {
        if (w.lit[static_cast<size_t>(i)]) continue;
        const Vec3 a = kLanterns[i].at;
        if (near(w, {a.x, a.z}, kLanternReach)) return i;
    }
    return -1;
}

bool atAppleTree(const World& w) {
    for (const T::Tree& t : T::trees())
        if (t.apple && near(w, t.at, kReach + t.trunk)) return true;
    return false;
}
bool atHerbBed(const World& w) { return near(w, T::kHerbBed, kHerbReach); }
bool atPot(const World& w) { return near(w, T::kFirePit, kPotReach); }
bool atMarket(const World& w) { return atDoor(w) == T::Market; }
bool atCircle(const World& w) { return near(w, T::kHill, kOfferReach); }
bool shopOpen(const World& w) { return w.knocked[T::Market] || !porchLightOn(w, T::Market); }
bool haggling(const World& w) { return actionHere(w) == Act::Haggle; }

std::string craftPrompt(const World& w) {
    if (w.asking >= 0) return "";
    const int l = lanternHere(w);
    if (!carrying(w)) {
        if (l < 0) return "";
        return w.form == Form::Cat ? "A cat can't carry the lantern jar. Q to be Wren"
                                   : "The lantern jar is in your bag, where you became a cat";
    }
    switch (actionHere(w)) {
        case Act::Light: return std::string("E  light ") + kLanterns[l].name + ", " + kLanterns[l].forWhom;
        case Act::Stir:
            if (w.stirs == 0) return "E  put in an apple, the rosemary and the cinnamon, and stir";
            return "E  stir the pot (" + std::to_string(w.stirs) + " of " + std::to_string(kStirs) + ")";
        case Act::Herb: return "E  pick a sprig of rosemary";
        case Act::Apple: return "E  pick an apple";
        case Act::Buy: return "E  buy a cinnamon stick (" + std::to_string(kCinnamonPrice) + " candy)";
        case Act::Haggle:
            return "A cinnamon stick is " + std::to_string(kCinnamonPrice) + " candy; you have " + std::to_string(w.candy) +
                   ".  T  offer Mr. Abbas a joke instead";
        case Act::Offer: return "E  make the offering with Nana";
        case Act::None: break;
    }
    // Nothing to do here, but something to say about it (when the lantern is
    // nearer than any door in reach, so the door's own line is not hidden).
    const int door = atDoor(w);
    const bool lanternNearer =
        l >= 0 && (door < 0 || dist2(w.player.x, w.player.z, kLanterns[l].at.x, kLanterns[l].at.z) <
                                   dist2(w.player.x, w.player.z, T::doorstep(static_cast<T::HouseId>(door)).x,
                                         T::doorstep(static_cast<T::HouseId>(door)).z));
    if (lanternNearer) {
        if (l == OkaforPorch) return "The Okafors' lantern, for Grandpa Ike. Knock first (before 9 pm): it is theirs to ask";
        if (l == LychLantern) return "A lantern hangs in the lychgate, but the gate is bolted from inside";
        return "";
    }
    if (atPot(w)) {
        if (w.cider) return "The pot is empty: Nana's cider is in your flask";
        return "Nana's iron pot. For the cider: " + missing(w);
    }
    if (atAppleTree(w) && w.apples >= kApplesWanted) return "Two apples in the basket: one for the pot, one for the offering";
    if (atCircle(w) && !w.offered) {
        if (!nanaArrived(w)) return "Nana meets you here at about " + minuteText(nanaArrives()) + " pm";
        return "For the offering: Nana's cider and an apple";
    }
    return "";
}

std::string reachHint(const World& w) {
    if (w.form != Form::Girl || w.asking >= 0) return "";
    const float fx = std::sin(w.player.yaw), fz = -std::cos(w.player.yaw);
    // How far off, if (x, z) is within kHintRange and within about 20 degrees of straight ahead; else -1.
    auto ahead = [&](float x, float z) {
        const float dx = x - w.player.x, dz = z - w.player.z;
        const float d = std::sqrt(dx * dx + dz * dz);
        if (d < 0.01f || d > kHintRange) return -1.0f;
        return (dx * fx + dz * fz) / d > 0.94f ? d : -1.0f;
    };
    std::string best;
    float bestD = kHintRange + 1;
    if (carrying(w))
        for (int i = 0; i < kLanternCount; ++i) {
            if (w.lit[static_cast<size_t>(i)] || !lanternReady(w, i)) continue;
            const float d = ahead(kLanterns[i].at.x, kLanterns[i].at.z);
            if (d < 0 || d >= bestD) continue;
            bestD = d;
            best = std::string(kLanterns[i].name) + ": come closer to light it";
        }
    if (!w.bagDown)
        for (int i = 0; i < T::HouseCount; ++i) {
            const auto id = static_cast<T::HouseId>(i);
            if (id == T::Nana || w.knocked[i] || !porchLightOn(w, id) || neighbour(id).candy <= 0) continue;
            const Vec2 d0 = T::doorAt(id);
            const float d = ahead(d0.x, d0.z);
            if (d < 0 || d >= bestD) continue;
            bestD = d;
            best = std::string(T::kHouses[i].name) + (id == T::Market ? "" : " door") + ": come closer to knock";
        }
    if (!best.empty() && best[0] >= 'a' && best[0] <= 'z') best[0] = static_cast<char>(best[0] - 'a' + 'A');
    return best;
}

std::string craftWord(const World& w) {
    switch (actionHere(w)) {
        case Act::Light: return "Light";
        case Act::Stir: return "Stir";
        case Act::Herb:
        case Act::Apple: return "Pick";
        case Act::Buy: return "Buy";
        case Act::Offer: return "Offer";
        case Act::Haggle:
        case Act::None: break;
    }
    return "";
}

std::string lanternLine(const World& w) { return std::to_string(lanternsLit(w)) + " of " + std::to_string(kLanternCount); }

std::string basketLine(const World& w) {
    std::string out;
    auto add = [&](const std::string& s) { out += (out.empty() ? "" : ", ") + s; };
    if (w.cider) add(w.offered ? "the cider is offered" : "Nana's cider");
    else if (w.stirs > 0) add("the cider is on");
    if (w.apples > 0) add(std::to_string(w.apples) + (w.apples == 1 ? " apple" : " apples"));
    if (w.rosemary) add("rosemary");
    if (w.cinnamon) add("cinnamon");
    return out.empty() ? "empty" : out;
}

std::string smokeLine(const World& w) {
    switch (w.smoke) {
        case SmokeState::Lost: return "not found yet";
        case SmokeState::Following: return "following you";
        case SmokeState::Waiting: return "waiting for the cat";
        case SmokeState::AtCircle: return "at the stones";
        case SmokeState::Home: return "gone home";
    }
    return "";
}

namespace {

constexpr const char* kNanaLanternNext = "Light Nana's window lantern: E at the window left of her door";

bool knockedAnywhere(const World& w) {
    for (bool k : w.knocked)
        if (k) return true;
    return false;
}

}  // namespace

namespace {

Vec3 ground(Vec2 p, float up) { return {p.x, T::groundHeight(p.x, p.z) + up, p.z}; }
Vec3 doorGoal(T::HouseId h) {
    const Vec2 d = T::doorAt(h);
    return {d.x, T::kPorchHeight + 1.2f, d.z + (T::kHouses[h].north ? 0.1f : -0.1f)};
}
Vec3 lanternGoal(int i) { return kLanterns[i].at; }

// The doors' nextStep, with where it points: the bag, or the nearest door still to knock at.
NextStep doorsStep(const World& w) {
    NextStep n{nextStep(w)};
    if (w.form == Form::Girl && w.bagDown) {
        n.goal = true;
        n.at = {w.bag.x, w.bag.y + 0.3f, w.bag.z};
        return n;
    }
    if (w.form != Form::Girl || housesLeft(w) == 0 || (canChooseCostume(w) && w.costume == Costume::None)) return n;
    float best = 1e9f;
    for (int i = 0; i < T::HouseCount; ++i) {
        const auto id = static_cast<T::HouseId>(i);
        if (id == T::Nana || w.knocked[i] || neighbour(id).candy <= 0 || !porchLightOn(w, id)) continue;
        const Vec2 d = T::doorstep(id);
        const float d2 = dist2(w.player.x, w.player.z, d.x, d.z);
        if (d2 < best) {
            best = d2;
            n.goal = true;
            n.at = doorGoal(id);
        }
    }
    return n;
}

NextStep to(std::string text, Vec3 at) { return {std::move(text), true, at}; }

// The nearest of the cider's missing ingredients.
Vec3 nearestMissing(const World& w) {
    Vec3 out = ground(T::kHerbBed, 0.3f);
    float best = 1e9f;
    auto consider = [&](Vec3 p) {
        const float d2 = dist2(w.player.x, w.player.z, p.x, p.z);
        if (d2 < best) {
            best = d2;
            out = p;
        }
    };
    if (w.apples < kApplesWanted)
        for (const T::Tree& t : T::trees())
            if (t.apple) consider(ground(t.at, 1.8f));
    if (!w.rosemary) consider(ground(T::kHerbBed, 0.3f));
    if (!w.cinnamon) consider(doorGoal(T::Market));
    return out;
}

}  // namespace

NextStep tonightStep(const World& w) {
    const int minute = static_cast<int>(gameMinutes(w));
    const Vec3 hill = ground(T::kHill, 1.0f);
    if (w.form == Form::Cat) {
        switch (w.smoke) {
            case SmokeState::Lost: return to("Smoke's paw prints glow for a cat: follow them from Nana's herb bed", ground(T::kHerbBed, 0.3f));
            case SmokeState::Waiting: return to("Smoke is waiting where you left her: go back for her", ground({w.smokeX, w.smokeZ}, 0.3f));
            case SmokeState::Following: return to("Lead Smoke up Lantern Hill, into the ring of stones", hill);
            default: break;
        }
        return doorsStep(w);
    }
    if (w.bagDown || (canChooseCostume(w) && w.costume == Costume::None)) return doorsStep(w);  // fetch the bag first (M3)
    const bool knocking = housesLeft(w) > 0 && minute < kPorchLightsOff;
    if (!w.lit[NanaWindow] && !(knocking && knockedAnywhere(w))) return to(kNanaLanternNext, lanternGoal(NanaWindow));
    if (knocking) {
        if (w.knocked[T::Okafor] && !w.lit[OkaforPorch]) return to("Light Grandpa Ike's lantern on the Okafors' porch", lanternGoal(OkaforPorch));
        return doorsStep(w);
    }
    for (int i = 0; i < kLanternCount; ++i)
        if (!w.lit[static_cast<size_t>(i)] && lanternReady(w, i))
            return to(std::string("Light ") + kLanterns[i].name + ", " + kLanterns[i].forWhom, lanternGoal(i));
    if (!w.cider) {
        if (w.stirs > 0 || (w.apples >= kApplesWanted && w.rosemary && w.cinnamon))
            return to("Brew Nana's cider at the iron pot behind her house", ground(T::kFirePit, 0.6f));
        return to("For Nana's cider: " + missing(w), nearestMissing(w));
    }
    if (w.smoke == SmokeState::Lost) return to("Find Smoke, Nana's old cat: only a cat can see her trail (Q)", ground(T::kHerbBed, 0.3f));
    if (w.smoke == SmokeState::Waiting) return to("Smoke waits where you left her: be a cat to lead her", ground({w.smokeX, w.smokeZ}, 0.3f));
    if (!w.lit[HaleStep])
        return to("June Hale's lantern: Mr. Hale's back gate is latched from inside. A cat could open it",
                  ground({T::kHaleYard.x0, T::kHaleGateZ}, 1.0f));
    if (!w.lit[LychLantern])
        return to("The lychgate is bolted from inside. A cat could find a way into the churchyard", lanternGoal(LychLantern));
    // Only before 9 pm, with the Okafors' porch still lit (after it, the lantern is ready and the loop above names it).
    if (!w.lit[OkaforPorch]) return to("Knock at the Okafors' before 9 pm: Grandpa Ike's lantern is theirs to ask", doorGoal(T::Okafor));
    if (!w.offered) {
        if (!nanaArrived(w)) return to("Meet Nana at the stones on Lantern Hill (about " + minuteText(nanaArrives()) + " pm)", hill);
        return to("Make the offering with Nana in the middle of the stones", hill);
    }
    if (w.smoke == SmokeState::AtCircle) return {"Wait with Nana and Smoke for midnight"};
    if (w.smoke == SmokeState::Home) return {"Smoke is home. The night carries on: walk home when you like"};
    return doorsStep(w);
}

std::string tonightNext(const World& w) { return tonightStep(w).text; }

std::string tonightAlso(const World& w) {
    if (w.form != Form::Girl || w.bagDown || (canChooseCostume(w) && w.costume == Costume::None)) return "";
    const bool knocking = housesLeft(w) > 0 && static_cast<int>(gameMinutes(w)) < kPorchLightsOff;
    if (!knocking || w.lit[NanaWindow]) return "";
    if (tonightNext(w) == kNanaLanternNext) return "Then knock where a porch light is on, before 9 pm (" + std::to_string(housesLeft(w)) + " to go)";
    return "And light Nana's window lantern, left of her door";
}

std::string deedText(const World& w) {
    switch (w.deed) {
        case Deed::Lit:
            switch (w.deedArg) {
                case NanaWindow:
                    return "You light the lantern in Nana's window, for her mother, Ada.\n"
                           "Nana: \"Thank you, love. That one's always first.\"";
                case OkaforPorch:
                    return "You light Grandpa Ike's lantern on the Okafors' porch. Mrs. Okafor, at the window, puts her hand to the glass.";
                case HaleStep:
                    return "You light June Hale's lantern on the back step. A lamp comes on inside, and then, out front, the porch light.";
                default: return "You light the lantern in the lychgate, for everyone in the churchyard.";
            }
        case Deed::Apple: return w.apples >= kApplesWanted ? "You pick an apple. Two in the basket: one for the pot, one for the offering." : "You pick an apple.";
        case Deed::Rosemary: return "You pick a sprig of rosemary from Nana's herb bed: rosemary, for remembrance.";
        case Deed::Bought: return "Mr. Abbas: \"One cinnamon stick, for your Nana's cider? Tell her happy Samhain from me.\"";
        case Deed::BoughtJoke:
            return std::string("You: \"") + joke(w.jokes - 1) + "\"\nMr. Abbas: \"Ha! Keep your candy. The cinnamon is on the house.\"";
        case Deed::Stir:
            if (w.deedArg == 1) return "In go an apple, the rosemary and the cinnamon stick. Stir (E).";
            if (w.deedArg == 2) return "You remember Nana's words: \"Stir slowly, and think of the people it's for.\"";
            return "It smells like every autumn at once.";
        case Deed::Brewed:
            return "Nana's cider is ready. You fill the flask: cider and an apple, for the offering on Lantern Hill.";
        case Deed::Found:
            if (w.deedArg == 1) return "Smoke gets up and follows you again.";
            return "A small grey cat with a white chin sits under the oak, faintly glowing. Smoke! She stretches, and follows you.";
        case Deed::Waits: return "Smoke sits down and waits. Tonight she will only follow a cat.";
        case Deed::Circle: return "Smoke steps into the ring of stones and sits, her tail around her paws.";
        case Deed::Offered:
            return "Nana lights a candle, pours the cider and sets the apple on the stone. She says the names of her dead, one by one, and then: \"Smoke.\"\n"
                   "Nana: \"An apple and a cup of cider, love. Harm none: nothing that costs anyone anything.\"";
        case Deed::Home:
            return "The church clock strikes twelve. Smoke looks up at Nana, then walks out between the stones into the dark, and does not look back.\n"
                   "Nana: \"Goodnight, Smoke. Go on home.\"";
        case Deed::None: break;
    }
    return "";
}

bool craftTick(World& w, const Input& in) {
    smokeTick(w);
    if (!in.use && !in.trick) return false;
    const Act a = actionHere(w);
    if (in.trick && a == Act::Haggle) {
        ++w.jokes;
        w.cinnamon = true;
        did(w, Deed::BoughtJoke);
        return true;
    }
    if (!in.use) return false;
    switch (a) {
        case Act::Light: {
            const int l = lanternHere(w);
            w.lit[static_cast<size_t>(l)] = true;
            did(w, Deed::Lit, l);
            return true;
        }
        case Act::Stir:
            if (w.stirs == 0) {
                w.apples -= 1;
                w.rosemary = false;
                w.cinnamon = false;
            }
            ++w.stirs;
            if (w.stirs >= kStirs) {
                w.cider = true;
                did(w, Deed::Brewed);
            } else {
                did(w, Deed::Stir, w.stirs);
            }
            return true;
        case Act::Herb:
            w.rosemary = true;
            did(w, Deed::Rosemary);
            return true;
        case Act::Apple:
            ++w.apples;
            did(w, Deed::Apple);
            return true;
        case Act::Buy:
            w.candy -= kCinnamonPrice;
            w.cinnamon = true;
            did(w, Deed::Bought);
            return true;
        case Act::Offer:
            w.offered = true;
            w.apples -= 1;
            did(w, Deed::Offered);
            return true;
        case Act::Haggle:
        case Act::None: break;
    }
    return false;
}

Vec2 spotStand(int spot) {
    switch (spot) {
        case 0: return kLanterns[NanaWindow].stand;
        case 1: return kLanterns[OkaforPorch].stand;
        case 2: return {T::kHerbBed.x, T::kHerbBed.z + 1.4f};
        case 3: return {T::kFirePit.x, T::kFirePit.z + 1.5f};
        case 4:
            for (const T::Tree& t : T::trees())
                if (t.apple) return {t.at.x, t.at.z - 1.4f};
            break;
        case 5: return T::doorstep(T::Market);
        case 6: return {T::kHill.x, T::kHill.z + 1.5f};
        case 7: return {kSmokeStart.x, kSmokeStart.z + 2.6f};
        default: break;
    }
    return T::kStart;
}

Vec2 spotFace(int spot) {
    switch (spot) {
        case 0: return {kLanterns[NanaWindow].at.x, kLanterns[NanaWindow].at.z};
        case 1: return {kLanterns[OkaforPorch].at.x, kLanterns[OkaforPorch].at.z};
        case 2: return T::kHerbBed;
        case 3: return T::kFirePit;
        case 4: {
            const Vec2 s = spotStand(4);
            return {s.x, s.z + 1.4f};
        }
        case 5: return T::doorAt(T::Market);
        case 6: return T::kHill;
        case 7: return kSmokeStart;
        default: break;
    }
    return {T::kStart.x, T::kStart.z + 1};
}

}  // namespace familiar
