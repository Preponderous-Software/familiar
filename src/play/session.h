// A game being played: the world, the scene, the input and the messages, one
// frame at a time. The desktop game (app/main.cpp) drives it from its own
// loop; the browser game (app/web_main.cpp) from the browser's. (Atman's
// session, cut down: no saves yet.)
#pragma once

#include <string>

#include "bridge/scene.h"
#include "play/view.h"
#include "sim/world.h"
#include "strata/strata.hpp"

namespace familiar::play {

struct SessionOptions {
    uint64_t seed = 1;
    bool browser = false;  // the page owns the pointer lock and the canvas size; Esc never quits
};

class Session {
public:
    Session(strata::Engine& engine, const SessionOptions& options);

    void handle(const StrataEvent& e);
    void frame(double dt);
    bool running() const { return running_; }
    void restart();
    // Stand on a house's doorstep, facing the door: for the browser smoke test,
    // whose software-rendered frames are too slow to steer there.
    void toDoorstep(int house);

    void look(float dx, float dy);
    void setStick(float forward, float strafe);
    void press(int scancode);
    void setCaptured(bool on) { captured_ = on; }
    void setHelp(bool on) { help_ = on; }
    void setCompactHud(bool on) { compact_ = on; }
    void setCover(float right, float bottom) { cover_ = {right, bottom}; }

    const World& world() const { return w_; }
    bool captured() const { return captured_; }
    bool help() const { return help_; }
    const std::string& message() const { return message_; }

private:
    void capture(bool on);
    void onKeyDown(int scancode, bool repeat);
    void step();
    void say(const std::string& text, double seconds = 5);

    strata::Engine& engine_;
    SessionOptions options_;
    World w_;
    bridge::Scene scene_;
    static constexpr int kMaxScancode = 512;
    bool keys_[kMaxScancode] = {};
    bool running_ = true, captured_ = false, help_ = true, compact_ = false;
    Cover cover_;
    float yaw_ = 0, pitch_ = 0;
    float stickForward_ = 0, stickStrafe_ = 0;
    Input once_;
    double pending_ = 0, clock_ = 0;
    float eyeY_ = 0;
    std::string message_;
    double messageUntil_ = 0;
    int64_t answerSeen_ = -1;
    Form formSeen_ = Form::Girl;
    Costume costumeSeen_ = Costume::None;
};

}  // namespace familiar::play
