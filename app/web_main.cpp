// Familiar in a browser: the same game on Strata's WebGL 2 backend
// (Emscripten), with the page's touch controls (web/shell.html). Atman's
// web_main, cut down: no saves yet, so a reload starts the night over.
#include <cinttypes>
#include <cstdio>
#include <memory>
#include <string>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include "play/session.h"
#include "play/view.h"
#include "sim/doors.h"
#include "strata/strata.hpp"

using namespace familiar;

namespace {

struct Game {
    std::unique_ptr<strata::Engine> engine;
    std::unique_ptr<play::Session> session;
    double last = 0;
    std::string statusJson;
};

Game* g = nullptr;

void frame(void*) {
    StrataEvent e;
    while (g->engine->pollEvent(e)) g->session->handle(e);
    const double now = emscripten_get_now() / 1000.0;
    g->session->frame(now - g->last);
    g->last = now;
    g->engine->present();
}

void jsonString(std::string& out, const std::string& s) {
    out += '"';
    for (char ch : s) {
        if (ch == '"' || ch == '\\') {
            out += '\\';
            out += ch;
        } else if (static_cast<unsigned char>(ch) < 0x20) {
            out += ' ';
        } else {
            out += ch;
        }
    }
    out += '"';
}

void jsonBool(std::string& out, const char* key, bool v) {
    out += ",\"";
    out += key;
    out += v ? "\":true" : "\":false";
}

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE void familiar_look(double dx, double dy) {
    if (g) g->session->look(static_cast<float>(dx), static_cast<float>(dy));
}
EMSCRIPTEN_KEEPALIVE void familiar_stick(double forward, double strafe) {
    if (g) g->session->setStick(static_cast<float>(forward), static_cast<float>(strafe));
}
EMSCRIPTEN_KEEPALIVE void familiar_press(int scancode) {
    if (g) g->session->press(scancode);
}
EMSCRIPTEN_KEEPALIVE void familiar_captured(int on) {
    if (g) g->session->setCaptured(on != 0);
}
EMSCRIPTEN_KEEPALIVE void familiar_restart() {
    if (g) g->session->restart();
}
EMSCRIPTEN_KEEPALIVE void familiar_to_doorstep(int house) {
    if (g) g->session->toDoorstep(house);
}
EMSCRIPTEN_KEEPALIVE void familiar_cover(double right, double bottom) {
    if (g) g->session->setCover(static_cast<float>(right), static_cast<float>(bottom));
}
EMSCRIPTEN_KEEPALIVE void familiar_help(int on) {
    if (g) g->session->setHelp(on != 0);
}
EMSCRIPTEN_KEEPALIVE void familiar_resize(double w, double h, int compact) {
    if (!g || w < 1 || h < 1) return;
    g->engine->resize(static_cast<int32_t>(w), static_cast<int32_t>(h));
    g->session->setCompactHud(compact != 0);
}
EMSCRIPTEN_KEEPALIVE const char* familiar_status() {
    if (!g) return "{}";
    const play::Session& s = *g->session;
    const World& w = s.world();
    std::string o = "{\"clock\":";
    jsonString(o, clockText(w));
    char buf[160];
    std::snprintf(buf, sizeof buf, ",\"tick\":%" PRId64 ",\"x\":%.3f,\"z\":%.3f,\"feet\":%.3f,\"yaw\":%.3f,\"candy\":%d", w.tick,
                  static_cast<double>(w.player.x), static_cast<double>(w.player.z), static_cast<double>(w.player.feet),
                  static_cast<double>(w.player.yaw), static_cast<int>(w.candy));
    o += buf;
    jsonBool(o, "captured", s.captured());
    jsonBool(o, "help", s.help());
    jsonBool(o, "cat", w.form == Form::Cat);
    o += ",\"message\":";
    jsonString(o, s.message());
    o += ",\"prompt\":";
    jsonString(o, play::prompt(w));
    o += ",\"hud\":[";
    bool first = true;
    for (const play::HudLine& l : play::hudLines(w)) {
        if (!first) o += ',';
        first = false;
        o += "{\"heading\":";
        o += l.heading ? "true" : "false";
        o += ",\"text\":";
        jsonString(o, l.text);
        o += '}';
    }
    o += "]}";
    g->statusJson = std::move(o);
    return g->statusJson.c_str();
}

}  // extern "C"

int main() {
    static Game game;
    g = &game;
    double cw = 0, ch = 0;
    emscripten_get_element_css_size("#canvas", &cw, &ch);
    if (cw < 1 || ch < 1) {
        cw = 1280;
        ch = 720;
    }
    strata::EngineDesc desc;
    desc.surface = STRATA_SURFACE_WINDOW;
    desc.width = static_cast<int32_t>(cw);
    desc.height = static_cast<int32_t>(ch);
    desc.title = "Familiar";
    try {
        g->engine = strata::Engine::create(desc);
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "familiar: cannot start the renderer: %s\n", ex.what());
        std::string js = "if (Module.onRendererFailed) Module.onRendererFailed(";
        jsonString(js, ex.what());
        js += ")";
        emscripten_run_script(js.c_str());
        return 1;
    }
    play::setLighting(*g->engine, true);
    play::SessionOptions o;
    o.browser = true;
    g->session = std::make_unique<play::Session>(*g->engine, o);
    g->last = emscripten_get_now() / 1000.0;
    emscripten_run_script("if (Module.onGameReady) Module.onGameReady()");
    emscripten_set_main_loop_arg(frame, nullptr, 0, false);
    return 0;
}
