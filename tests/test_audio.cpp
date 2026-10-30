// Sound (M5): the cues the night's events make, the steady sounds of where
// Wren is and what hour it is, and the synth that makes them all from
// nothing. The autopilot's whole night sounds every cue at least once, and
// the church clock strikes each hour it passes (twelve at midnight, as Smoke
// goes home). Every sound is finite, and the mix never leaves -1..1.
#include <cmath>
#include <cstdio>
#include <set>
#include <vector>

#include "audio/cues.h"
#include "audio/synth.h"
#include "check.h"
#include "sim/autopilot.h"
#include "sim/craft.h"
#include "sim/world.h"

using namespace familiar;
using namespace familiar::audio;
namespace T = familiar::town;

namespace {

int64_t tickAt(int minute) { return static_cast<int64_t>((minute - kStartMinute) / kGameMinutesPerSecond * kTickRate); }

int countOf(const std::vector<Cue>& cs, Cue c) {
    int n = 0;
    for (Cue x : cs) n += x == c ? 1 : 0;
    return n;
}

bool finiteAndWithin(const std::vector<float>& s, float limit) {
    for (float v : s)
        if (!std::isfinite(v) || v > limit || v < -limit) return false;
    return true;
}

float peak(const std::vector<float>& s) {
    float p = 0;
    for (float v : s) p = std::fmax(p, std::fabs(v));
    return p;
}

}  // namespace

int main(int, char**) {
    // ---- Every one-off sound renders: not empty, not silent, finite, and not too loud.
    {
        Synth synth;
        for (int c = 0; c < static_cast<int>(Cue::Count); ++c) {
            const std::vector<float>& s = synth.sound(static_cast<Cue>(c));
            CHECK(!s.empty());
            CHECK(peak(s) > 0.01f);
            CHECK(finiteAndWithin(s, 1.5f));
            if (peak(s) <= 0.01f || !finiteAndWithin(s, 1.5f)) std::printf("cue %d: peak %f\n", c, static_cast<double>(peak(s)));
        }
    }

    // ---- The mix, with everything at once, stays within -1..1.
    {
        Synth synth;
        Ambience a;
        a.wind = a.leaves = a.creek = a.owl = a.purr = 1;
        a.steps = 3;
        synth.setAmbience(a);
        for (int c = 0; c < static_cast<int>(Cue::Count); ++c) synth.play(static_cast<Cue>(c));
        std::vector<float> out(static_cast<size_t>(kRate) * 6);
        synth.render(out.data(), static_cast<int>(out.size()));
        CHECK(finiteAndWithin(out, 1.0f));
        CHECK(peak(out) > 0.05f);
        for (Ground g : {Ground::Road, Ground::Wood}) {
            a.ground = g;
            a.cat = g == Ground::Wood;
            synth.setAmbience(a);
            synth.render(out.data(), kRate);
            CHECK(finiteAndWithin(out, 1.0f));
        }
    }

    // ---- The church bell's strokes follow one another rather than all at once.
    {
        Synth synth;
        for (int i = 0; i < 12; ++i) synth.play(Cue::Bell);
        CHECK(synth.playing() == 12);
        // Each stroke rings for five seconds, 2.2 s after the one before: at six
        // seconds the first has rung out and the other eleven ring or wait.
        std::vector<float> out(static_cast<size_t>(kRate) * 6);
        synth.render(out.data(), static_cast<int>(out.size()));
        CHECK(synth.playing() == 11);
        std::vector<float> rest(static_cast<size_t>(kRate) * 30);
        synth.render(rest.data(), static_cast<int>(rest.size()));
        CHECK(synth.playing() == 0);
        CHECK(finiteAndWithin(rest, 1.0f));
    }

    // ---- The steady sounds of the place and the hour.
    {
        World w = newWorld(1);
        const Ambience street = ambience(w, eyePosition(w), 0);
        CHECK(street.owl == 0);  // not dark yet
        CHECK(street.wind > 0 && street.leaves > 0 && street.purr == 0);
        CHECK(street.ground == Ground::Leaves);  // Nana's front walk

        World night = w;
        night.tick = tickAt(20 * 60);
        CHECK(ambience(night, eyePosition(night), 0).owl > 0);

        World creek = w;
        creek.player.x = 47.0f;  // the bridge's west end, by the water
        creek.player.z = 0.0f;
        CHECK(ambience(creek, eyePosition(creek), 0).creek > 2 * street.creek);
        creek.player.x = 52.0f;
        CHECK(ambience(creek, eyePosition(creek), 0).ground == Ground::Wood);
        creek.player.x = 0.0f;
        CHECK(ambience(creek, eyePosition(creek), 0).ground == Ground::Road);

        World hill = w;
        hill.player.x = T::kHill.x;
        hill.player.z = T::kHill.z;
        const Vec3 top{hill.player.x, T::groundHeight(hill.player.x, hill.player.z) + 1.5f, hill.player.z};
        CHECK(ambience(hill, top, 0).wind > street.wind);

        // Smoke, following the cat, purrs close by.
        World cat = w;
        cat.form = Form::Cat;
        cat.smoke = SmokeState::Following;
        cat.smokeX = cat.player.x + 1.2f;
        cat.smokeZ = cat.player.z;
        const Ambience a = ambience(cat, eyePosition(cat), 2);
        CHECK(a.purr > 0.9f && a.cat && a.steps == 2);
    }

    // ---- The cues for what happens, and none for a jump.
    {
        World w = newWorld(1);
        CHECK(cues(w, w).empty());
        World a = w;
        ++a.tick;
        a.candy = 3;
        a.answered = T::Okafor;
        a.answeredTick = a.tick;
        const std::vector<Cue> c = cues(w, a);
        CHECK(countOf(c, Cue::Knock) == 1 && countOf(c, Cue::Candy) == 1);

        World loaded = w;
        loaded.tick += 10 * kTickRate;
        loaded.candy = 9;
        CHECK(cues(w, loaded).empty());  // a load is not a knock

        World six = w, past = w;
        six.tick = tickAt(18 * 60) - 1;
        past.tick = six.tick + 1;
        CHECK(countOf(cues(six, past), Cue::Bell) == 6);
        CHECK(countOf(cues(six, six), Cue::Bell) == 0);
    }

    // ---- The autopilot's whole night sounds every cue, and the bell strikes
    // 6, 7, 8, 9, 10 and 11, then twelve at midnight as Smoke goes home.
    {
        World w = newWorld(1);
        std::set<int> heard;
        int bells = 0, bellsAtMidnight = 0;
        while (!playthroughDone(w) && w.tick < kPlaythroughLimit) {
            const World before = w;
            tick(w, autopilot(w));
            const std::vector<Cue> c = cues(before, w);
            for (Cue x : c) heard.insert(static_cast<int>(x));
            bells += countOf(c, Cue::Bell);
            if (countOf(c, Cue::Home)) bellsAtMidnight = countOf(c, Cue::Bell);
        }
        CHECK(playthroughDone(w));
        for (int c = 0; c < static_cast<int>(Cue::Count); ++c) {
            if (!heard.count(c)) std::printf("the night never sounded cue %d\n", c);
            CHECK(heard.count(c) == 1);
        }
        CHECK(bellsAtMidnight == 12);
        CHECK(bells == 6 + 7 + 8 + 9 + 10 + 11 + 12);
    }

    TEST_MAIN_END();
}
