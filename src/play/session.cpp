#include "play/session.h"

#include <algorithm>
#include <cmath>

#include "sim/cat.h"
#include "sim/doors.h"

namespace familiar::play {

namespace {

// SDL scancodes.
constexpr int kW = 26, kA = 4, kS = 22, kD = 7, kE = 8, kQ = 20, kT = 23, kOne = 30, kFour = 33, kLShift = 225, kRShift = 229, kEscape = 41, kF1 = 58;
constexpr float kLookPerPixel = 0.0025f;
constexpr float kHurryStick = 0.95f;
constexpr const char* kGreeting =
    "Halloween in Hollowell. Nana waves from her porch: \"Pick a costume, love, and knock where the porch light's on.\"\n"
    "1-4 chooses a costume. Q turns you into a cat, and back.";

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
    costumeSeen_ = w_.costume;
    bagSeen_ = w_.bagDown;
    gatesSeen_ = w_.gateOpen;
    spiritSeen_ = -1;
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

void Session::toSpot(int spot) {
    struct Place {
        float x, z, yaw;
    };
    const float gx = (town::kLychgateX0 + town::kLychgateX1) / 2;
    const Place places[SpotCount] = {
        {-34.9f, town::kHaleGateZ, -kPi / 2},      // inside Mr. Hale's yard, at the latch
        {gx, -30.9f, kPi},                         // inside the churchyard, at the lychgate's bolt
        {35.0f, -37.0f, 1.89f},                    // among the stones, facing the farmer
        {44.0f, town::kHedgeGapZ, -kPi / 2},       // outside the hedge gap
    };
    if (spot < 0 || spot >= SpotCount) return;
    const Place& p = places[spot];
    w_.player.x = p.x;
    w_.player.z = p.z;
    float feet = 0;
    if (standAt(w_, w_.form, p.x, p.z, 0.0f, feet)) w_.player.feet = feet;
    yaw_ = p.yaw;
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
        if (w_.form == Form::Cat && atDoor(w_) < 0 && latchHere(w_) < 0) say("A cat can't knock, or carry things. Q to be Wren again.", 3);
        else if (w_.form == Form::Girl && w_.bagDown && atDoor(w_) >= 0 && !nearBag(w_))
            say("You need your candy bag to trick-or-treat. It's where you became a cat.", 4);
    } else if (scancode == kT) {
        once_.trick = true;
        if (w_.asking < 0) say("Tell a joke (T) when a neighbour asks for a trick.", 3);
    } else if (scancode >= kOne && scancode <= kFour) {
        once_.costume = scancode - kOne + 1;
        if (!canChooseCostume(w_)) say("Costumes are chosen on Nana's walk, before the first knock.", 3);
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
    if (w_.costume != costumeSeen_) {
        costumeSeen_ = w_.costume;
        if (w_.costume != Costume::None) say(std::string("You are dressed as ") + costumeName(w_.costume) + ".", 3);
    }
    if (w_.form != formSeen_) {
        formSeen_ = w_.form;
        if (w_.form == Form::Cat)
            say("You are a black cat: small and quick. Jump onto fences and sheds, slip through gaps. "
                "Your candy bag stays here on the ground; a cat can't carry it.",
                5);
        else say(w_.bagDown ? "You are Wren again. Your candy bag is where you left it." : "You are Wren again.", 3);
        bagSeen_ = w_.bagDown;
    }
    if (w_.bagDown != bagSeen_) {
        bagSeen_ = w_.bagDown;
        if (!w_.bagDown) say("You pick up your candy bag.", 3);
    }
    for (int g = 0; g < town::GateCount; ++g) {
        if (w_.gateOpen[static_cast<size_t>(g)] == gatesSeen_[static_cast<size_t>(g)]) continue;
        gatesSeen_[static_cast<size_t>(g)] = w_.gateOpen[static_cast<size_t>(g)];
        say(g == town::HaleGate ? "Click. The latch lifts and Mr. Hale's gate swings open."
                                : "The old bolt slides back, and the lychgate stands open.",
            4);
    }
    // A spirit the cat meets: what she sees, once each time she comes near.
    const int near = spiritNear(w_);
    if (near != spiritSeen_) {
        spiritSeen_ = near;
        if (near >= 0) say(spiritText(near), 8);
    }
    // The eye eases toward its height, so a change of form or a drop is not a jump cut.
    const float target = eyePosition(w_).y;
    const float k = static_cast<float>(1.0 - std::exp(-dt * 12.0));
    eyeY_ += (target - eyeY_) * k;
    scene_.update(w_);
    render(engine_, w_, help_ && !compact_, eyeY_, compact_, message_, cover_);
}

}  // namespace familiar::play
