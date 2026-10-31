// Witchcraft (M4): the four family lanterns (Mr. Hale's and the lychgate's
// only once a gate is open), Nana's cider from two apples, rosemary and a
// cinnamon stick bought with candy (or a joke, when short), Smoke found by the
// cat and led to the stones, the offering with Nana, and Smoke going home at
// midnight only with everything done. Then the notebook: pages earned as the
// night goes, secrets found where they are, every page about belief or history
// citing a source that exists in RESEARCH.md, and nothing but ASCII.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string>

#include "check.h"
#include "sim/cat.h"
#include "sim/craft.h"
#include "sim/doors.h"
#include "sim/notebook.h"
#include "sim/npc.h"
#include "sim/world.h"

using namespace familiar;
namespace T = familiar::town;

namespace {

void standAt(World& w, Vec2 p) {
    w.player.x = p.x;
    w.player.z = p.z;
    float feet = 0;
    if (familiar::standAt(w, w.form, p.x, p.z, 10.0f, feet)) w.player.feet = feet;
}

void press(World& w, bool use, bool trick = false, bool toggle = false) {
    Input in;
    in.yaw = w.player.yaw;
    in.use = use;
    in.trick = trick;
    in.toggleForm = toggle;
    tick(w, in);
}

void idle(World& w, int ticks) {
    for (int i = 0; i < ticks; ++i) press(w, false);
}

int64_t tickAt(int minute) { return static_cast<int64_t>((minute - kStartMinute) / kGameMinutesPerSecond * kTickRate) + 1; }

std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

}  // namespace

int main(int, char**) {
    // ---- Nana's window lantern: E at the window, as Wren; the Samhain page with it.
    {
        World w = newWorld(1);
        CHECK(lanternsLit(w) == 0 && w.pages == 0);
        standAt(w, lantern(NanaWindow).stand);
        CHECK(lanternHere(w) == NanaWindow);
        CHECK(craftPrompt(w).find("E  light Nana's window lantern") == 0);
        CHECK(craftWord(w) == "Light");
        press(w, true);
        CHECK(w.lit[NanaWindow] && w.deed == Deed::Lit && w.deedArg == NanaWindow);
        CHECK(deedText(w).find("Ada") != std::string::npos);
        CHECK(w.pages & (1u << SamhainPage));
        CHECK(lanternHere(w) == -1);  // lit, nothing more to do there

        // A cat cannot carry the jar, and nor can Wren while it lies in her bag where she changed.
        World c = newWorld(1);
        press(c, false, false, true);
        CHECK(c.form == Form::Cat);
        standAt(c, lantern(NanaWindow).stand);
        CHECK(craftPrompt(c).find("A cat can't carry") == 0);
        press(c, true);
        CHECK(!c.lit[NanaWindow]);
        press(c, false, false, true);
        CHECK(c.form == Form::Girl && c.bagDown);
        CHECK(craftPrompt(c).find("in your bag") != std::string::npos);
        press(c, true);
        CHECK(!c.lit[NanaWindow]);
        CHECK(c.pages & (1u << CatsPage));  // the black cats page, on first becoming one
    }

    // ---- 0.1.0 playtest (#12, #13): lanterns and doors in reach from a step
    // away, Nana out of the way of her window, and "come closer" straight ahead.
    {
        World w = newWorld(1);
        const Vec3 a = lantern(NanaWindow).at;
        standAt(w, {-67.2f, -9.2f});  // where Nana stood in 0.1.0
        CHECK(lanternHere(w) == NanaWindow);
        standAt(w, {a.x + 1.0f, a.z + 1.9f});  // about 2.1 m off, on the porch
        CHECK(lanternHere(w) == NanaWindow);
        const Walker n = nana(w);
        CHECK((n.at.x - a.x) * (n.at.x - a.x) + (n.at.z - a.z) * (n.at.z - a.z) > 3.0f * 3.0f);
        CHECK(n.at.x > T::doorAt(T::Nana).x);  // to the right of her door, the window is to the left
        // On Nana's doorstep with the lantern unlit (and the costume line gone), her door points at it.
        w.candy = 1;
        standAt(w, T::doorstep(T::Nana));
        CHECK(usePrompt(w).find("window lantern") != std::string::npos);
        // A door answers from a step back off the doorstep.
        World d = newWorld(1);
        standAt(d, {T::doorstep(T::Okafor).x, T::doorstep(T::Okafor).z + 1.75f});
        CHECK(atDoor(d) == T::Okafor);
        press(d, true);
        CHECK(d.asking == T::Okafor);
        // Out of reach, looking straight at it: come closer.
        World h = newWorld(1);
        standAt(h, {-66.0f, -4.0f});
        CHECK(lanternHere(h) == -1);
        h.player.yaw = std::atan2(a.x - h.player.x, -(a.z - h.player.z));
        CHECK(reachHint(h).find("Nana's window lantern: come closer") == 0);
        h.player.yaw += kPi;  // looking away
        CHECK(reachHint(h).empty());
        standAt(h, {-48.0f, -4.0f});
        h.player.yaw = 0;  // north, at the Okafors' door
        CHECK(reachHint(h).find("The Okafors' door: come closer to knock") == 0);
        h.lit[NanaWindow] = true;
        h.knocked[T::Okafor] = true;
        CHECK(reachHint(h).empty());  // been there (and Grandpa Ike's lantern is off to the left)
    }

    // ---- Grandpa Ike's lantern: only once the Okafors have been knocked at.
    {
        World w = newWorld(1);
        standAt(w, lantern(OkaforPorch).stand);
        CHECK(lanternHere(w) == OkaforPorch);
        CHECK(craftPrompt(w).find("Knock first") != std::string::npos);
        press(w, true);
        CHECK(!w.lit[OkaforPorch]);
        // Knock, tell a joke: Mrs. Okafor asks for the lantern.
        standAt(w, T::doorstep(T::Okafor));
        press(w, true);
        CHECK(w.asking == T::Okafor);
        press(w, false, true);
        CHECK(w.candy == 3 && answerText(w).find("lantern") != std::string::npos);
        CHECK(tonightNext(w).find("Okafors' porch") != std::string::npos ||
              tonightNext(w).find("Nana's window") != std::string::npos);
        CHECK(w.pages & (1u << GuisingPage));
        CHECK(w.pages & (1u << JackPage));
        standAt(w, lantern(OkaforPorch).stand);
        press(w, true);
        CHECK(w.lit[OkaforPorch]);
        CHECK(!(w.pages & (1u << LanternsPage)));  // one lantern so far
        standAt(w, lantern(NanaWindow).stand);
        press(w, true);
        CHECK(w.pages & (1u << LanternsPage));
    }

    // ---- Mr. Hale's and the lychgate's: shut gates first; June's lantern lights his porch however late.
    {
        World w = newWorld(1);
        w.tick = tickAt(23 * 60 + 10);
        standAt(w, lantern(HaleStep).stand);
        CHECK(lanternHere(w) == HaleStep);
        press(w, true);
        CHECK(!w.lit[HaleStep]);  // the back gate is latched from inside
        w.gateOpen[T::HaleGate] = true;
        press(w, true);
        CHECK(w.lit[HaleStep] && porchLightOn(w, T::Hale));
        standAt(w, T::doorstep(T::Hale));
        press(w, true);
        CHECK(w.candy == 2 && w.knocked[T::Hale] && answerText(w).find("June") != std::string::npos);

        standAt(w, lantern(LychLantern).stand);
        CHECK(craftPrompt(w).find("bolted") != std::string::npos);
        press(w, true);
        CHECK(!w.lit[LychLantern]);
        w.gateOpen[T::Lychgate] = true;
        press(w, true);
        CHECK(w.lit[LychLantern]);
    }

    // ---- The cider: two apples, rosemary, and cinnamon from the market, stirred four times.
    {
        World w = newWorld(1);
        standAt(w, spotStand(3));
        CHECK(atPot(w));
        CHECK(craftPrompt(w).find("rosemary") != std::string::npos);
        press(w, true);
        CHECK(w.stirs == 0 && !w.cider);

        standAt(w, spotStand(2));
        CHECK(craftWord(w) == "Pick");
        press(w, true);
        CHECK(w.rosemary && w.deed == Deed::Rosemary);

        standAt(w, spotStand(4));
        CHECK(atAppleTree(w));
        press(w, true);
        press(w, true);
        press(w, true);
        CHECK(w.apples == kApplesWanted);  // the basket holds two

        // The market: the first E knocks (one candy), the next buys, at three.
        standAt(w, spotStand(5));
        CHECK(atMarket(w) && !shopOpen(w));
        press(w, true);
        CHECK(w.candy == 1 && w.knocked[T::Market] && !w.cinnamon);
        CHECK(haggling(w));  // one candy is not three: a joke will do
        CHECK(craftPrompt(w).find("T  offer Mr. Abbas a joke") != std::string::npos);
        World rich = w;
        rich.candy = 5;
        CHECK(craftWord(rich) == "Buy");
        press(rich, true);
        CHECK(rich.cinnamon && rich.candy == 2 && rich.deed == Deed::Bought);
        press(w, true);
        CHECK(!w.cinnamon);  // E is not a joke
        const int jokes = w.jokes;
        press(w, false, true);
        CHECK(w.cinnamon && w.candy == 1 && w.jokes == jokes + 1 && w.deed == Deed::BoughtJoke);
        press(w, false, true);
        CHECK(w.jokes == jokes + 1);  // one stick is enough

        standAt(w, spotStand(3));
        CHECK(craftWord(w) == "Stir");
        for (int i = 0; i < kStirs; ++i) press(w, true);
        CHECK(w.cider && w.apples == 1 && !w.rosemary && !w.cinnamon && w.deed == Deed::Brewed);
        CHECK(w.pages & (1u << WheelPage));
        CHECK(basketLine(w).find("Nana's cider") != std::string::npos);
        press(w, true);
        CHECK(w.apples == 1);  // the pot is done
    }

    // ---- Smoke: only a cat finds her; she follows the cat, waits for Wren, sits in the circle.
    {
        World w = newWorld(1);
        CHECK(w.smoke == SmokeState::Lost);
        standAt(w, spotStand(7));
        idle(w, 2);
        CHECK(w.smoke == SmokeState::Lost);  // Wren cannot see her
        Vec3 at;
        float yaw = 0;
        CHECK(!smokeShown(w, at, yaw));
        press(w, false, false, true);
        w.player.x = kSmokeStart.x;
        w.player.z = kSmokeStart.z + 1.5f;
        idle(w, 1);
        CHECK(w.smoke == SmokeState::Following && w.deed == Deed::Found);
        CHECK(smokeShown(w, at, yaw));
        // She keeps up with the cat.
        w.player.x += 8;
        idle(w, kTickRate * 3);
        const float dx = w.player.x - w.smokeX, dz = w.player.z - w.smokeZ;
        CHECK(dx * dx + dz * dz < 1.3f * 1.3f);
        // Wren stops being a cat: Smoke waits; the cat comes back: she follows again.
        press(w, false, false, true);
        CHECK(w.form == Form::Girl && w.smoke == SmokeState::Waiting);
        const float sx = w.smokeX;
        w.player.x += 4;
        idle(w, kTickRate);
        CHECK(w.smokeX == sx);
        press(w, false, false, true);
        w.player.x = w.smokeX + 1;
        idle(w, 1);
        CHECK(w.smoke == SmokeState::Following && w.deedArg == 1);
        // Into the ring of stones.
        w.player.x = T::kHill.x;
        w.player.z = T::kHill.z;
        idle(w, kTickRate * 20);
        CHECK(w.smoke == SmokeState::AtCircle);
        w.player.x = 0;
        idle(w, kTickRate * 3);
        CHECK(w.smoke == SmokeState::AtCircle);  // she stays

        // The offering: Nana's cider and an apple, once Nana is at the stones.
        press(w, false, false, true);
        CHECK(w.form == Form::Girl);
        standAt(w, spotStand(6));
        CHECK(w.bagDown && !carrying(w));  // the jar and basket went down with the bag (M3)
        w.cider = true;
        w.apples = 1;
        w.tick = tickAt(nanaArrivalMinute());
        CHECK(craftWord(w).empty());
        w.tick = 0;
        w.bagDown = false;  // she fetches it
        w.cider = true;
        w.apples = 1;
        CHECK(!nanaArrived(w));
        CHECK(craftPrompt(w).find("Nana meets you here") == 0);
        press(w, true);
        CHECK(!w.offered);
        w.tick = tickAt(nanaArrivalMinute());
        CHECK(nanaArrived(w));
        CHECK(craftWord(w) == "Offer");
        press(w, true);
        CHECK(w.offered && w.apples == 0 && w.deed == Deed::Offered);
        CHECK(w.pages & (1u << RedePage));
        CHECK(smokeShown(w, at, yaw));  // at the stones, with the offering made, Wren sees her too

        // Midnight: not without every lantern.
        World most = w;
        most.lit = {true, true, true, false};
        most.tick = tickAt(kMidnight);
        idle(most, 2);
        CHECK(most.smoke == SmokeState::AtCircle);
        // With them all, not before midnight, then home at the stroke.
        w.lit = {true, true, true, true};
        w.tick = tickAt(kMidnight) - 40;
        idle(w, 2);
        CHECK(w.smoke == SmokeState::AtCircle);
        idle(w, 60);
        CHECK(w.smoke == SmokeState::Home && w.deed == Deed::Home && w.homeTick > 0);
        CHECK(smokeShown(w, at, yaw));
        idle(w, kTickRate * 15);
        CHECK(!smokeShown(w, at, yaw));  // gone into the dark
        CHECK(tonightNext(w).find("Smoke is home") == 0);
    }

    // ---- Nana: on her porch, then on the road, then waiting at the stones.
    {
        World w = newWorld(1);
        CHECK(nana(w).at.x < -60);
        w.tick = tickAt(kNanaLeaves + 20);
        const Walker mid = nana(w);
        CHECK(mid.at.x > -60 && mid.at.x < 75);
        w.tick = tickAt(nanaArrivalMinute());
        const Walker top = nana(w);
        const float dx = top.at.x - T::kHill.x, dz = top.at.z - T::kHill.z;
        CHECK(dx * dx + dz * dz < T::kCircleRadius * T::kCircleRadius);
        CHECK(top.at.y > T::kHillHeight - 1);
        CHECK(nanaArrivalMinute() > kNanaLeaves && nanaArrivalMinute() < 23 * 60 + 45);
        // On the bridge she walks on the deck, not in the creek.
        for (int m = kNanaLeaves; m < nanaArrivalMinute(); ++m) {
            w.tick = tickAt(m);
            const Walker n = nana(w);
            if (T::kBridge.contains(n.at.x, n.at.z)) CHECK(n.at.y >= T::kBridgeDeck - 0.05f);
            CHECK(n.at.y > T::kWaterY);
        }
    }

    // ---- The NEXT line walks through the night.
    {
        World w = newWorld(1);
        w.costume = Costume::Ghost;
        standAt(w, {-60.0f, 0.0f});
        CHECK(tonightNext(w).find("Nana's window") != std::string::npos);
        w.lit[NanaWindow] = true;
        w.tick = tickAt(21 * 60 + 5);
        CHECK(tonightNext(w).find("Light") == 0 || tonightNext(w).find("For Nana's cider") == 0);
        w.knocked[T::Okafor] = true;
        w.lit[OkaforPorch] = true;
        CHECK(tonightNext(w).find("For Nana's cider") == 0);
        w.cider = true;
        CHECK(tonightNext(w).find("Find Smoke") == 0);
        w.smoke = SmokeState::AtCircle;
        CHECK(tonightNext(w).find("June Hale's lantern") == 0);
    }

    // ---- The night can always be finished (softlock fix). A player who never knocks at the
    // Okafors' (or anywhere) before the porch lights go off at 9 pm can still light all four
    // lanterns, brew the cider, make the offering, and see Smoke home at midnight; and the
    // NEXT line never points at a step that can no longer be done.
    {
        World w = newWorld(1);
        w.costume = Costume::Ghost;
        // Before 9 pm, without a knock: Grandpa Ike's lantern waits for the Okafors.
        w.tick = tickAt(20 * 60 + 50);
        standAt(w, lantern(OkaforPorch).stand);
        press(w, true);
        CHECK(!w.lit[OkaforPorch]);
        CHECK(craftPrompt(w).find("Knock first") != std::string::npos);
        // Past 9 pm, never having knocked: the porch is dark, and the lantern can be lit anyway.
        w.tick = tickAt(kPorchLightsOff + 1);
        CHECK(!porchLightOn(w, T::Okafor) && !w.knocked[T::Okafor]);
        CHECK(craftWord(w) == "Light");
        press(w, true);
        CHECK(w.lit[OkaforPorch] && w.deed == Deed::Lit && w.deedArg == OkaforPorch);
        CHECK(w.candy == 0);  // nothing was knocked for

        standAt(w, lantern(NanaWindow).stand);
        press(w, true);
        CHECK(w.lit[NanaWindow]);

        // The gates, opened by the cat from inside with the real latches.
        press(w, false, false, true);
        CHECK(w.form == Form::Cat);
        w.player.x = T::kHaleYard.x0 + 0.6f;
        w.player.z = T::kHaleGateZ;
        CHECK(latchHere(w) == T::HaleGate);
        press(w, true);
        CHECK(w.gateOpen[T::HaleGate]);
        w.player.x = (T::kLychgateX0 + T::kLychgateX1) / 2;
        w.player.z = T::kChurchyard.z1 - 0.8f;
        CHECK(latchHere(w) == T::Lychgate);
        press(w, true);
        CHECK(w.gateOpen[T::Lychgate]);
        // Back to the bag, and Wren again.
        w.player.x = w.bag.x;
        w.player.z = w.bag.z;
        w.player.feet = w.bag.y;
        press(w, false, false, true);
        CHECK(w.form == Form::Girl && !w.bagDown);
        standAt(w, lantern(HaleStep).stand);
        press(w, true);
        standAt(w, lantern(LychLantern).stand);
        press(w, true);
        CHECK(lanternsLit(w) == kLanternCount);

        // The cider after the market's porch light is out (11 pm), with no candy: a joke buys the cinnamon.
        w.tick = tickAt(23 * 60 + 1);
        standAt(w, spotStand(2));
        press(w, true);
        standAt(w, spotStand(4));
        press(w, true);
        press(w, true);
        standAt(w, spotStand(5));
        CHECK(!w.knocked[T::Market] && shopOpen(w) && haggling(w));
        press(w, false, true);
        CHECK(w.cinnamon);
        standAt(w, spotStand(3));
        for (int i = 0; i < kStirs; ++i) press(w, true);
        CHECK(w.cider && w.apples == 1);

        // Smoke, found by the cat and led into the stones.
        press(w, false, false, true);
        w.player.x = kSmokeStart.x;
        w.player.z = kSmokeStart.z + 1.5f;
        idle(w, 1);
        CHECK(w.smoke == SmokeState::Following);
        w.player.x = T::kHill.x;
        w.player.z = T::kHill.z;
        idle(w, kTickRate * 20);
        CHECK(w.smoke == SmokeState::AtCircle);
        w.form = Form::Girl;  // (the bag walk is covered above)
        w.bagDown = false;

        // The offering once Nana is there, then midnight: Smoke goes home.
        standAt(w, spotStand(6));
        if (gameMinutes(w) < nanaArrivalMinute()) w.tick = tickAt(nanaArrivalMinute());
        CHECK(tonightNext(w).find("Make the offering") == 0);
        press(w, true);
        CHECK(w.offered);
        CHECK(tonightNext(w).find("Wait with Nana and Smoke for midnight") == 0);
        w.tick = tickAt(kMidnight);
        idle(w, 2);
        CHECK(w.smoke == SmokeState::Home && w.deed == Deed::Home);
    }

    // ---- The old softlock's HUD: everything else done after 9 pm, Grandpa Ike's lantern never lit.
    // NEXT used to say "Wait with Nana and Smoke for midnight" though midnight would change nothing.
    {
        World w = newWorld(1);
        w.costume = Costume::Ghost;
        w.tick = tickAt(23 * 60 + 40);
        w.lit = {true, false, true, true};
        w.cider = true;
        w.offered = true;
        w.smoke = SmokeState::AtCircle;
        standAt(w, spotStand(6));
        CHECK(tonightNext(w).find("Light Grandpa Ike's lantern") == 0);
        World late = w;
        late.tick = tickAt(kMidnight + 10);
        idle(late, 2);
        CHECK(late.smoke == SmokeState::AtCircle);  // not home without it...
        standAt(late, lantern(OkaforPorch).stand);
        press(late, true);
        CHECK(late.lit[OkaforPorch]);
        idle(late, 2);
        CHECK(late.smoke == SmokeState::Home);  // ...and home as soon as it is lit, after midnight
        // Before 9 pm the NEXT line sends Wren to knock, never to wait.
        World early = w;
        early.tick = tickAt(20 * 60);
        for (int i = 0; i < T::HouseCount; ++i) early.knocked[static_cast<size_t>(i)] = i != T::Okafor;
        CHECK(tonightNext(early).find("Wait") != 0);
        CHECK(tonightNext(early).find("Knock") != std::string::npos);  // one lit porch to go: the Okafors'
    }

    // ---- NEXT never says to wait for midnight with a lantern unlit, at any hour, in any mix of
    // lanterns, gates and knocks.
    {
        for (int minute : {19 * 60, 20 * 60 + 59, 21 * 60 + 1, 23 * 60 + 30, kMidnight + 5})
            for (int mask = 0; mask < 16; ++mask)
                for (int extra = 0; extra < 8; ++extra) {
                    World w = newWorld(1);
                    w.costume = Costume::Ghost;
                    w.tick = tickAt(minute);
                    for (int i = 0; i < kLanternCount; ++i) w.lit[static_cast<size_t>(i)] = (mask >> i) & 1;
                    w.knocked[T::Okafor] = extra & 1;
                    w.gateOpen[T::HaleGate] = (extra >> 1) & 1;
                    w.gateOpen[T::Lychgate] = (extra >> 2) & 1;
                    for (int i = 0; i < T::HouseCount; ++i)
                        if (i != T::Okafor) w.knocked[static_cast<size_t>(i)] = true;
                    w.cider = true;
                    w.offered = true;
                    w.smoke = SmokeState::AtCircle;
                    const std::string next = tonightNext(w);
                    if (lanternsLit(w) < kLanternCount) CHECK(next.find("Wait") != 0);
                    // A lantern the hint names as ready to light can be lit.
                    for (int i = 0; i < kLanternCount; ++i)
                        if (next == std::string("Light ") + lantern(i).name + ", " + lantern(i).forWhom) {
                            standAt(w, lantern(i).stand);
                            press(w, true);
                            CHECK(w.lit[static_cast<size_t>(i)]);
                        }
                }
    }

    // ---- Secrets: where they are, in the right form.
    {
        World w = newWorld(1);
        standAt(w, T::kSwapTree);
        w.player.x += 1.0f;
        idle(w, 1);
        CHECK(w.secrets & (1u << SwapTree));
        CHECK(secretsFound(w) == 1);
        // Nana's roof is a cat's secret.
        World r = newWorld(1);
        const Box2 f = T::footprint(T::Nana);
        r.player.x = (f.x0 + f.x1) / 2;
        r.player.z = (f.z0 + f.z1) / 2;
        r.player.feet = 5.0f;
        CHECK(!secretHere(r, RoofView));
        r.form = Form::Cat;
        CHECK(secretHere(r, RoofView));
        // The owl only after dark.
        World o = newWorld(1);
        standAt(o, {T::kOldOak.x, T::kOldOak.z + 2.5f});
        CHECK(!secretHere(o, OwlInOak));
        o.tick = tickAt(19 * 60);
        CHECK(secretHere(o, OwlInOak));
        // The lamplighter (M3's spirit on the hill path): a cat's secret.
        World l = newWorld(1);
        l.tick = tickAt(21 * 60);
        const Walker lamp = spirits(l)[Lamplighter];
        l.player.x = lamp.at.x;
        l.player.z = lamp.at.z + 1;
        CHECK(!secretHere(l, LamplighterSecret));
        l.form = Form::Cat;
        CHECK(secretHere(l, LamplighterSecret) == (spiritNear(l) == Lamplighter));
    }

    // ---- The notebook's writing: ASCII, and every page about belief or history cites RESEARCH.md.
    {
        const std::string research = readFile(std::string(FAMILIAR_SOURCE_DIR) + "/RESEARCH.md");
        CHECK(!research.empty());
        std::set<std::string> defined;
        const std::regex row(R"(\n\| (S\d+) \|)");
        for (auto it = std::sregex_iterator(research.begin(), research.end(), row); it != std::sregex_iterator(); ++it)
            defined.insert((*it)[1]);
        CHECK(defined.size() >= 30);
        const std::regex cite(R"(\[(S\d+)\])");
        auto ascii = [](const char* s) {
            for (const char* p = s; *p; ++p)
                if (!(*p == '\n' || (*p >= 32 && *p < 127))) return false;
            return true;
        };
        for (int i = 0; i < PageCount; ++i) {
            const Page& p = page(i);
            const std::string text = p.text;
            CHECK(p.aboutBelief);
            int cites = 0;
            for (auto it = std::sregex_iterator(text.begin(), text.end(), cite); it != std::sregex_iterator(); ++it) {
                ++cites;
                if (!defined.count((*it)[1])) {
                    std::fprintf(stderr, "page '%s' cites %s, which RESEARCH.md does not define\n", p.title, (*it)[1].str().c_str());
                    CHECK(false);
                }
            }
            CHECK(cites > 0);
            CHECK(ascii(p.title) && ascii(p.text));
        }
        for (int i = 0; i < SecretCount; ++i) CHECK(ascii(secret(i).title) && ascii(secret(i).text));
        // The fantasy is labelled as fantasy.
        CHECK(std::string(page(CatsPage).text).find("not something Wiccans believe or do") != std::string::npos);
        CHECK(std::string(page(SamhainPage).text).find("game's fairy tale") != std::string::npos);
        // Every message the night can show is ASCII too.
        World w = newWorld(1);
        for (int d = 0; d <= static_cast<int>(Deed::Home); ++d)
            for (int a = 0; a <= 4; ++a) {
                w.deed = static_cast<Deed>(d);
                w.deedArg = a;
                w.jokes = 1;
                CHECK(ascii(deedText(w).c_str()));
            }
    }

    // ---- Determinism: the same night twice, the same world.
    {
        auto run = [] {
            World w = newWorld(7);
            standAt(w, lantern(NanaWindow).stand);
            press(w, true);
            press(w, false, false, true);
            w.player.x = kSmokeStart.x;
            w.player.z = kSmokeStart.z + 1;
            for (int i = 0; i < 300; ++i) {
                Input in;
                in.forward = 1;
                in.yaw = 2.5f;
                tick(w, in);
            }
            return fingerprint(w);
        };
        CHECK(run() == run());
    }
    TEST_MAIN_END();
}
