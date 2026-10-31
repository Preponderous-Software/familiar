#include "play/view.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "bridge/geometry.h"
#include "sim/cat.h"
#include "sim/craft.h"
#include "sim/doors.h"
#include "sim/notebook.h"
#include "sim/town.h"

namespace familiar::play {

namespace T = familiar::town;

namespace {

constexpr float kFovY = 70.0f;
float gTextSize = 16.0f;
float gLine = 20.0f;
constexpr float kMargin = 16.0f;
constexpr bridge::Rgb kPorchColor{1.0f, 0.70f, 0.38f};
constexpr bridge::Rgb kPumpkinColor{1.0f, 0.45f, 0.08f};
constexpr bridge::Rgb kStreetColor{1.0f, 0.82f, 0.55f};
constexpr bridge::Rgb kLanternColor{1.0f, 0.62f, 0.25f};
constexpr float kLabelRange = 22.0f;
constexpr float kLabelNear = 3.0f;
constexpr float kGoalNear = 3.5f;  // the NEXT marker hides this close to its place

StrataCamera lookFrom(Vec3 eye, float yaw, float pitch, float aspect, float nearPlane) {
    const Vec3 d = lookDirection(yaw, pitch);
    const float e[3] = {eye.x, eye.y, eye.z};
    const float target[3] = {eye.x + d.x, eye.y + d.y, eye.z + d.z};
    const float up[3] = {0, 1, 0};
    return strata::cameraLookAt(e, target, up, kFovY, aspect, nearPlane, 400.0f);
}

void text(std::vector<StrataText>& out, std::vector<std::string>& keep, float x, float y, const std::string& s,
          const float color[4], float size = 0) {
    keep.push_back(s);
    StrataText t{};
    t.x = x;
    t.y = y;
    t.pixel_size = size > 0 ? size : gTextSize;
    t.flags = STRATA_TEXT_SHADOW;
    for (int i = 0; i < 4; ++i) t.color[i] = color[i];
    out.push_back(t);
}

float nightness(float sunElevation) { return clampf(-sunElevation / 6.0f, 0.0f, 1.0f); }

// The jack-o'-lanterns' candle flames, and the streetlamps' glow, as soft points.
void drawFlames(strata::Engine& engine, const World& w, float night) {
    std::vector<StrataPoint> pts;
    const float t = static_cast<float>(w.tick) / kTickRate;
    for (int i = 0; i < T::HouseCount; ++i) {
        const auto id = static_cast<T::HouseId>(i);
        int k = 0;
        for (Vec3 p : bridge::pumpkins(id)) {
            StrataPoint q{};
            q.position[0] = p.x;
            q.position[1] = p.y + 0.22f;
            q.position[2] = p.z + (T::kHouses[i].north ? 0.27f : -0.27f);
            q.size_pixels = 2.0f;
            q.world_diameter = 0.5f;
            q.color[0] = 1.0f;
            q.color[1] = 0.5f;
            q.color[2] = 0.1f;
            q.color[3] = (0.35f + 0.1f * std::sin(t * 9.0f + static_cast<float>(i * 3 + k))) * (0.4f + 0.6f * night);
            q.flags = STRATA_POINT_SOFT;
            pts.push_back(q);
            ++k;
        }
        if (porchLightOn(w, id)) {
            const Vec3 l = T::porchLight(id);
            StrataPoint q{};
            q.position[0] = l.x;
            q.position[1] = l.y;
            q.position[2] = l.z + (T::kHouses[i].north ? 0.15f : -0.15f);
            q.size_pixels = 3.0f;
            q.world_diameter = 0.45f;
            q.color[0] = 1.0f;
            q.color[1] = 0.8f;
            q.color[2] = 0.5f;
            q.color[3] = 0.85f * night;
            q.flags = STRATA_POINT_SOFT;
            pts.push_back(q);
        }
    }
    // The family lanterns that are lit: a candle flame in each jar.
    for (int i = 0; i < kLanternCount; ++i) {
        if (!w.lit[static_cast<size_t>(i)]) continue;
        // The flame over the rim, and a halo round the jar.
        const Vec3 a = lantern(i).at;
        const float flicker = 0.15f * std::sin(t * 7.0f + static_cast<float>(i));
        for (int k = 0; k < 2; ++k) {
            StrataPoint q{};
            q.position[0] = a.x;
            q.position[1] = a.y + (k ? 0.18f : 0.3f);
            q.position[2] = a.z;
            q.size_pixels = 3.0f;
            q.world_diameter = k ? 0.75f : 0.16f;
            q.color[0] = 1.0f;
            q.color[1] = k ? 0.55f : 0.8f;
            q.color[2] = k ? 0.15f : 0.4f;
            q.color[3] = (k ? 0.35f : 0.85f) + flicker;
            q.flags = STRATA_POINT_SOFT;
            pts.push_back(q);
        }
    }
    const Vec3 eye = eyePosition(w);
    std::vector<StrataPoint>* into = &pts;
    auto soft = [into](Vec3 p, float diameter, float r, float g, float b, float a) {
        StrataPoint q{};
        q.position[0] = p.x;
        q.position[1] = p.y;
        q.position[2] = p.z;
        q.size_pixels = 2.0f;
        q.world_diameter = diameter;
        q.color[0] = r;
        q.color[1] = g;
        q.color[2] = b;
        q.color[3] = a;
        q.flags = STRATA_POINT_SOFT;
        into->push_back(q);
    };
    // The dropped candy bag glints, so it can be found again in a dark yard (#15),
    // and the NEXT line's place glows faintly (#14), seen from afar.
    if (w.bagDown) {
        const float glint = 0.6f + 0.3f * std::sin(t * 4.0f);
        soft({w.bag.x, w.bag.y + 0.35f, w.bag.z}, 0.45f, 1.0f, 0.9f, 0.55f, glint);
        soft({w.bag.x, w.bag.y + 0.35f, w.bag.z}, 1.4f, 1.0f, 0.75f, 0.35f, 0.25f * glint);
    }
    if (const NextStep n = tonightStep(w); n.goal) {
        const float dx = n.at.x - eye.x, dz = n.at.z - eye.z;
        if (dx * dx + dz * dz > kGoalNear * kGoalNear)
            soft(n.at, 0.9f, 0.75f, 0.85f, 1.0f, 0.35f + 0.15f * std::sin(t * 3.0f));
    }
    // What E would use, in reach: a pulsing glint on the lantern jar, or by the door.
    {
        const float pulse = 0.55f + 0.3f * std::sin(t * 5.0f);
        const int l = lanternHere(w);
        if (l >= 0 && craftWord(w) == "Light") {
            const Vec3 a = lantern(l).at;
            soft({a.x, a.y + 0.2f, a.z}, 0.55f, 1.0f, 0.85f, 0.45f, pulse);
        } else if (const int h = atDoor(w); h >= 0 && h != T::Nana && usePrompt(w).rfind("E  ", 0) == 0) {
            const auto id = static_cast<T::HouseId>(h);
            const Vec2 d = T::doorAt(id);
            soft({d.x + 0.35f, T::kPorchHeight + 1.05f, d.z + (T::kHouses[h].north ? 0.12f : -0.12f)}, 0.3f, 1.0f, 0.85f, 0.45f, pulse);
        }
    }
    // Smoke's paw prints, from Nana's herb bed to the oak, glowing for a cat.
    if (w.form == Form::Cat && (w.smoke == SmokeState::Lost || w.smoke == SmokeState::Waiting)) {
        const auto& trail = smokeTrail();
        int k = 0;
        for (size_t i = 0; i + 1 < trail.size(); ++i) {
            const Vec2 a = trail[i], b = trail[i + 1];
            const float dx = b.x - a.x, dz = b.z - a.z, len = std::sqrt(dx * dx + dz * dz);
            const float nx = -dz / len, nz = dx / len;
            for (float s = 0; s < len; s += 0.7f, ++k) {
                const float side = k % 2 ? 0.07f : -0.07f;
                const float x = a.x + dx * s / len + nx * side, z = a.z + dz * s / len + nz * side;
                const float ex = x - eye.x, ez = z - eye.z;
                if (ex * ex + ez * ez > 28.0f * 28.0f) continue;
                const float y = T::kBridge.contains(x, z) ? T::kBridgeDeck : T::groundHeight(x, z);
                soft({x, y + 0.03f, z}, 0.13f, 0.55f, 0.85f, 1.0f, 0.75f);
            }
        }
    }
    // The owl's eyes in the old oak, after dark.
    if (night > 0.5f)
        for (float dx : {-0.07f, 0.07f}) soft({T::kOldOak.x + dx, 4.6f, T::kOldOak.z + 0.85f}, 0.06f, 1.0f, 0.85f, 0.3f, 0.9f);
    for (Vec2 p : T::kStreetlamps) {
        StrataPoint q{};
        q.position[0] = p.x;
        q.position[1] = T::kStreetlampHeight - 0.3f;
        q.position[2] = p.z;
        q.size_pixels = 3.0f;
        q.world_diameter = 1.1f;
        q.color[0] = 1.0f;
        q.color[1] = 0.85f;
        q.color[2] = 0.6f;
        q.color[3] = 0.5f * night;
        q.flags = STRATA_POINT_SOFT;
        pts.push_back(q);
    }
    // The lamplighter's lamp, which only the cat sees.
    if (w.form == Form::Cat) {
        const Vec3 l = lamplighterLamp(w);
        StrataPoint q{};
        q.position[0] = l.x;
        q.position[1] = l.y;
        q.position[2] = l.z;
        q.size_pixels = 3.0f;
        q.world_diameter = 0.5f;
        q.color[0] = 1.0f;
        q.color[1] = 0.85f;
        q.color[2] = 0.55f;
        q.color[3] = 0.6f + 0.15f * std::sin(t * 5.0f);
        q.flags = STRATA_POINT_SOFT;
        pts.push_back(q);
    }
    if (!pts.empty()) engine.submitPoints(pts.data(), pts.size());
}

// The waning gibbous moon, risen in the east-north-east at about 10:46 pm
// (RESEARCH.md, setting facts), climbing slowly: a pale disc far away.
void drawMoon(strata::Engine& engine, const World& w, Vec3 eye) {
    const float minute = static_cast<float>(gameMinutes(w));
    const float rise = 22 * 60 + 46;
    if (minute < rise) return;
    const float elev = std::min(40.0f, (minute - rise) * 0.16f) * kPi / 180;
    const float az = (60.0f + (minute - rise) * 0.12f) * kPi / 180;  // clockwise from north
    const float d = 300.0f;
    StrataPoint q{};
    q.position[0] = eye.x + d * std::cos(elev) * std::sin(az);
    q.position[1] = eye.y + d * std::sin(elev);
    q.position[2] = eye.z - d * std::cos(elev) * std::cos(az);
    q.size_pixels = 4.0f;
    q.world_diameter = 2.8f;
    q.color[0] = 0.95f;
    q.color[1] = 0.93f;
    q.color[2] = 0.85f;
    q.color[3] = 1.0f;
    engine.submitPoints(&q, 1);
}

struct Place {
    const char* name;
    Vec3 at;
    float range;
};

std::vector<Place> places(const World& w) {
    std::vector<Place> out;
    if (w.bagDown) out.push_back({"Your candy bag", {w.bag.x, w.bag.y + 0.9f, w.bag.z}, 40.0f});
    for (int i = 0; i < T::HouseCount; ++i) {
        const T::House& h = T::kHouses[i];
        const Vec2 d = T::doorAt(static_cast<T::HouseId>(i));
        out.push_back({h.name, {d.x, 3.2f, h.north ? d.z + 1.5f : d.z - 1.5f}, kLabelRange});
    }
    out.push_back({"The bandstand", {T::kBandstand.x, 4.5f, T::kBandstand.z}, kLabelRange});
    out.push_back({"St. Brendan's churchyard", {(T::kLychgateX0 + T::kLychgateX1) / 2, 4.0f, T::kChurchyard.z1}, 30.0f});
    out.push_back({"The creek bridge", {(T::kBridge.x0 + T::kBridge.x1) / 2, 2.0f, 0.0f}, kLabelRange});
    out.push_back({"The orchard", {T::kOrchard.x0 + 2, T::groundHeight(T::kOrchard.x0, T::kOrchard.z0 + 2) + 3.0f, T::kOrchard.z0 + 2}, 26.0f});
    out.push_back({"Lantern Hill", {T::kHill.x, T::kHillHeight + 3.5f, T::kHill.z}, 40.0f});
    return out;
}

bool toScreen(const StrataCamera& cam, Vec3 p, int32_t width, int32_t height, float& x, float& y) {
    float v[4], c[4];
    const float in[4] = {p.x, p.y, p.z, 1};
    for (int r = 0; r < 4; ++r) v[r] = cam.view[r] * in[0] + cam.view[4 + r] * in[1] + cam.view[8 + r] * in[2] + cam.view[12 + r] * in[3];
    for (int r = 0; r < 4; ++r)
        c[r] = cam.projection[r] * v[0] + cam.projection[4 + r] * v[1] + cam.projection[8 + r] * v[2] + cam.projection[12 + r] * v[3];
    if (c[3] <= 0.1f) return false;
    x = (c[0] / c[3] * 0.5f + 0.5f) * static_cast<float>(width);
    y = (0.5f - c[1] / c[3] * 0.5f) * static_cast<float>(height);
    return x > 0 && x < static_cast<float>(width) && y > 0 && y < static_cast<float>(height);
}

}  // namespace

void setLighting(strata::Engine& engine, bool lightweight) {
    StrataLighting light = strata::lightingDefaults(STRATA_LIGHTING_SUN);
    light.cascade_count = lightweight ? 3 : 4;
    light.shadow_map_size = lightweight ? 1024 : 2048;
    light.shadow_distance = 70.0f;
    light.fog = 1;
    engine.setLighting(light);
}

float setSky(strata::Engine& engine, const World& w) {
    const StrataSolarPosition sun = strata::solarPosition(kFirstMidnightUtc + gameMinutes(w) * 60.0, kLatitude, kLongitude);
    engine.setSunDirection(sun.to_sun);
    return sun.elevation_degrees;
}

void setLights(strata::Engine& engine, const World& w, float sunElevation) {
    struct Candidate {
        StrataPointLight light;
        float d2;
    };
    std::vector<Candidate> c;
    const Vec3 eye = eyePosition(w);
    auto add = [&](Vec3 at, float range, bridge::Rgb color, float intensity) {
        StrataPointLight l{};
        l.position[0] = at.x;
        l.position[1] = at.y;
        l.position[2] = at.z;
        l.range = range;
        l.color[0] = color.r;
        l.color[1] = color.g;
        l.color[2] = color.b;
        l.intensity = intensity;
        const float dx = at.x - eye.x, dz = at.z - eye.z;
        c.push_back({l, dx * dx + dz * dz});
    };
    const float night = nightness(sunElevation);
    const float lit = 0.35f + 0.65f * night;
    for (int i = 0; i < T::HouseCount; ++i) {
        const auto id = static_cast<T::HouseId>(i);
        const bool north = T::kHouses[i].north;
        if (porchLightOn(w, id)) {
            const Vec3 l = T::porchLight(id);
            add({l.x, l.y - 0.2f, l.z + (north ? 0.6f : -0.6f)}, 10.0f, kPorchColor, 1.6f * lit);
        }
        const auto p = bridge::pumpkins(id);
        if (!p.empty()) {
            const Vec3 m{(p[0].x + p[1].x) / 2, p[0].y + 0.4f, p[0].z + (north ? 0.6f : -0.6f)};
            add(m, 5.0f, kPumpkinColor, 0.9f * lit);
        }
    }
    for (Vec2 p : T::kStreetlamps) add({p.x, T::kStreetlampHeight - 0.5f, p.z}, 14.0f, kStreetColor, 1.3f * lit);
    if (w.form == Form::Cat) add(lamplighterLamp(w), 8.0f, kStreetColor, 0.9f * lit);
    for (int i = 0; i < kLanternCount; ++i)
        if (w.lit[static_cast<size_t>(i)]) {
            const Vec3 a = lantern(i).at;
            add({a.x, a.y + 0.3f, a.z}, 6.0f, kLanternColor, 1.1f * lit);
        }
    std::sort(c.begin(), c.end(), [](const Candidate& a, const Candidate& b) { return a.d2 < b.d2; });
    std::vector<StrataPointLight> lights;
    for (size_t i = 0; i < c.size() && lights.size() < 30; ++i) lights.push_back(c[i].light);
    // A faint fill from the night sky, so shapes keep their edges in the dark.
    if (night > 0) {
        StrataPointLight l{};
        l.position[0] = eye.x;
        l.position[1] = eye.y + 50.0f;
        l.position[2] = eye.z;
        l.range = 160.0f;
        l.color[0] = 0.5f;
        l.color[1] = 0.6f;
        l.color[2] = 1.0f;
        l.intensity = 0.7f * night;
        lights.push_back(l);
    }
    engine.setPointLights(lights.empty() ? nullptr : lights.data(), static_cast<uint32_t>(lights.size()));
}

std::vector<HudLine> hudLines(const World& w) {
    std::vector<HudLine> lines;
    lines.push_back({"31 October  " + clockText(w), true});
    lines.push_back({"Candy     " + std::to_string(w.candy) + (w.bagDown ? "  (bag on the ground)" : "")});
    if (w.form == Form::Girl) lines.push_back({std::string("Costume   ") + costumeShort(w.costume)});
    lines.push_back({std::string("You are   ") + (w.form == Form::Cat ? "a black cat  (Q: Wren)" : "Wren  (Q: a cat)")});
    if (w.spiritsSeen) lines.push_back({"Spirits   " + std::to_string(spiritsSeen(w)) + " of " + std::to_string(SpiritCount) + " met"});
    lines.push_back({"Lanterns  " + lanternLine(w)});
    lines.push_back({"Basket    " + basketLine(w)});
    lines.push_back({"Smoke     " + smokeLine(w)});
    lines.push_back({""});
    lines.push_back({"NEXT", true});
    lines.push_back({tonightNext(w)});
    if (std::string also = tonightAlso(w); !also.empty()) lines.push_back({also});
    return lines;
}

std::string prompt(const World& w) {
    if (std::string b = catPrompt(w); !b.empty()) return b;  // the bag, a latch (M3)
    if (std::string c = craftPrompt(w); !c.empty()) return c;
    // A door's own line, unless it only says something (Nana's door, which does
    // nothing, or the costume line once one is chosen) and something usable is
    // straight ahead, out of reach.
    const std::string u = usePrompt(w);
    const bool weak = canChooseCostume(w) ? w.costume != Costume::None : atDoor(w) == T::Nana;
    if (!u.empty() && !weak) return u;
    const std::string h = reachHint(w);
    return h.empty() ? u : h;
}

namespace {

// Whether E (or T) does something here: the prompt offers it.
bool usable(const std::string& p) { return p.rfind("E  ", 0) == 0 || p.rfind("T  ", 0) == 0; }

}  // namespace

std::vector<std::string> wrapped(const std::string& text, size_t columns) {
    std::vector<std::string> out;
    for (size_t at = 0; at < text.size();) {
        size_t end = text.find('\n', at);
        if (end == std::string::npos) end = text.size();
        std::string line;
        for (size_t i = at; i < end;) {
            size_t space = text.find(' ', i);
            if (space == std::string::npos || space > end) space = end;
            const std::string word = text.substr(i, space - i);
            if (!line.empty() && line.size() + 1 + word.size() > columns) {
                out.push_back(line);
                line.clear();
            }
            line += (line.empty() ? "" : " ") + word;
            i = space + 1;
        }
        out.push_back(line);
        at = end + 1;
    }
    return out;
}

void render(strata::Engine& engine, const World& w, bool help, float eyeY, bool noText, const std::string& message,
            Cover cover, int notebookPage) {
    const float elevation = setSky(engine, w);
    setLights(engine, w, elevation);
    int32_t width = 0, height = 0;
    engine.drawableSize(width, height);
    const float fit = height < 600 ? 0.75f : 1.0f;
    gTextSize = std::round(16.0f * fit);
    gLine = std::round(20.0f * fit);
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    Vec3 eye = eyePosition(w);
    eye.y = eyeY;
    const StrataCamera camera = lookFrom(eye, w.player.yaw, w.player.pitch, aspect, w.form == Form::Cat ? 0.03f : 0.05f);
    engine.setCamera(camera);

    StrataFrameDesc fd{};
    fd.clear_flags = STRATA_CLEAR_COLOR | STRATA_CLEAR_DEPTH | STRATA_CLEAR_SKY;
    engine.beginFrame(fd);
    const float night = nightness(elevation);
    drawFlames(engine, w, night);
    drawMoon(engine, w, eye);

    std::vector<StrataText> texts;
    std::vector<std::string> keep;
    keep.reserve(48);
    const float ink[4] = {0.95f, 0.92f, 0.85f, 1.0f};
    const float lamp[4] = {1.0f, 0.55f, 0.12f, 1.0f};
    float y = kMargin;
    std::vector<float> hudRight;
    // Long HUD lines (the NEXT line) wrap within the left part of the screen.
    std::vector<HudLine> shown;
    const auto hudColumns = static_cast<size_t>(std::max(24.0f, (static_cast<float>(width) * 0.62f - kMargin) / gTextSize));
    for (const HudLine& l : hudLines(w)) {
        if (l.text.size() <= hudColumns) {
            shown.push_back(l);
            continue;
        }
        for (const std::string& part : wrapped(l.text, hudColumns)) shown.push_back({part, l.heading});
    }
    for (const HudLine& l : shown) {
        hudRight.push_back(noText ? 0 : kMargin + gTextSize * static_cast<float>(l.text.size()));
        if (!l.text.empty()) text(texts, keep, kMargin, y, l.text, l.heading ? lamp : ink);
        y += gLine;
    }
    const size_t hudTexts = texts.size();
    // The names of places nearby, fading with distance.
    for (const Place& place : places(w)) {
        const float dx = place.at.x - eye.x, dz = place.at.z - eye.z;
        const float d = std::sqrt(dx * dx + dz * dz);
        float sx = 0, sy = 0;
        if (d < kLabelNear || d > place.range || !toScreen(camera, place.at, width, height, sx, sy)) continue;
        const float fade[4] = {ink[0], ink[1], ink[2], clampf((place.range - d) / 4.0f, 0.0f, 0.8f)};
        const std::string name = place.name;
        const float wide = gTextSize * static_cast<float>(name.size());
        const float left = clampf(sx - wide / 2, kMargin, static_cast<float>(width) - kMargin - wide);
        bool covered = false;
        for (size_t i = 0; i < hudRight.size(); ++i) {
            const float top = kMargin + gLine * static_cast<float>(i);
            if (sy + gTextSize > top && sy < top + gTextSize && hudRight[i] + gTextSize > left) covered = true;
        }
        if (left < cover.right && sy < cover.bottom) covered = true;
        if (!covered) text(texts, keep, left, sy, name, fade);
    }
    // Where the NEXT line points (#14): a "v" over the place, with how far it is,
    // or an arrow at the side of the screen to turn toward it.
    if (const NextStep n = tonightStep(w); n.goal) {
        const float dx = n.at.x - eye.x, dz = n.at.z - eye.z;
        const float d = std::sqrt(dx * dx + dz * dz);
        if (d > kGoalNear) {
            const float goal[4] = {0.75f, 0.85f, 1.0f, 0.95f};
            const std::string far = std::to_string(static_cast<int>(d + 0.5f)) + " m";
            float sx = 0, sy = 0;
            if (toScreen(camera, n.at, width, height, sx, sy)) {
                text(texts, keep, sx - gTextSize / 2, sy - gTextSize * 1.6f, "v", goal);
                text(texts, keep, sx - gTextSize * 0.5f * static_cast<float>(far.size()), sy - gTextSize * 2.8f, far, goal);
            } else {
                // Which way to turn: the bearing to the place, against the way she faces.
                float turn = std::atan2(dx, -dz) - w.player.yaw;
                while (turn > kPi) turn -= 2 * kPi;
                while (turn < -kPi) turn += 2 * kPi;
                const float ay = std::round(static_cast<float>(height) * 0.58f);
                if (turn < 0) {
                    text(texts, keep, kMargin, ay, "<", goal);
                    text(texts, keep, kMargin + gTextSize * 1.5f, ay, far, goal);
                } else {
                    const float right = static_cast<float>(width) - kMargin - gTextSize;
                    text(texts, keep, right, ay, ">", goal);
                    text(texts, keep, right - gTextSize * (0.5f + static_cast<float>(far.size())), ay, far, goal);
                }
            }
        }
    }
    const size_t labelsEnd = texts.size();
    const std::string here = prompt(w);
    // The crosshair turns lantern-orange when E (or T) does something here.
    text(texts, keep, static_cast<float>(width) / 2 - gTextSize / 2, static_cast<float>(height) / 2 - gTextSize / 2, "+",
         usable(here) ? lamp : ink);
    const auto columns = static_cast<size_t>(std::max(16.0f, (static_cast<float>(width) - 2 * kMargin) / gTextSize));
    auto centred = [&](const std::string& s, float row, const float* colour) {
        for (const std::string& line : wrapped(s, columns)) {
            text(texts, keep, static_cast<float>(width) / 2 - gTextSize * 0.5f * static_cast<float>(line.size()), row, line, colour);
            row += gLine;
        }
        return row;
    };
    float py = static_cast<float>(height) / 2 + 2 * gLine;
    if (!here.empty()) py = centred(here, py, ink);
    if (!message.empty()) centred(message, std::max(static_cast<float>(height) / 2 + 4 * gLine, py + gLine), lamp);
    if (help) {
        std::vector<std::string> items = {"W A S D walk", "Shift hurry", "Mouse look", "E knock / use", "T tell a joke", "1-4 costume", "Q cat / Wren",
                                          "Tab notebook", "M sound", "Esc free the mouse", "F1 hide help"};
        std::vector<std::string> lines(1);
        for (const std::string& item : items) {
            if (!lines.back().empty() && lines.back().size() + 3 + item.size() > columns) lines.emplace_back();
            lines.back() += (lines.back().empty() ? "" : "   ") + item;
        }
        float bottom = static_cast<float>(height) - kMargin - gTextSize;
        for (size_t i = lines.size(); i-- > 0;) {
            text(texts, keep, kMargin, bottom, lines[i], ink);
            bottom -= gLine;
        }
    }
    // Wren's notebook, open over the night (Tab): one page at a time.
    const std::vector<const Page*> pages = notebook(w);
    if (!noText && notebookPage >= 0) {
        texts.clear();
        keep.clear();
        const float pw = std::min(static_cast<float>(width) - 2 * kMargin, gTextSize * 64 + 2 * kMargin);
        const auto cols = static_cast<size_t>((pw - 2 * kMargin) / gTextSize);
        const float px = (static_cast<float>(width) - pw) / 2;
        float ty = kMargin * 2;
        std::vector<std::string> body;
        std::string title = "Wren's notebook";
        if (pages.empty()) {
            body = wrapped("Nothing yet. Pages fill in as the night goes: light Nana's window lantern first.", cols);
        } else {
            const Page& pg = *pages[static_cast<size_t>(notebookPage) % pages.size()];
            title = pg.title;
            body = wrapped(pg.text, cols);
        }
        const std::string foot = "Page " + std::to_string(pages.empty() ? 0 : notebookPage % static_cast<int>(pages.size()) + 1) +
                                 " of " + std::to_string(pages.size()) + "   Secrets " + std::to_string(secretsFound(w)) + " of " +
                                 std::to_string(SecretCount) + "   Left/Right turn   Tab close";
        const float ph = gLine * static_cast<float>(body.size() + 4) + 2 * kMargin;
        StrataRect r{};
        r.x = px;
        r.y = ty - kMargin;
        r.width = pw;
        r.height = std::min(ph, static_cast<float>(height) - ty);
        r.color[0] = 0.07f;
        r.color[1] = 0.05f;
        r.color[2] = 0.08f;
        r.color[3] = 0.9f;
        engine.submitRects(&r, 1);
        text(texts, keep, px + kMargin, ty, title, lamp);
        ty += gLine * 2;
        for (const std::string& line : body) {
            text(texts, keep, px + kMargin, ty, line, ink);
            ty += gLine;
        }
        ty += gLine;
        for (const std::string& line : wrapped(foot, cols)) {
            const float dim[4] = {0.7f, 0.64f, 0.62f, 1.0f};
            text(texts, keep, px + kMargin, ty, line, dim);
            ty += gLine;
        }
    }
    for (size_t i = 0; i < texts.size(); ++i) texts[i].utf8 = keep[i].c_str();
    if (!noText) engine.submitText(texts.data(), texts.size());
    else if (labelsEnd > hudTexts) engine.submitText(texts.data() + hudTexts, labelsEnd - hudTexts);
    engine.endFrame();
}

}  // namespace familiar::play
