// familiar: the game on the Strata engine.
//
//   familiar [--seed N] [--width W --height H]
//   familiar --headless --tour --frames DIR [--width W --height H]
//   familiar --version
//
// Controls: mouse to look, W A S D to walk, Shift to hurry, E to knock, Q to
// turn into a cat and back, F1 for help, Esc to free the mouse (Esc again quits).
//
// --tour walks a fixed route through town into the night; headless, it
// writes a screenshot at each stop to DIR.
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "bridge/scene.h"
#include "play/session.h"
#include "play/view.h"
#include "sim/doors.h"
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
};

const TourStop kTour[] = {
    {-66.0f, -7.2f, 2.6f, -0.02f, 0, false, false, "01-nanas-porch", false, 4},
    {-60.0f, 0.0f, 1.57f, 0.0f, 0, false, false, "02-elm-street"},
    {-48.0f, -6.6f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-48.0f, -9.6f, 0.0f, 0.05f, 1, true, false, "03-knock-okafors", true},
    {-48.0f, -3.0f, 1.9f, -0.05f, 240, false, false, "04-dusk-street"},
    {-45.0f, -3.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {-45.0f, -8.0f, 3.14f, 0.1f, 0, false, true, "05-cat-over-fence"},
    {-56.5f, -18.6f, -1.57f, 0.0f, 0, false, true, nullptr},
    {-59.3f, -18.6f, -1.57f, 0.0f, 0, false, true, nullptr},
    {-63.0f, -18.6f, -1.57f, 0.0f, 0, false, true, nullptr},
    {-63.0f, -15.6f, 1.57f, -0.15f, 600, false, true, "06-cat-on-nanas-roof"},
    {-63.0f, -23.0f, 0.0f, 0.0f, 0, false, true, nullptr},
    {-55.5f, -23.0f, 0.0f, 0.0f, 0, false, false, nullptr},
    {-55.5f, -2.5f, 0.0f, 0.0f, 0, false, false, nullptr},
    {20.0f, -2.5f, 0.0f, 0.0f, 0, false, false, nullptr},
    {20.0f, -6.0f, 0.0f, 0.12f, 0, false, false, "07-churchyard-lychgate"},
    {56.0f, 0.0f, 0.9f, 0.08f, 0, false, false, "08-bridge-night"},
    {75.0f, -55.0f, -1.9f, -0.1f, 0, false, false, "09-lantern-hill"},
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
        for (int t = 0; t < 200 * kTickRate && !arrived; ++t) {
            const float dx = stop.x - w.player.x, dz = stop.z - w.player.z;
            arrived = dx * dx + dz * dz < 0.04f;
            if (arrived) break;
            Input in;
            in.forward = 1;
            in.hurry = dx * dx + dz * dz > 25.0f;
            in.yaw = std::atan2(dx, -dz);
            in.pitch = 0;
            tick(w, in);
        }
        if (!arrived) {
            std::fprintf(stderr, "familiar: the tour got stuck at (%.2f, %.2f) on the way to (%.2f, %.2f)\n", w.player.x,
                         w.player.z, stop.x, stop.z);
            return 1;
        }
        const float yaw0 = w.player.yaw;
        float dyaw = stop.yaw - yaw0;
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
        if (stop.trick) {
            Input in;
            in.yaw = w.player.yaw;
            in.pitch = w.player.pitch;
            in.trick = true;
            tick(w, in);
        }
        if (!stop.shot) continue;
        scene.update(w);
        play::render(engine, w, false, eyePosition(w).y, false, answerText(w));
        engine.present();
        const float elevation = play::setSky(engine, w);
        std::printf("%-26s (%7.2f, %7.2f) feet %5.2f at %s, sun %5.1f deg, candy %d\n", stop.shot, w.player.x, w.player.z,
                    w.player.feet, clockText(w).c_str(), elevation, w.candy);
        if (!args.framesDir.empty()) engine.saveScreenshotPng(args.framesDir + "/" + stop.shot + ".png");
    }
    std::printf("%zu meshes, fingerprint %016" PRIx64 "\n", scene.meshCount(), fingerprint(w));
    if (w.candy != 3 || w.costume != Costume::Witch) {
        std::fprintf(stderr, "familiar: the tour did not dress up, knock and tell a joke for three candies\n");
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
