// familiar: the game on the Strata engine.
//
//   familiar [--seed N] [--width W --height H]
//   familiar --headless --tour --frames DIR [--width W --height H]
//   familiar --version
//
// Controls: mouse to look, W A S D to walk, Shift to hurry, E to knock or use
// (light, pick, buy, stir, offer), T to tell a joke, Q to turn into a cat and
// back, Tab for Wren's notebook, F1 for help, Esc to free the mouse (Esc again quits).
//
// --tour walks a fixed route through town into the night, as Wren and as
// the cat (over a fence, onto Nana's roof, into Mr. Hale's yard and the
// churchyard to open their gates, past the spirits); headless, it writes a
// screenshot at each stop to DIR.
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "bridge/scene.h"
#include "play/session.h"
#include "play/view.h"
#include "sim/cat.h"
#include "sim/craft.h"
#include "sim/doors.h"
#include "sim/notebook.h"
#include "sim/npc.h"
#include "sim/town.h"
#include "sim/world.h"
#include "strata/strata.hpp"

using namespace familiar;

namespace {

struct Args {
    uint64_t seed = 1;
    bool headless = false, tour = false;
    std::string framesDir;
    int width = 1280, height = 720;
};

bool parseArgs(int argc, char** argv, Args& a) {
    for (int i = 1; i < argc; ++i) {
        const std::string s = argv[i];
        auto next = [&](const char* what) -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "familiar: %s needs a value\n", what);
                std::exit(2);
            }
            return argv[++i];
        };
        if (s == "--seed") a.seed = std::strtoull(next("--seed"), nullptr, 10);
        else if (s == "--headless") a.headless = true;
        else if (s == "--tour") a.tour = true;
        else if (s == "--frames") a.framesDir = next("--frames");
        else if (s == "--width") a.width = std::atoi(next("--width"));
        else if (s == "--height") a.height = std::atoi(next("--height"));
        else if (s == "--version") {
            std::printf("familiar %s\n", FAMILIAR_VERSION);
            std::exit(0);
        } else {
            std::fprintf(stderr, "familiar: unknown argument '%s'\n", s.c_str());
            return false;
        }
    }
    if (a.headless && !a.tour) {
        std::fprintf(stderr, "familiar: --headless needs --tour\n");
        return false;
    }
    return true;
}

// ---- the tour: a fixed walk for screenshots and CI smoke runs
struct TourStop {
    float x, z;          // walk here (as Wren, or as the cat when `cat`)
    float yaw, pitch;    // then look this way
    int waitSeconds;     // and wait
    bool knock;          // knock (E) at the door here
    bool cat;            // be a cat for this stop
    const char* shot;    // screenshot file name, or null
    bool trick = false;  // tell a joke after knocking
    int costume = 0;     // choose this costume (1-4) first
    int spirit = -1;     // >= 0: walk up to this spirit (sim/cat.h) instead of (x, z), and look at it
    bool pickup = false; // E here picks up the candy bag (checked)
    int uses = 0;        // then press E this many times (light, pick, buy, stir, offer)
    int untilMinute = 0; // and wait until this minute of the night first (kOffering: Nana at the stones)
};

const int kOffering = 1;  // stand-in for nanaArrivalMinute(), which is not a constant

// The street at dusk; the cat over a fence, onto Nana's roof, and in to open
// Mr. Hale's latch and the lychgate's bolt (M3); Samhain kept (M4): all four
// lanterns lit, apples from the orchard, cinnamon from the market, rosemary
// from the herb bed, the cider brewed and carried up the hill; the cat
// leaves the bag at the stones, finds Smoke and leads her there; Wren takes
// up the bag again, makes the offering with Nana, and Smoke goes home at
// midnight.
const TourStop kTour[] = {
    {-66.0f, -7.2f, 2.6f, -0.02f, 0, false, false, "01-nanas-porch", false, 4},
    {-68.2f, -9.9f, -0.43f, -0.15f, 0, false, false, "02-nanas-window-lantern", false, 0, -1, false, 1},
    {-66.0f, -7.2f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-60.0f, 0.0f, 1.57f, 0.0f, 0, false, false, "03-elm-street"},
    {-48.0f, -6.6f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-48.0f, -9.6f, 0.0f, 0.05f, 1, true, false, "04-knock-okafors", true},
    {-49.9f, -9.6f, -2.3f, -0.75f, 0, false, false, "05-grandpa-ikes-lantern", false, 0, -1, false, 1},
    {-48.0f, -6.6f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-48.0f, -3.0f, 1.9f, -0.05f, 240, false, false, "06-dusk-street"},
    // The cat (M3): the bag stays on the sidewalk here; over the fence, up onto Nana's roof.
    {-45.0f, -3.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {-45.0f, -8.0f, 3.14f, 0.1f, 0, false, true, "07-cat-over-fence"},
    {-56.5f, -18.6f, -1.57f, 0.0f, 0, false, true, nullptr},
    {-59.3f, -18.6f, -1.57f, 0.0f, 0, false, true, nullptr},
    {-63.0f, -18.6f, -1.57f, 0.0f, 0, false, true, nullptr},
    {-63.0f, -15.6f, 1.57f, -0.15f, 300, false, true, "08-cat-on-nanas-roof"},
    {-63.0f, -23.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    // Behind the houses to Mr. Hale's woodpile, over his fence, and the latch from inside.
    {-63.0f, -34.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {-22.0f, -34.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {-22.0f, -26.8f, 0.0f, 0.0f, 0, false, true, nullptr},
    {-23.7f, -26.8f, 0.0f, 0.0f, 0, false, true, nullptr},
    {-28.0f, -26.8f, 0.0f, 0.0f, 0, false, true, nullptr},
    {-34.9f, -26.0f, -1.57f, -0.05f, 1, true, true, "09-cat-hale-latch"},
    // Wren again, out through the open gate, back to the bag, and back in with the jar for June's lantern.
    {-38.0f, -26.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-41.0f, -26.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-41.0f, -3.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-48.0f, -3.0f, 1.57f, 0.3f, 0, true, false, nullptr, false, 0, -1, true},
    {-41.0f, -3.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-41.0f, -26.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-34.0f, -26.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-30.0f, -24.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-30.0f, -21.6f, 0.0f, -0.6f, 0, false, false, "10-junes-lantern", false, 0, -1, false, 1},
    {-34.0f, -26.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-41.0f, -26.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-41.0f, -3.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {20.0f, -2.5f, 0.0f, 0.0f, 0, false, false, nullptr},
    {20.0f, -6.0f, 0.0f, 0.12f, 0, false, false, "11-churchyard-lychgate"},
    // The cat through the hedge gap, the spirits in the churchyard, the bolt.
    {44.0f, -6.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {44.0f, -40.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {40.0f, -40.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {0.0f, 0.0f, 0.0f, 0.12f, 2, false, true, "12-cat-churchyard-spirits", false, 0, 1},
    {23.5f, -31.0f, 3.14f, 0.0f, 0, true, true, nullptr},
    {23.5f, -28.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {20.0f, -6.0f, 0.0f, 0.3f, 0, true, false, nullptr, false, 0, -1, true},
    {23.5f, -28.8f, 0.0f, 0.25f, 0, false, false, "13-lychgate-lantern", false, 0, -1, false, 1},
    {20.0f, -6.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {56.0f, 0.0f, 0.9f, 0.08f, 0, false, false, "14-bridge-night"},
    {75.0f, -55.0f, -1.9f, -0.1f, 0, false, false, "15-lantern-hill"},
    {72.0f, -44.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {0.0f, 0.0f, 0.0f, 0.38f, 1, false, true, "16-cat-lamplighter", false, 0, 3},
    {75.0f, -55.0f, 0.0f, 0.3f, 0, true, false, nullptr, false, 0, -1, true},
    // Apples from the orchard.
    {61.0f, -3.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {65.0f, 8.6f, 3.14f, 0.2f, 0, false, false, nullptr, false, 0, -1, false, 1},
    {70.0f, 8.6f, 3.14f, 0.2f, 0, false, false, "17-orchard-apples", false, 0, -1, false, 1},
    // Back over the bridge to Abbas's Market for a cinnamon stick.
    {60.0f, 0.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {45.0f, 0.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {44.0f, -2.5f, 0.0f, 0.0f, 0, false, false, nullptr},
    {20.0f, 9.8f, 3.14f, 0.0f, 0, false, false, nullptr, false, 0, -1, false, 2},
    // Rosemary from Nana's herb bed, then the cider at the iron pot.
    {20.0f, -2.5f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-55.5f, -2.5f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-55.5f, -23.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-62.5f, -22.8f, -2.5f, -0.4f, 0, false, false, nullptr, false, 0, -1, false, 1},
    {-68.0f, -24.0f, 3.14f, -0.35f, 0, false, false, "18-nanas-cider", false, 0, -1, false, 4},
    // The cider up the hill to the stones.
    {-55.5f, -23.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-55.5f, -2.5f, 0.0f, 0.0f, 0, false, false, nullptr},
    {44.0f, -2.5f, 0.0f, 0.0f, 0, false, false, nullptr},
    {45.0f, 0.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {60.0f, 0.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {61.0f, -3.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {75.0f, -55.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {73.9f, -56.9f, 0.0f, 0.0f, 0, false, false, nullptr},  // between two stones
    {80.4f, -59.9f, 0.0f, 0.0f, 0, false, false, nullptr},
    // The cat leaves the bag at the stones, finds Smoke under the old oak, and leads her back.
    {73.9f, -56.9f, 0.0f, 0.0f, 0, false, true, nullptr},
    {75.0f, -55.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {65.5f, -19.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {58.6f, -19.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {59.2f, -23.6f, 0.5f, -0.1f, 0, false, true, "19-smoke-under-the-oak"},
    {58.6f, -19.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {65.5f, -19.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {75.0f, -55.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {73.9f, -56.9f, 0.0f, 0.0f, 0, false, true, nullptr},
    {80.4f, -59.9f, 0.0f, 0.0f, 0, false, true, nullptr},
    // Wren again beside her bag, with Nana at the stones: the offering, then midnight.
    {80.4f, -59.9f, -0.45f, -0.35f, 0, false, false, "20-the-offering", false, 0, -1, true, 1, kOffering},
    {80.4f, -59.9f, 0.65f, 0.0f, 0, false, false, "21-midnight-smoke-goes-home", false, 0, -1, false, 0, kMidnight + 1},
};

int runTour(strata::Engine& engine, const Args& args) {
    World w = newWorld(args.seed);
    bridge::Scene scene(engine);
    for (const TourStop& stop : kTour) {
        const Form want = stop.cat ? Form::Cat : Form::Girl;
        if (w.form != want) {
            Input in;
            in.yaw = w.player.yaw;
            in.pitch = w.player.pitch;
            in.toggleForm = true;
            tick(w, in);
            if (w.form != want) {
                std::fprintf(stderr, "familiar: the tour could not change form at (%.2f, %.2f)\n", w.player.x, w.player.z);
                return 1;
            }
        }
        if (stop.costume) {
            Input in;
            in.yaw = w.player.yaw;
            in.costume = stop.costume;
            tick(w, in);
        }
        bool arrived = false;
        float yawTo = stop.yaw;
        for (int t = 0; t < 200 * kTickRate && !arrived; ++t) {
            float tx = stop.x, tz = stop.z, near = 0.04f;
            if (stop.spirit >= 0) {
                const Walker s = spirits(w)[static_cast<size_t>(stop.spirit)];
                tx = s.at.x;
                tz = s.at.z;
                near = 25.0f;  // five metres off
            }
            const float dx = tx - w.player.x, dz = tz - w.player.z;
            arrived = dx * dx + dz * dz < near;
            if (arrived && stop.spirit >= 0) yawTo = std::atan2(dx, -dz);
            if (arrived) break;
            Input in;
            in.forward = 1;
            in.hurry = dx * dx + dz * dz > 25.0f;
            in.yaw = std::atan2(dx, -dz);
            in.pitch = 0;
            tick(w, in);
        }
        if (!arrived) {
            std::fprintf(stderr, "familiar: the tour got stuck at (%.2f, %.2f) on the way to (%.2f, %.2f)%s\n", w.player.x,
                         w.player.z, stop.x, stop.z, stop.spirit >= 0 ? " (a spirit)" : "");
            return 1;
        }
        const float yaw0 = w.player.yaw;
        float dyaw = yawTo - yaw0;
        while (dyaw > kPi) dyaw -= 2 * kPi;
        while (dyaw < -kPi) dyaw += 2 * kPi;
        const int turn = kTickRate, wait = stop.waitSeconds * kTickRate;
        for (int t = 0; t < turn + wait; ++t) {
            const float k = t < turn ? static_cast<float>(t + 1) / turn : 1.0f;
            Input in;
            in.yaw = yaw0 + dyaw * k;
            in.pitch = stop.pitch * k;
            in.use = stop.knock && t == turn - 1;
            tick(w, in);
        }
        if (stop.pickup && w.bagDown) {
            std::fprintf(stderr, "familiar: the tour could not pick up the candy bag at (%.2f, %.2f)\n", w.player.x, w.player.z);
            return 1;
        }
        if (stop.trick) {
            Input in;
            in.yaw = w.player.yaw;
            in.pitch = w.player.pitch;
            in.trick = true;
            tick(w, in);
        }
        const int until = stop.untilMinute == kOffering ? nanaArrivalMinute() : stop.untilMinute;
        while (until > 0 && gameMinutes(w) < until) {
            Input in;
            in.yaw = w.player.yaw;
            in.pitch = w.player.pitch;
            tick(w, in);
        }
        for (int u = 0; u < stop.uses; ++u) {
            Input in;
            in.yaw = w.player.yaw;
            in.pitch = w.player.pitch;
            in.use = true;
            tick(w, in);
        }
        for (int t = 0; t < kTickRate / 2 && stop.uses > 0; ++t) {  // a moment to see what happened
            Input in;
            in.yaw = w.player.yaw;
            in.pitch = w.player.pitch;
            tick(w, in);
        }
        if (!stop.shot) continue;
        scene.update(w);
        std::string said = !deedText(w).empty() && w.deedTick > w.answeredTick ? deedText(w) : answerText(w);
        if (said.empty() && spiritNear(w) >= 0) said = spiritText(spiritNear(w));
        play::render(engine, w, false, eyePosition(w).y, false, said);
        engine.present();
        const float elevation = play::setSky(engine, w);
        std::printf("%-30s (%7.2f, %7.2f) feet %5.2f at %s, sun %5.1f deg, candy %d%s, spirits met %d, lanterns %d, basket %s, Smoke %s\n",
                    stop.shot, w.player.x, w.player.z, w.player.feet, clockText(w).c_str(), elevation, w.candy,
                    w.bagDown ? " (bag down)" : "", spiritsSeen(w), lanternsLit(w), basketLine(w).c_str(), smokeLine(w).c_str());
        if (!args.framesDir.empty()) engine.saveScreenshotPng(args.framesDir + "/" + stop.shot + ".png");
    }
    std::printf("%zu meshes, fingerprint %016" PRIx64 "\n", scene.meshCount(), fingerprint(w));
    if (w.costume != Costume::Witch || w.jokes < 1 || !w.knocked[town::Okafor]) {
        std::fprintf(stderr, "familiar: the tour did not dress up, knock and tell a joke\n");
        return 1;
    }
    if (lanternsLit(w) != kLanternCount || !w.cider || !w.offered || w.smoke != SmokeState::Home) {
        std::fprintf(stderr, "familiar: the tour did not light the four lanterns, brew the cider, make the offering and see Smoke home\n");
        return 1;
    }
    std::printf("notebook: %d pages, %d secrets\n", pagesFound(w), secretsFound(w));
    if (!w.gateOpen[town::HaleGate] || !w.gateOpen[town::Lychgate]) {
        std::fprintf(stderr, "familiar: the tour's cat did not open Mr. Hale's latch and the lychgate's bolt\n");
        return 1;
    }
    const uint32_t met = (1u << Farmer) | (1u << Lamplighter);
    if ((w.spiritsSeen & met) != met) {
        std::fprintf(stderr, "familiar: the tour's cat did not meet the farmer and the lamplighter\n");
        return 1;
    }
    return 0;
}

int runGame(strata::Engine& engine, const Args& args) {
    play::SessionOptions o;
    o.seed = args.seed;
    play::Session session(engine, o);
    while (session.running()) {
        StrataEvent e;
        while (engine.pollEvent(e)) session.handle(e);
        session.frame(engine.frameSeconds());
        engine.present();
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    Args args;
    if (!parseArgs(argc, argv, args)) {
        std::puts("usage: familiar [--seed N] [--width W --height H]\n"
                  "       familiar --headless --tour --frames DIR [--width W --height H]\n"
                  "       familiar --version");
        return 2;
    }
    try {
        strata::EngineDesc desc;
        desc.surface = args.headless ? STRATA_SURFACE_HEADLESS : STRATA_SURFACE_WINDOW;
        desc.width = args.width;
        desc.height = args.height;
        desc.flags = args.headless ? 0u : static_cast<uint32_t>(STRATA_FLAG_VSYNC);
        desc.title = "Familiar";
        auto engine = strata::Engine::create(desc);
        play::setLighting(*engine);
        if (args.tour) return runTour(*engine, args);
        return runGame(*engine, args);
    } catch (const strata::Error& e) {
        std::fprintf(stderr, "familiar: %s\n", e.what());
        return 1;
    }
}
