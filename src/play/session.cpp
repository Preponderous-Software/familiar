#include "play/session.h"

#include <algorithm>
#include <cmath>

#include "sim/doors.h"

namespace familiar::play {

namespace {

// SDL scancodes.
constexpr int kW = 26, kA = 4, kS = 22, kD = 7, kE = 8, kQ = 20, kLShift = 225, kRShift = 229, kEscape = 41, kF1 = 58;
constexpr float kLookPerPixel = 0.0025f;
constexpr float kHurryStick = 0.95f;
constexpr const char* kGreeting =
    "Halloween in Hollowell. Nana waves you off from her porch: \"Knock where the porch light's on.\"\n"
    "Q turns you into a cat, and back.";

}  // namespace

Session::Session(strata::Engine& engine, const SessionOptions& options)
    : engine_(engine), options_(options), w_(newWorld(options.seed)), scene_(engine) {
    restart();
    if (!options_.browser) capture(true);
}

void Session::restart() {
    w_ = newWorld(options_.seed);
    yaw_ = w_.player.yaw;
    pitch_ = w_.player.pitch;
    once_ = Input{};
    eyeY_ = eyePosition(w_).y;
    answerSeen_ = -1;
    formSeen_ = w_.form;
    say(kGreeting, 10);
}

void Session::toDoorstep(int house) {
    if (house < 0 || house >= town::HouseCount) return;
    const auto h = static_cast<town::HouseId>(house);
    const Vec2 d = town::doorstep(h), door = town::doorAt(h);
    w_.player.x = d.x;
    w_.player.z = d.z;
    float feet = 0;
    if (standAt(w_, w_.form, d.x, d.z, 10.0f, feet)) w_.player.feet = feet;
    yaw_ = std::atan2(door.x - d.x, -(door.z - d.z));
    w_.player.yaw = yaw_;
}

void Session::say(const std::string& text, double seconds) {
    message_ = text;
    messageUntil_ = clock_ + seconds;
}

void Session::capture(bool on) {
    if (!options_.browser) engine_.setRelativeMouseMode(on);
    captured_ = on;
}

void Session::look(float dx, float dy) {
    yaw_ += dx * kLookPerPixel;
    pitch_ = clampf(pitch_ - dy * kLookPerPixel, -1.45f, 1.45f);
}

void Session::setStick(float forward, float strafe) {
    stickForward_ = clampf(forward, -1.0f, 1.0f);
    stickStrafe_ = clampf(strafe, -1.0f, 1.0f);
}

void Session::press(int scancode) { onKeyDown(scancode, false); }

void Session::onKeyDown(int scancode, bool repeat) {
    if (scancode == kEscape) {
        if (options_.browser) return;
        if (captured_) capture(false);
        else running_ = false;
        return;
    }
    if (scancode == kF1) {
        help_ = !help_;
        return;
    }
    if (repeat) return;
    if (scancode == kE) {
        once_.use = true;
        if (w_.form == Form::Cat && atDoor(w_) < 0) say("A cat can't knock. Q to be Wren again.", 3);
    } else if (scancode == kQ) {
        if (w_.form == Form::Cat && !canBecomeGirl(w_)) say("No room to be a girl here: get down to the ground first.", 4);
        else once_.toggleForm = true;
    }
}

void Session::handle(const StrataEvent& e) {
    switch (e.type) {
        case STRATA_EVENT_QUIT: running_ = false; break;
        case STRATA_EVENT_KEY_DOWN:
            if (e.scancode >= 0 && e.scancode < kMaxScancode) keys_[e.scancode] = true;
            onKeyDown(e.scancode, e.repeat != 0);
            break;
        case STRATA_EVENT_KEY_UP:
            if (e.scancode >= 0 && e.scancode < kMaxScancode) keys_[e.scancode] = false;
            break;
        case STRATA_EVENT_MOUSE_BUTTON_DOWN:
            if (!captured_ && !options_.browser) capture(true);
            break;
        case STRATA_EVENT_MOUSE_MOTION:
            if (captured_ && !options_.browser) look(e.dx, e.dy);
            break;
        case STRATA_EVENT_RESIZED:
            if (!options_.browser) engine_.resize(e.width, e.height);
            break;
        default: break;
    }
}

void Session::step() {
    Input in = once_;
    once_ = Input{};
    const float f = (keys_[kW] ? 1.0f : 0.0f) - (keys_[kS] ? 1.0f : 0.0f) + stickForward_;
    const float s = (keys_[kD] ? 1.0f : 0.0f) - (keys_[kA] ? 1.0f : 0.0f) + stickStrafe_;
    in.forward = clampf(f, -1.0f, 1.0f);
    in.strafe = clampf(s, -1.0f, 1.0f);
    in.hurry = keys_[kLShift] || keys_[kRShift] ||
               stickForward_ * stickForward_ + stickStrafe_ * stickStrafe_ > kHurryStick * kHurryStick;
    in.yaw = yaw_;
    in.pitch = pitch_;
    tick(w_, in);
}

void Session::frame(double dt) {
    clock_ += dt;
    if (!message_.empty() && clock_ > messageUntil_) message_.clear();
    pending_ += dt;
    if (pending_ > 0.25) pending_ = 0.25;
    while (pending_ >= 1.0 / kTickRate) {
        pending_ -= 1.0 / kTickRate;
        step();
    }
    if (w_.answeredTick != answerSeen_) {
        answerSeen_ = w_.answeredTick;
        const std::string a = answerText(w_);
        if (!a.empty()) say(a, kAnswerSeconds);
    }
    if (w_.form != formSeen_) {
        formSeen_ = w_.form;
        say(w_.form == Form::Cat ? "You are a black cat: small, quick, and you can jump onto fences and sheds."
                                 : "You are Wren again.",
            3);
    }
    // The eye eases toward its height, so a change of form or a drop is not a jump cut.
    const float target = eyePosition(w_).y;
    const float k = static_cast<float>(1.0 - std::exp(-dt * 12.0));
    eyeY_ += (target - eyeY_) * k;
    scene_.update(w_);
    render(engine_, w_, help_ && !compact_, eyeY_, compact_, message_, cover_);
}

}  // namespace familiar::play
