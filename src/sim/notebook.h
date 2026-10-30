// Wren's notebook (M4): pages that fill in as the night goes, about Samhain,
// the Wheel of the Year, the Rede, lanterns in windows, guising,
// jack-o'-lanterns and black cats, and the eight secrets. Every page about
// belief or history cites its sources by key from RESEARCH.md ([S1] ...),
// says which parts are history, which are modern practice and which are the
// game's own invention, and is written for a twelve-year-old (RESEARCH.md,
// sensitivity notes). Text is plain ASCII: Strata's font draws nothing else.
#pragma once

#include <vector>

#include "sim/world.h"

namespace familiar {

enum PageId : int { SamhainPage = 0, WheelPage, LanternsPage, GuisingPage, JackPage, CatsPage, RedePage, PageCount };
enum SecretId : int { BridgeInitials = 0, OwlInOak, FairyRing, RoofView, StoneCat, Scarecrow, LamplighterSecret, SwapTree, SecretCount };

struct Page {
    const char* title;
    const char* text;
    bool aboutBelief;  // about belief or history: must cite RESEARCH.md
};
const Page& page(int i);    // PageId
const Page& secret(int i);  // SecretId

// Whether the cat or Wren is where secret i is found (and in the right form, at the right hour).
bool secretHere(const World& w, int i);

int pagesFound(const World& w);
int secretsFound(const World& w);

// The pages Wren has, in notebook order: the pages, then the secrets found.
std::vector<const Page*> notebook(const World& w);

// Called by tick after everything else: turns pages and secrets on as the night earns them.
void notebookTick(World& w);

}  // namespace familiar
