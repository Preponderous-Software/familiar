// Saving and continuing.
//
// A save is one file in the slot directory:
//
//   "FAMILIAR"  8 bytes, the magic
//   version     u32 little-endian (kSaveVersion)
//   length      u64 little-endian, bytes of payload
//   payload     the World, field by field, little-endian
//   sha256      32 bytes, over the magic, version, length and payload
//
// The checksum is verified before a byte of the payload is read, and every
// read is bounds-checked, so a damaged or truncated save is refused, never
// half-loaded. A world that decodes but could not happen (a costume that does
// not exist, a door answered in the future) is refused too. Writing goes to a
// temporary file first and is renamed into place; the save it replaces is
// kept as the previous save, which is loaded instead when the current one is
// damaged. (Kindling's and Atman's format, with Familiar's World.)
//
// Growing the format: a later version only ever adds fields, read when the
// save's version has them and left at the new game's defaults when it does
// not. Arrays (the gates, the houses) are written with their length, so a
// town with more of them still loads a save from a town with fewer. A save
// from a newer version than this game knows is refused, never half-read.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "sim/world.h"

namespace familiar {

// 1: M2's night (costumes, knocking, tricks). 2: M3's cat (the dropped candy
// bag and where it lies, the spirits seen); a format 1 save loads with the bag
// in hand and no spirits seen, as it was. Every older version still loads
// (see decodeSave and tests/test_saves.cpp).
constexpr uint32_t kSaveVersion = 2;

// The World as save bytes, and back. decode returns false (with a reason)
// on anything it does not accept, and leaves `out` untouched.
std::vector<uint8_t> encodeSave(const World& w);
bool decodeSave(const std::vector<uint8_t>& bytes, World& out, std::string* error);

struct LoadResult {
    bool ok = false;
    bool fromPrevious = false;  // the current save was damaged; the previous one was loaded
    std::string message;        // what happened, for the player
};

// The slot's files.
std::string savePath(const std::string& slot);
std::string previousSavePath(const std::string& slot);

// Writes the world to the slot, keeping the save it replaces as the previous one.
bool writeSave(const std::string& slot, const World& w, std::string* error);
// Loads the slot's save, or its previous save if the current one is damaged.
LoadResult loadSave(const std::string& slot, World& out);
// Whether the slot has a save of either kind, good or not.
bool hasSave(const std::string& slot);

// Where saves go when no --slot is given: %APPDATA%\Familiar\default on
// Windows, $XDG_DATA_HOME/familiar/default (or ~/.local/share/...) elsewhere.
std::string defaultSlot();

}  // namespace familiar
