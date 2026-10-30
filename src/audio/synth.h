// Hollowell's sounds, made from nothing: every sound is synthesised (no
// recordings ship with the game). The steady sounds are generated sample by
// sample from the Ambience; the one-off sounds are rendered once, when the
// synth is made, and mixed in when they are played. Mono, at kRate.
// (Atman's synth, with an autumn night's sounds.)
//
// Not thread-safe on its own: the device (device.h) locks around it.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "audio/cues.h"

namespace familiar::audio {

constexpr int kRate = 48000;

class Synth {
public:
    Synth();

    // Where the steady sounds are heading; they glide there over about half a second.
    void setAmbience(const Ambience& a) { target_ = a; }
    // Plays a one-off sound. The church bell's strokes follow one another, a
    // couple of seconds apart, however many are played at once.
    void play(Cue c);
    // Writes `frames` samples, each in -1..1.
    void render(float* out, int frames);

    // The rendered one-off sound for a cue (for the tests).
    const std::vector<float>& sound(Cue c) const { return cues_[static_cast<size_t>(c)]; }
    // How many one-off sounds are playing or waiting to (for the tests).
    size_t playing() const { return playing_.size(); }

private:
    struct Playing {
        const std::vector<float>* samples;
        int64_t at;  // negative: still waiting to start
        float gain;
    };
    float noise();
    float next();
    void every(float& clock, float perSecond, const std::vector<std::vector<float>>& sounds, float gain);

    uint32_t seed_ = 0x2545F491u;
    Ambience target_, now_;
    std::array<std::vector<float>, static_cast<size_t>(Cue::Count)> cues_;
    std::vector<std::vector<float>> owls_, leafSteps_, roadSteps_, woodSteps_, pads_, gusts_;
    std::vector<Playing> playing_;
    int64_t clock_ = 0;      // samples rendered
    int64_t nextBell_ = 0;   // when the next stroke of the bell may start
    // The steady sounds' state.
    float windLow_ = 0, leafHigh_ = 0, leafLow_ = 0, creekLow_ = 0, creekHigh_ = 0, creekHigh2_ = 0, purrLow_ = 0;
    double time_ = 0;
    float owlClock_ = 0, gustClock_ = 0, stepClock_ = 0;
};

}  // namespace familiar::audio
