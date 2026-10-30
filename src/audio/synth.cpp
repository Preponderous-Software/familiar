#include "audio/synth.h"

#include <cmath>
#include <initializer_list>

namespace familiar::audio {

namespace {

constexpr float kTwoPi = 6.2831853f;
constexpr float kDt = 1.0f / static_cast<float>(kRate);
constexpr float kGlide = 1.0f / (0.5f * static_cast<float>(kRate));  // the steady sounds' glide, per sample
constexpr float kBellApart = 2.2f;  // seconds between the church bell's strokes

// A small deterministic noise source, so the one-off sounds render the same
// every time.
struct Rng {
    uint32_t s;
    float next() {  // -1..1
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return static_cast<float>(s >> 8) / 8388608.0f - 1.0f;
    }
    float between(float a, float b) { return a + (b - a) * (next() * 0.5f + 0.5f); }
};

// One-pole low-pass filter coefficient for a cutoff in Hz.
float pole(float hz) { return 1.0f - std::exp(-kTwoPi * hz * kDt); }

int samples(float seconds) { return static_cast<int>(seconds * static_cast<float>(kRate)); }
float at(int i) { return static_cast<float>(i) * kDt; }
std::vector<float> silence(float seconds) { return std::vector<float>(static_cast<size_t>(samples(seconds)), 0.0f); }

// 0 at the edges of [start, start + length], 1 in the middle.
float window(float t, float start, float length) {
    const float u = (t - start) / length;
    if (u <= 0 || u >= 1) return 0;
    const float s = std::sin(u * 3.14159265f);
    return s * s;
}

float decay(float t, float start, float tau) { return t < start ? 0.0f : std::exp(-(t - start) / tau); }

// Adds `src` into `dst` from `start` seconds, times `gain`.
void mix(std::vector<float>& dst, const std::vector<float>& src, float start, float gain) {
    const size_t from = static_cast<size_t>(samples(start));
    for (size_t i = 0; i < src.size() && from + i < dst.size(); ++i) dst[from + i] += gain * src[i];
}

// A struck bell: inharmonic partials, the higher ones dying first.
std::vector<float> bell(float hz, float seconds, float gain) {
    static const float ratio[] = {1.0f, 2.0f, 2.76f, 5.4f, 8.9f};
    static const float amp[] = {1.0f, 0.6f, 0.4f, 0.25f, 0.1f};
    static const float tau[] = {1.6f, 1.1f, 0.8f, 0.4f, 0.2f};
    std::vector<float> out = silence(seconds);
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(static_cast<int>(i));
        const float attack = std::fmin(t / 0.004f, 1.0f);
        float s = 0;
        for (int k = 0; k < 5; ++k) s += amp[k] * std::exp(-t / tau[k]) * std::sin(kTwoPi * hz * ratio[k] * t);
        out[i] = gain * attack * s;
    }
    return out;
}

// The church clock's bell: a big bell's hum, prime, minor third, fifth and
// nominal, ringing on for seconds, heard from across the town.
std::vector<float> churchBell() {
    static const float ratio[] = {0.5f, 1.0f, 1.2f, 1.5f, 2.0f, 2.5f};
    static const float amp[] = {0.7f, 1.0f, 0.5f, 0.3f, 0.45f, 0.15f};
    static const float tau[] = {3.5f, 2.2f, 1.6f, 1.2f, 0.9f, 0.4f};
    const float hz = 233.0f;  // a B flat, the old bell of a small New England church
    std::vector<float> out = silence(5.0f);
    float lo = 0;
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(static_cast<int>(i));
        float s = 0;
        for (int k = 0; k < 6; ++k) s += amp[k] * std::exp(-t / tau[k]) * std::sin(kTwoPi * hz * ratio[k] * t + static_cast<float>(k));
        lo += pole(1800) * (s - lo);  // across the town: the edge taken off
        out[i] = 0.22f * std::fmin(t / 0.006f, 1.0f) * lo;
    }
    return out;
}

// Soft sine tones starting `apart` seconds apart.
std::vector<float> chime(std::initializer_list<float> notes, float apart, float tau, float seconds, float gain = 0.25f) {
    std::vector<float> out = silence(seconds);
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(static_cast<int>(i));
        float s = 0, start = 0;
        for (float hz : notes) {
            if (t >= start) {
                const float u = t - start;
                s += std::fmin(u / 0.006f, 1.0f) * std::exp(-u / tau) *
                     (std::sin(kTwoPi * hz * u) + 0.3f * std::sin(kTwoPi * 2 * hz * u));
            }
            start += apart;
        }
        out[i] = gain * s;
    }
    return out;
}

// A rustle: band-passed noise in a soft swell.
std::vector<float> rustle(Rng& r, float seconds, float lowHz, float highHz, float gain) {
    std::vector<float> out = silence(seconds);
    float lo = 0, hi = 0;
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(static_cast<int>(i)), n = r.next();
        lo += pole(lowHz) * (n - lo);
        hi += pole(highHz) * (n - hi);
        const float flutter = 0.6f + 0.4f * std::sin(kTwoPi * 23.0f * t + 3 * r.next() * 0.02f);
        out[i] = gain * (hi - lo) * window(t, 0, seconds) * flutter;
    }
    return out;
}

// Three knuckles on a wooden door, and a moment later the latch and the door
// swinging open.
std::vector<float> knock(Rng& r) {
    std::vector<float> out = silence(1.6f);
    for (float start : {0.0f, 0.2f, 0.4f}) {
        float lo = 0;
        for (int i = samples(start); i < samples(start + 0.12f); ++i) {
            const float u = at(i) - start, n = r.next();
            lo += pole(700) * (n - lo);
            const float body = std::sin(kTwoPi * 165.0f * u) + 0.5f * std::sin(kTwoPi * 310.0f * u);
            out[static_cast<size_t>(i)] += (2.2f * lo + 0.5f * body) * std::exp(-u / 0.025f);
        }
    }
    // The latch: a small click; the door: a soft, low creak.
    for (int i = samples(0.95f); i < samples(0.97f); ++i) out[static_cast<size_t>(i)] += 0.3f * r.next() * decay(at(i), 0.95f, 0.004f);
    float creak = 0;
    for (int i = samples(1.0f); i < static_cast<int>(out.size()); ++i) {
        const float u = at(i) - 1.0f;
        creak += pole(900) * (r.next() - creak);
        const float pulse = 0.5f + 0.5f * std::sin(kTwoPi * (38.0f + 30.0f * u) * u);
        out[static_cast<size_t>(i)] += 0.12f * window(u, 0, 0.55f) * (pulse * pulse * pulse * std::sin(kTwoPi * 320.0f * u) + 0.6f * creak);
    }
    for (float& s : out) s *= 0.8f;
    return out;
}

// Candy into the bag: a crinkle of wrappers and two bright little notes.
std::vector<float> candy(Rng& r) {
    std::vector<float> out = rustle(r, 0.35f, 1500, 7000, 1.4f);
    out.resize(static_cast<size_t>(samples(1.0f)), 0.0f);
    mix(out, chime({1319, 1568}, 0.09f, 0.18f, 0.8f, 0.18f), 0.12f, 1.0f);
    return out;
}

// A cat's meow: a voice whose pitch rises and falls, its vowel opening from
// "mi" to "ow". `hz` is where the pitch starts.
std::vector<float> meow(float hz, float seconds, float gain) {
    std::vector<float> out = silence(seconds);
    double phase = 0;
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(static_cast<int>(i)), u = t / seconds;
        const float f0 = hz * (1.0f + 0.45f * std::sin(3.14159265f * std::fmin(u * 1.3f, 1.0f)) - 0.15f * u);
        phase += static_cast<double>(f0 * kDt);
        // The mouth opens then closes: the formant rises and falls.
        const float formant = 900.0f + 1400.0f * std::sin(3.14159265f * u);
        float s = 0;
        for (int k = 1; k <= 14; ++k) {
            const float fk = f0 * static_cast<float>(k);
            const float d = (fk - formant) / 600.0f;
            s += std::exp(-d * d) * std::sin(static_cast<float>(6.283185307179586 * std::fmod(phase * k, 1.0)));
        }
        out[i] = gain * window(t, 0, seconds) * s;
    }
    return out;
}

// A purr: a low rumble, about 25 breaths of it a second.
std::vector<float> purr(Rng& r, float seconds, float gain) {
    std::vector<float> out = silence(seconds);
    float lo = 0;
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(static_cast<int>(i));
        lo += pole(180) * (r.next() - lo);
        float pulse = 0.5f + 0.5f * std::sin(kTwoPi * 24.0f * t);
        pulse *= pulse;
        const float breath = 0.6f + 0.4f * std::sin(kTwoPi * 0.5f * t);
        out[i] = gain * window(t, 0, seconds) * breath * pulse * (5.0f * lo + 0.3f * std::sin(kTwoPi * 48.0f * t));
    }
    return out;
}

// Wren becoming the cat (falling notes, soft as a shiver) or herself again (rising).
std::vector<float> change(bool toCat) {
    if (toCat) return chime({1568, 1319, 1047, 784}, 0.07f, 0.35f, 1.3f, 0.12f);
    return chime({784, 1047, 1319, 1568}, 0.07f, 0.35f, 1.3f, 0.12f);
}

// The bag set down or picked up: a papery shuffle and a soft bump.
std::vector<float> bag(Rng& r) {
    std::vector<float> out = rustle(r, 0.3f, 400, 3000, 1.6f);
    float lo = 0;
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(static_cast<int>(i));
        lo += pole(150) * (r.next() - lo);
        out[i] += 3.0f * lo * decay(t, 0.05f, 0.04f);
    }
    return out;
}

// A latch lifting, or an old bolt sliding back, and the gate easing open.
std::vector<float> latch(Rng& r) {
    std::vector<float> out = silence(1.1f);
    float hi = 0, scrape = 0;
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(static_cast<int>(i)), n = r.next();
        hi += pole(5000) * (n - hi);
        scrape += pole(1200) * (n - scrape);
        float s = 0.9f * (n - hi) * decay(t, 0, 0.006f) + 0.7f * (n - hi) * decay(t, 0.12f, 0.008f);  // click, clack
        s += 0.6f * scrape * window(t, 0.25f, 0.5f) * (0.7f + 0.3f * std::sin(kTwoPi * 31.0f * t));  // the gate swings
        out[i] = 0.7f * s;
    }
    return out;
}

// A match struck and a lantern lit: the scratch and flare, then a warm chord.
std::vector<float> lanternLit(Rng& r) {
    std::vector<float> out = silence(2.2f);
    float lo = 0, hi = 0;
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(static_cast<int>(i)), n = r.next();
        lo += pole(1500) * (n - lo);
        hi += pole(6000) * (n - hi);
        float s = 0.9f * (hi - lo) * window(t, 0, 0.12f);    // the scratch
        s += 1.4f * lo * window(t, 0.1f, 0.4f) * (0.5f + 0.5f * r.next());  // the flare
        out[i] = 0.6f * s;
    }
    mix(out, chime({523, 659, 784}, 0.05f, 0.7f, 2.0f, 0.12f), 0.25f, 1.0f);
    return out;
}

// An apple or a sprig picked: a small snap among the leaves.
std::vector<float> pick(Rng& r) {
    std::vector<float> out = rustle(r, 0.4f, 800, 5000, 1.2f);
    for (int i = samples(0.15f); i < samples(0.17f); ++i) out[static_cast<size_t>(i)] += 0.6f * r.next() * decay(at(i), 0.15f, 0.004f);
    return out;
}

// The iron pot stirred: a slow swirl and a few bubbles.
std::vector<float> stir(Rng& r) {
    std::vector<float> out = silence(1.2f);
    float lo = 0;
    float blipAt[5], blipHz[5];
    for (int k = 0; k < 5; ++k) {
        blipAt[k] = r.between(0.1f, 1.0f);
        blipHz[k] = r.between(250, 600);
    }
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(static_cast<int>(i));
        lo += pole(450) * (r.next() - lo);
        float s = 2.0f * lo * window(t, 0, 1.2f) * (0.6f + 0.4f * std::sin(kTwoPi * 1.6f * t));
        for (int k = 0; k < 5; ++k) {
            const float u = t - blipAt[k];
            if (u > 0 && u < 0.06f) s += 0.2f * window(u, 0, 0.06f) * std::sin(kTwoPi * blipHz[k] * (1 + 6 * u) * u);
        }
        out[i] = s;
    }
    return out;
}

// Nana's cider ready: a last swirl and a glad little chime.
std::vector<float> brewed(Rng& r) {
    std::vector<float> out = stir(r);
    out.resize(static_cast<size_t>(samples(2.4f)), 0.0f);
    mix(out, chime({659, 784, 988, 1319}, 0.11f, 0.6f, 1.9f, 0.16f), 0.4f, 1.0f);
    return out;
}

// The page of a notebook turned.
std::vector<float> page(Rng& r) {
    std::vector<float> out = silence(0.35f);
    float lo = 0, hi = 0;
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(static_cast<int>(i)), n = r.next();
        lo += pole(1000) * (n - lo);
        hi += pole(5000) * (n - hi);
        out[i] = 1.0f * (hi - lo) * window(t, 0, 0.3f) * (0.4f + 0.6f * decay(t, 0.15f, 0.05f));
    }
    return out;
}

// A spirit: airy, slow-swelling high notes, a little apart from one another.
std::vector<float> spirit() {
    std::vector<float> out = silence(3.0f);
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(static_cast<int>(i));
        const float s = std::sin(kTwoPi * 1175.0f * t) + std::sin(kTwoPi * 1177.5f * t) + 0.7f * std::sin(kTwoPi * 1760.0f * t) +
                        0.5f * std::sin(kTwoPi * 2349.0f * t);
        out[i] = 0.05f * window(t, 0, 3.0f) * s;
    }
    return out;
}

// An owl, from the trees: hoo, hoo-hoo.
std::vector<float> owl(Rng& r) {
    std::vector<float> out = silence(2.0f);
    const float hz = r.between(330, 400);
    const float starts[] = {0.0f, 0.75f, 1.05f};
    const float lengths[] = {0.45f, 0.22f, 0.5f};
    float lo = 0;
    for (int k = 0; k < 3; ++k)
        for (int i = samples(starts[k]); i < samples(starts[k] + lengths[k]) && i < static_cast<int>(out.size()); ++i) {
            const float u = at(i) - starts[k];
            const float f = hz * (1.0f - 0.06f * u / lengths[k]);
            out[static_cast<size_t>(i)] += window(u, 0, lengths[k]) * (std::sin(kTwoPi * f * u) + 0.15f * std::sin(kTwoPi * 2 * f * u));
        }
    for (float& s : out) {
        lo += pole(1200) * (s - lo);
        s = 0.22f * lo;
    }
    return out;
}

// A footstep: on dry leaves (a crunch), the road (a scuff), wood (a hollow
// knock), or a cat's soft pad.
std::vector<float> step(Rng& r, Ground g, bool cat) {
    std::vector<float> out = silence(cat ? 0.08f : 0.16f);
    float lo = 0, hi = 0;
    for (size_t i = 0; i < out.size(); ++i) {
        const float t = at(static_cast<int>(i)), n = r.next();
        lo += pole(cat ? 250.0f : g == Ground::Wood ? 500.0f : 900.0f) * (n - lo);
        hi += pole(4000) * (n - hi);
        float s;
        if (cat) s = 1.2f * lo * window(t, 0, 0.07f);
        else if (g == Ground::Leaves) s = 0.9f * lo * decay(t, 0, 0.04f) + 0.7f * (hi - lo) * window(t, 0.0f, 0.14f) * (r.next() > 0.6f ? 1.0f : 0.3f);
        else if (g == Ground::Road) s = 1.6f * lo * std::fmin(t / 0.003f, 1.0f) * decay(t, 0, 0.025f);
        else s = (1.4f * lo + 0.5f * std::sin(kTwoPi * 140.0f * t)) * std::fmin(t / 0.002f, 1.0f) * decay(t, 0, 0.035f);
        out[i] = s;
    }
    return out;
}

// A gust in the leaves overhead.
std::vector<float> gust(Rng& r) { return rustle(r, r.between(1.2f, 2.4f), 1200, 6000, 0.8f); }

// A pure tone at time `t`, in double: a float t of minutes would smear the
// phase of anything above a few hundred hertz.
float tone(double hz, double t) { return static_cast<float>(std::sin(6.283185307179586 * hz * t)); }

// Gentle saturation, so loud moments round off instead of clipping.
float soften(float x) {
    if (x > 3) return 1;
    if (x < -3) return -1;
    return x * (27 + x * x) / (27 + 9 * x * x);
}

}  // namespace

Synth::Synth() {
    Rng r{0x9E3779B9u};
    auto set = [&](Cue c, std::vector<float> s) { cues_[static_cast<size_t>(c)] = std::move(s); };
    set(Cue::Knock, knock(r));
    set(Cue::Candy, candy(r));
    set(Cue::Meow, meow(560, 0.65f, 0.14f));
    set(Cue::Change, change(true));
    set(Cue::ChangeBack, change(false));
    set(Cue::Bag, bag(r));
    set(Cue::Latch, latch(r));
    set(Cue::Lantern, lanternLit(r));
    set(Cue::Pick, pick(r));
    set(Cue::ShopBell, bell(1568, 1.6f, 0.12f));
    set(Cue::Stir, stir(r));
    set(Cue::Brewed, brewed(r));
    set(Cue::Spirit, spirit());
    set(Cue::Page, page(r));
    {
        // Smoke: a small, high, glad meow, then a purr.
        std::vector<float> s = meow(700, 0.45f, 0.11f);
        s.resize(static_cast<size_t>(samples(2.6f)), 0.0f);
        mix(s, purr(r, 2.0f, 0.35f), 0.5f, 1.0f);
        set(Cue::Found, s);
    }
    set(Cue::Settle, purr(r, 2.5f, 0.4f));
    set(Cue::Offering, chime({523, 659, 784, 1047}, 0.3f, 1.2f, 3.5f, 0.12f));
    set(Cue::Home, chime({784, 988, 1175, 1568, 1976}, 0.45f, 1.6f, 5.5f, 0.11f));
    set(Cue::Bell, churchBell());
    for (int i = 0; i < 3; ++i) owls_.push_back(owl(r));
    for (int i = 0; i < 4; ++i) {
        leafSteps_.push_back(step(r, Ground::Leaves, false));
        roadSteps_.push_back(step(r, Ground::Road, false));
        woodSteps_.push_back(step(r, Ground::Wood, false));
        pads_.push_back(step(r, Ground::Leaves, true));
    }
    for (int i = 0; i < 4; ++i) gusts_.push_back(gust(r));
}

void Synth::play(Cue c) {
    const auto& s = cues_[static_cast<size_t>(c)];
    float gain = 0.8f;
    if (c == Cue::Page || c == Cue::Spirit) gain = 0.6f;  // not rewards: they only say something happened
    int64_t start = 0;
    if (c == Cue::Bell) {  // one stroke after another
        const int64_t when = nextBell_ > clock_ ? nextBell_ : clock_;
        start = clock_ - when;
        nextBell_ = when + samples(kBellApart);
    }
    playing_.push_back({&s, start, gain});
}

float Synth::noise() {
    seed_ ^= seed_ << 13;
    seed_ ^= seed_ >> 17;
    seed_ ^= seed_ << 5;
    return static_cast<float>(seed_ >> 8) / 8388608.0f - 1.0f;
}

void Synth::every(float& clock, float perSecond, const std::vector<std::vector<float>>& sounds, float gain) {
    if (perSecond <= 0) return;
    clock += perSecond * kDt;
    if (clock < 1) return;
    clock = -0.6f * (noise() * 0.5f + 0.5f);  // not like clockwork
    const size_t pick = static_cast<size_t>((noise() * 0.5f + 0.5f) * static_cast<float>(sounds.size())) % sounds.size();
    playing_.push_back({&sounds[pick], 0, gain});
}

float Synth::next() {
    auto glide = [](float& v, float to) { v += (to - v) * kGlide; };
    glide(now_.wind, target_.wind);
    glide(now_.leaves, target_.leaves);
    glide(now_.creek, target_.creek);
    glide(now_.owl, target_.owl);
    glide(now_.purr, target_.purr);
    time_ += static_cast<double>(kDt);
    const double td = std::fmod(time_, 1000.0);  // for tones
    const float t = static_cast<float>(td);        // for slow swells and rhythms
    const float n = noise();
    float s = 0;

    // The wind: low noise whose colour drifts, in slow gusts.
    windLow_ += pole(150 + 110 * std::sin(kTwoPi * 0.05f * t)) * (n - windLow_);
    const float gusting = 0.65f + 0.35f * std::sin(kTwoPi * 0.06f * t + 1) * std::sin(kTwoPi * 0.023f * t);
    s += now_.wind * 0.8f * windLow_ * gusting;

    // Dry leaves stirring: a thin hiss that rises with the gusts, and now and
    // then a gust through the branches.
    leafHigh_ += pole(5000) * (n - leafHigh_);
    leafLow_ += pole(1500) * (leafHigh_ - leafLow_);
    s += now_.leaves * 0.18f * (leafHigh_ - leafLow_) * gusting * gusting;
    every(gustClock_, now_.leaves * 0.08f, gusts_, 0.5f * now_.leaves);

    // The creek: a low rush and a bright babble, quickly breathing.
    creekLow_ += pole(400) * (n - creekLow_);
    creekHigh_ += pole(3500) * (n - creekHigh_);
    creekHigh2_ += pole(1100) * (creekHigh_ - creekHigh2_);
    const float babble = 0.7f + 0.2f * std::sin(kTwoPi * 0.9f * t) + 0.15f * std::sin(kTwoPi * 2.3f * t + 2);
    s += now_.creek * babble * (0.8f * creekLow_ + 0.8f * (creekHigh_ - creekHigh2_));

    // An owl, now and then.
    every(owlClock_, now_.owl * 0.045f, owls_, 0.8f * now_.owl);

    // Smoke purring.
    if (now_.purr > 0.001f) {
        purrLow_ += pole(180) * (n - purrLow_);
        float pulse = 0.5f + 0.5f * static_cast<float>(tone(24.0, td));
        pulse *= pulse;
        s += now_.purr * 0.12f * (0.6f + 0.4f * std::sin(kTwoPi * 0.45f * t)) * pulse * (5.0f * purrLow_ + 0.3f * tone(48.0, td));
    }

    // Footsteps, as fast as the body walks.
    if (target_.steps > 0) {
        stepClock_ += target_.steps * kDt;
        if (stepClock_ >= 1) {
            stepClock_ -= 1;
            const auto& set = target_.cat ? pads_
                              : target_.ground == Ground::Road ? roadSteps_
                              : target_.ground == Ground::Wood ? woodSteps_
                                                               : leafSteps_;
            playing_.push_back({&set[static_cast<size_t>((noise() * 0.5f + 0.5f) * 3.99f)], 0, target_.cat ? 0.25f : 0.5f});
        }
    } else {
        stepClock_ = 0.7f;  // the first step comes soon after setting off
    }

    // Everything playing once (or waiting its turn, like the bell's strokes).
    for (size_t i = 0; i < playing_.size();) {
        Playing& p = playing_[i];
        if (p.at >= 0) s += p.gain * (*p.samples)[static_cast<size_t>(p.at)];
        ++p.at;
        if (p.at >= static_cast<int64_t>(p.samples->size())) {
            playing_[i] = playing_.back();
            playing_.pop_back();
        } else {
            ++i;
        }
    }
    ++clock_;
    return soften(0.7f * s);
}

void Synth::render(float* out, int frames) {
    for (int i = 0; i < frames; ++i) out[i] = next();
}

}  // namespace familiar::audio
