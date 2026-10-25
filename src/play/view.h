// What the player sees each frame: the sky at the hour, the camera at the
// eyes (Wren's or the cat's), the lights of the town, and the HUD. (Atman's
// view, for Hollowell.)
#pragma once

#include <string>
#include <vector>

#include "bridge/scene.h"
#include "sim/world.h"
#include "strata/strata.hpp"

namespace familiar::play {

// Hollowell's place and date, for the sun: about 42.4 N, 72.5 W (a hill town
// in western Massachusetts), on 31 October 2026, Eastern Daylight Time.
constexpr double kLatitude = 42.4;
constexpr double kLongitude = -72.5;
constexpr double kFirstMidnightUtc = 1793419200.0;  // 2026-10-31 00:00 at UTC-4

void setLighting(strata::Engine& engine, bool lightweight = false);
float setSky(strata::Engine& engine, const World& w);
void setLights(strata::Engine& engine, const World& w, float sunElevation);

struct HudLine {
    std::string text;
    bool heading = false;
};
std::vector<HudLine> hudLines(const World& w);
std::string prompt(const World& w);

struct Cover {
    float right = 0, bottom = 0;  // in canvas pixels
};

// Draws one frame (not presented). `eyeY` is the camera's eye height, eased
// by the session so a cat's drop from a roof is not a jump cut.
void render(strata::Engine& engine, const World& w, bool help, float eyeY, bool noText = false,
            const std::string& message = "", Cover cover = {});

std::vector<std::string> wrapped(const std::string& text, size_t columns);

}  // namespace familiar::play
