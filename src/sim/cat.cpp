#include "sim/cat.h"

#include <cmath>

namespace familiar {

namespace T = familiar::town;

namespace {

float dist2(float ax, float az, float bx, float bz) {
    const float dx = ax - bx, dz = az - bz;
    return dx * dx + dz * dz;
}

// Back and forth along [0, length] at `speed`, starting `offset` along.
float pingPong(float seconds, float speed, float offset, float length) {
    float s = offset + speed * seconds;
    const float loop = 2 * length;
    s = s - loop * static_cast<float>(static_cast<int>(s / loop));
    return s < length ? s : loop - s;
}

// The lamplighter's path: from the bridge's east end up to the circle on
// Lantern Hill (the same line the terrain draws as the hill path).
constexpr Vec2 kPathA{60.0f, -2.0f};
constexpr Vec2 kPathB{T::kHill.x - 4.0f, T::kHill.z + 5.0f};

const char* const kSpiritText[SpiritCount] = {
    "A woman in a long coat walks between the stones, reading the names as if greeting old friends. "
    "She smiles at you. The town's own dead are visiting tonight.",
    "An old farmer in a flat cap leans on the churchyard wall, looking toward the orchard he planted. "
    "He nods to you, the way people nod to a neighbour's cat.",
    "A man with a fiddle under his arm stands by the tower, listening to the children out on Elm Street. "
    "He taps his foot in time.",
    "An old lamplighter walks the hill path with his long pole and his lamp, lighting the way up Lantern Hill "
    "as he did for fifty years. He tips his cap to you.",
};

}  // namespace

bool nearBag(const World& w) {
    if (!w.bagDown || w.form != Form::Girl) return false;
    if (w.player.feet > w.bag.y + 0.6f || w.player.feet < w.bag.y - 0.6f) return false;
    return dist2(w.player.x, w.player.z, w.bag.x, w.bag.z) < kBagReach * kBagReach;
}

int latchHere(const World& w) {
    if (w.form != Form::Cat) return -1;
    const Player& p = w.player;
    if (!w.gateOpen[T::HaleGate] && T::kHaleYard.contains(p.x, p.z) && p.x > T::kHaleYard.x0 &&
        dist2(p.x, p.z, T::kHaleYard.x0, T::kHaleGateZ) < kLatchReach * kLatchReach)
        return T::HaleGate;
    const float gx = (T::kLychgateX0 + T::kLychgateX1) / 2, gz = T::kChurchyard.z1;
    if (!w.gateOpen[T::Lychgate] && p.z < gz && T::kChurchyard.contains(p.x, p.z) &&
        dist2(p.x, p.z, gx, gz) < (kLatchReach + 0.5f) * (kLatchReach + 0.5f))
        return T::Lychgate;
    return -1;
}

int latchedOutside(const World& w) {
    const Player& p = w.player;
    if (!w.gateOpen[T::HaleGate] && p.x < T::kHaleYard.x0 && dist2(p.x, p.z, T::kHaleYard.x0, T::kHaleGateZ) < 2.0f * 2.0f)
        return T::HaleGate;
    const float gx = (T::kLychgateX0 + T::kLychgateX1) / 2, gz = T::kChurchyard.z1;
    if (!w.gateOpen[T::Lychgate] && p.z > gz && dist2(p.x, p.z, gx, gz) < 2.4f * 2.4f) return T::Lychgate;
    return -1;
}

std::vector<Walker> spirits(const World& w) {
    std::vector<Walker> out(SpiritCount);
    const float seconds = static_cast<float>(w.tick) / kTickRate;
    const bool cat = w.form == Form::Cat;
    for (Walker& s : out) s.shown = cat;
    // The reader walks the rows between the first two lines of stones, slowly.
    {
        Walker& s = out[Reader];
        const float x0 = 13.0f, x1 = 39.0f;
        const float along = pingPong(seconds, 0.35f, 4.0f, x1 - x0);
        const float back = pingPong(seconds + 0.5f, 0.35f, 4.0f, x1 - x0) < along;
        s.at = {x0 + along, T::groundHeight(x0 + along, -42.9f), -42.9f};
        s.yaw = back ? -kPi / 2 : kPi / 2;
        s.height = 1.66f;
        s.look = Reader;
    }
    // The farmer leans on the east wall by the hedge, looking toward the orchard.
    {
        Walker& s = out[Farmer];
        s.at = {39.6f, T::groundHeight(39.6f, -35.5f), -35.5f};
        s.yaw = kPi / 2 + 0.25f * detSin(seconds * 0.2f);
        s.height = 1.76f;
        s.look = Farmer;
    }
    // The fiddler stands by the tower, facing the town.
    {
        Walker& s = out[Fiddler];
        s.at = {21.0f, T::groundHeight(21.0f, -47.6f), -47.6f};
        s.yaw = kPi;
        s.at.y += 0.02f * detSin(seconds * 6.0f);  // tapping his foot
        s.height = 1.72f;
        s.look = Fiddler;
    }
    // The lamplighter walks the hill path, up and down.
    {
        Walker& s = out[Lamplighter];
        const float dx = kPathB.x - kPathA.x, dz = kPathB.z - kPathA.z;
        const float len = std::sqrt(dx * dx + dz * dz);
        const float along = pingPong(seconds, 0.6f, 10.0f, len);
        const bool up = pingPong(seconds + 0.5f, 0.6f, 10.0f, len) > along;
        const float k = along / len;
        const float x = kPathA.x + dx * k, z = kPathA.z + dz * k;
        s.at = {x, T::groundHeight(x, z), z};
        s.yaw = up ? detAtan2(dx, -dz) : detAtan2(-dx, dz);
        s.height = 1.74f;
        s.look = Lamplighter;
    }
    return out;
}

int spiritNear(const World& w) {
    if (w.form != Form::Cat) return -1;
    int best = -1;
    float bestD = kSpiritSight * kSpiritSight;
    const std::vector<Walker> all = spirits(w);
    for (int i = 0; i < SpiritCount; ++i) {
        const float d = dist2(w.player.x, w.player.z, all[static_cast<size_t>(i)].at.x, all[static_cast<size_t>(i)].at.z);
        if (d < bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

const char* spiritText(int i) { return i >= 0 && i < SpiritCount ? kSpiritText[i] : ""; }

int spiritsSeen(const World& w) {
    int n = 0;
    for (int i = 0; i < SpiritCount; ++i)
        if (w.spiritsSeen & (1u << i)) ++n;
    return n;
}

Vec3 lamplighterLamp(const World& w) {
    const Walker s = spirits(w)[Lamplighter];
    const float sy = detSin(s.yaw), cy = detCos(s.yaw);
    // Held out ahead and to the right, at shoulder height.
    return {s.at.x + sy * 0.4f + cy * 0.3f, s.at.y + 1.35f, s.at.z - cy * 0.4f + sy * 0.3f};
}

std::string catPrompt(const World& w) {
    if (nearBag(w)) return "E  pick up your candy bag";
    switch (latchHere(w)) {
        case T::HaleGate: return "E  nose up the latch";
        case T::Lychgate: return "E  nudge the bolt back with your paw";
        default: break;
    }
    switch (latchedOutside(w)) {
        case T::HaleGate:
            return w.form == Form::Cat ? "Latched on the inside. Over the fence, from the woodpile?"
                                       : "Mr. Hale's gate is latched on the inside. A cat might find a way in";
        case T::Lychgate:
            return w.form == Form::Cat ? "Bolted on the inside. Is there a gap in the hedge?"
                                       : "The lychgate is bolted for the night. A cat might find a way in";
        default: break;
    }
    return "";
}

std::string useLabel(const World& w) {
    if (nearBag(w)) return "Pick up";
    if (latchHere(w) >= 0) return "Open";
    return w.form == Form::Cat ? "Meow" : "Knock";
}

bool catTick(World& w, const Input& in, Form before) {
    Player& p = w.player;
    if (before == Form::Girl && w.form == Form::Cat) {
        // The cat can't carry the bag: it stays here until Wren comes back for it.
        w.bagDown = true;
        w.bag = {p.x, p.feet, p.z};
    } else if (before == Form::Cat && w.form == Form::Girl && nearBag(w)) {
        w.bagDown = false;  // changed back beside it: she picks it straight up
    }
    if (const int s = spiritNear(w); s >= 0) w.spiritsSeen |= 1u << s;
    if (!in.use) return false;
    if (nearBag(w)) {
        w.bagDown = false;
        return true;
    }
    if (const int g = latchHere(w); g >= 0) {
        w.gateOpen[static_cast<size_t>(g)] = true;
        return true;
    }
    return false;
}

}  // namespace familiar
