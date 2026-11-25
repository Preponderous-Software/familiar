// Screenshots of the NEXT marker for docs/screenshots/wayfinding (#25, #26,
// #27, #29): each scene sets up a World (the hour, where Wren or the cat
// stands and looks, what is done) and renders one frame, as the game draws it.
// `--walk` also writes the frames of Wren following the marker round the
// Whitfields' house, for the GIF.
//
//   cmake -S . -B build -DFAMILIAR_SHOTS=ON && cmake --build build --target wayfinding_shots
//   ./build/wayfinding_shots DIR [--width W --height H] [--walk]
//
// A developer's tool: not built by default, not part of the game.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "bridge/scene.h"
#include "play/view.h"
#include "sim/craft.h"
#include "sim/doors.h"
#include "sim/world.h"
#if __has_include("sim/ways.h")
#include "sim/ways.h"
#define SHOTS_HAVE_WAYS 1
#endif

using namespace familiar;
namespace T = familiar::town;

namespace {

int64_t tickAt(int minute) { return static_cast<int64_t>((minute - kStartMinute) / kGameMinutesPerSecond * kTickRate) + 1; }

void put(World& w, float x, float z, float yaw, float pitch) {
    w.player.x = x;
    w.player.z = z;
    w.player.yaw = yaw;
    w.player.pitch = pitch;
    float feet = 0;
    if (standAt(w, w.form, x, z, 10.0f, feet)) w.player.feet = feet;
}

float facing(float x, float z, float tx, float tz) { return detAtan2(tx - x, -(tz - z)); }

// The evening so far: a ghost, Nana's lantern lit, and every lit door but `left` knocked at.
World evening(int minute, int left) {
    World w = newWorld(1);
    w.costume = Costume::Ghost;
    w.lit[NanaWindow] = true;
    w.tick = tickAt(minute);
    for (int h = 1; h < T::HouseCount; ++h)
        if (h != left) w.knocked[static_cast<size_t>(h)] = true;
    w.lit[OkaforPorch] = true;
    return w;
}

struct Shot {
    const char* name;
    World w;
};

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::puts("usage: wayfinding_shots DIR [--width W --height H] [--walk]");
        return 2;
    }
    const std::string dir = argv[1];
    int width = 960, height = 540;
    bool walk = false;
    for (int i = 2; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--width" && i + 1 < argc) width = std::atoi(argv[++i]);
        else if (a == "--height" && i + 1 < argc) height = std::atoi(argv[++i]);
        else if (a == "--walk") walk = true;
    }
    strata::EngineDesc desc;
    desc.surface = STRATA_SURFACE_HEADLESS;
    desc.width = width;
    desc.height = height;
    desc.title = "Familiar shots";
    auto engine = strata::Engine::create(desc);
    play::setLighting(*engine);
    bridge::Scene scene(*engine);

    auto frame = [&](const World& w, const std::string& name) {
        scene.update(w);
        play::render(*engine, w, false, eyePosition(w).y);
        engine->present();
        engine->saveScreenshotPng(dir + "/" + name + ".png");
        std::printf("%s: %s\n", name.c_str(), tonightNext(w).c_str());
    };

    // 1. Beside the Whitfields' house after dark, the last door to knock at (#25).
    {
        World w = evening(20 * 60 + 30, T::Whitfield);
        put(w, 42.5f, 16.0f, -0.4f, -0.05f);  // the house on the left, the street ahead
        frame(w, "whitfields-side-wall");
    }
    // 2. West of the creek at 9:30 pm, the apples next: the orchard is across the water (#25).
    {
        World w = evening(21 * 60 + 30, -1);
        w.cinnamon = w.rosemary = true;
        w.gateOpen = {true, true};
        w.lit = {true, true, true, true};
        put(w, 44.0f, 16.0f, 0.75f, -0.02f);  // the orchard on the right, the bridge on the left
        frame(w, "creek-after-dark");
    }
    // 3. The cat on Smoke's trail, east of Nana's herb bed, facing along it (#26).
    {
        World w = newWorld(1);
        w.tick = tickAt(19 * 60 + 30);
        w.form = Form::Cat;
        w.bagDown = true;
        w.bag = {T::kStart.x, 0.0f, T::kStart.z};
        put(w, -55.0f, -28.5f, facing(-55.0f, -28.5f, -40.0f, -34.0f), -0.1f);
        frame(w, "cat-trail");
    }
    // 4. Grandpa Ike's lantern, lit, from the Okafors' walk at dusk (#29).
    {
        World w = evening(18 * 60 + 10, -1);
        put(w, -48.5f, -6.0f, facing(-48.5f, -6.0f, -50.4f, -9.0f), -0.3f);
        frame(w, "ike-lantern-lit");
    }
    // 5. At the Okafors' door, asked to light it: the jar glows for NEXT (#29).
    {
        World w = evening(18 * 60 + 10, -1);
        w.lit[OkaforPorch] = false;
        put(w, -47.8f, -9.4f, facing(-47.8f, -9.4f, -50.4f, -9.0f), -0.45f);  // by the door, out of reach
        frame(w, "ike-lantern-next");
    }
    // 6. Looking down at the street with the place ahead and above the view (#27).
    {
        World w = evening(19 * 60, T::Pell);
        const Vec2 d = T::doorAt(T::Pell);
        put(w, 34.0f, -1.0f, facing(34.0f, -1.0f, d.x, d.z), -0.75f);
        frame(w, "marker-look-down");
    }
    // 7. A place up and to the left, where the HUD is (#27).
    {
        World w = evening(19 * 60, T::Lindqvist);
        const Vec2 d = T::doorAt(T::Lindqvist);
        put(w, 8.0f, -3.0f, facing(8.0f, -3.0f, d.x, d.z) + 0.3f, -0.4f);
        frame(w, "marker-under-hud");
    }
#ifdef SHOTS_HAVE_WAYS
    // The walk: following only the marker from beside the Whitfields' to their door.
    if (walk) {
        World w = evening(20 * 60 + 30, T::Whitfield);
        put(w, 42.5f, 16.0f, 0, -0.05f);
        int n = 0;
        float yaw = facing(42.5f, 16.0f, T::doorAt(T::Whitfield).x, T::doorAt(T::Whitfield).z);
        for (int t = 0; t < 30 * 40; ++t) {
            const NextStep s = tonightStep(w);
            const Vec2 p{w.player.x, w.player.z};
            if (ways::dist(p, s.stand) < 0.8f) break;
            const ways::Wayfinding f = ways::wayfind(w, s.stand, s.at);
            const Vec2 to = f.direct ? s.stand : Vec2{f.aim.x, f.aim.z};
            // Turns toward it like a player would, a little each tick.
            float want = facing(p.x, p.z, to.x, to.z) - yaw;
            while (want > kPi) want -= 2 * kPi;
            while (want < -kPi) want += 2 * kPi;
            yaw += clampf(want, -0.06f, 0.06f);
            Input in;
            in.forward = std::fabs(want) < 0.5f ? 1.0f : 0.0f;
            in.yaw = yaw;
            in.pitch = -0.05f;
            tick(w, in);
            if (t % 6 == 0) {
                char name[32];
                std::snprintf(name, sizeof name, "walk-%03d", n++);
                frame(w, name);
            }
        }
    }
#endif
    return 0;
}
