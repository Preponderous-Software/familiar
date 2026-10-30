#include "play/view.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "bridge/geometry.h"
#include "sim/cat.h"
#include "sim/doors.h"
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
constexpr float kLabelRange = 22.0f;
constexpr float kLabelNear = 3.0f;

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

std::vector<Place> places() {
    std::vector<Place> out;
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
    lines.push_back({""});
    lines.push_back({"NEXT", true});
    lines.push_back({nextStep(w)});
    return lines;
}

std::string prompt(const World& w) { return usePrompt(w); }

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
            Cover cover) {
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
    for (const HudLine& l : hudLines(w)) {
        hudRight.push_back(noText ? 0 : kMargin + gTextSize * static_cast<float>(l.text.size()));
        if (!l.text.empty()) text(texts, keep, kMargin, y, l.text, l.heading ? lamp : ink);
        y += gLine;
    }
    const size_t hudTexts = texts.size();
    // The names of places nearby, fading with distance.
    for (const Place& place : places()) {
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
    const size_t labelsEnd = texts.size();
    text(texts, keep, static_cast<float>(width) / 2 - gTextSize / 2, static_cast<float>(height) / 2 - gTextSize / 2, "+", ink);
    const auto columns = static_cast<size_t>(std::max(16.0f, (static_cast<float>(width) - 2 * kMargin) / gTextSize));
    auto centred = [&](const std::string& s, float row, const float* colour) {
        for (const std::string& line : wrapped(s, columns)) {
            text(texts, keep, static_cast<float>(width) / 2 - gTextSize * 0.5f * static_cast<float>(line.size()), row, line, colour);
            row += gLine;
        }
        return row;
    };
    float py = static_cast<float>(height) / 2 + 2 * gLine;
    const std::string here = prompt(w);
    if (!here.empty()) py = centred(here, py, ink);
    if (!message.empty()) centred(message, std::max(static_cast<float>(height) / 2 + 4 * gLine, py + gLine), lamp);
    if (help) {
        std::vector<std::string> items = {"W A S D walk", "Shift hurry", "Mouse look", "E knock / pick up / open", "T tell a joke", "1-4 costume", "Q cat / Wren", "Esc free the mouse",
                                          "F1 hide help"};
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
    for (size_t i = 0; i < texts.size(); ++i) texts[i].utf8 = keep[i].c_str();
    if (!noText) engine.submitText(texts.data(), texts.size());
    else if (labelsEnd > hudTexts) engine.submitText(texts.data() + hudTexts, labelsEnd - hudTexts);
    engine.endFrame();
}

}  // namespace familiar::play
