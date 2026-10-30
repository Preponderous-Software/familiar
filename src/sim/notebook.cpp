#include "sim/notebook.h"

#include <cmath>

#include "sim/cat.h"
#include "sim/craft.h"
#include "sim/doors.h"

namespace familiar {

namespace T = familiar::town;

namespace {

// The pages. Keys in [brackets] are sources in RESEARCH.md; tests check that
// every one exists there. Nana's words are original to the game, and marked in
// RESEARCH.md for a practising Wiccan's review before the game is promoted.
const Page kPages[PageCount] = {
    {"Samhain",
     "Nana says it SOW-in. Tonight is her festival.\n"
     "History: Samhain, from Old Irish 'samain', is usually read as 'summer's end'. It marked the start of winter in "
     "the Gaelic year, when the cattle came down from the summer pastures [S1]. It is in the oldest Irish stories, "
     "from about the 9th century [S3] [S4], and people believed the otherworld came close at Samhain [S2]. Books often "
     "call it 'the Celtic New Year', but historians find little evidence for that [S3] [S4].\n"
     "Today: many Pagans, and Wiccans like Nana, keep Samhain on 31 October to remember the dead: family, friends, "
     "even pets [S1] [S15]. Many mark it as the turn of their year, and say the veil between the living and the dead "
     "is thinnest tonight [S1] [S5].\n"
     "In this game: the spirits a cat can see are the game's fairy tale, not something Wiccans claim.",
     true},
    {"The Wheel of the Year",
     "Nana's year has eight festivals, about six weeks apart: Samhain, Yule (midwinter), Imbolc, Ostara (the spring "
     "equinox), Beltane, Litha (midsummer), Lughnasadh (the first harvest) and Mabon (the autumn equinox) [S5] [S6].\n"
     "History: four are old Gaelic festivals; the solstices and equinoxes come from the sky. Putting all eight on one "
     "wheel is modern: Gerald Gardner and Ross Nichols did it in the 1950s [S1] [S5]. The name Mabon for the autumn equinox has no older pagan source [S6].\n"
     "Nana: \"Samhain's at the top of the wheel for me. The harvest is in, and the year turns.\"",
     true},
    {"Lanterns in the window",
     "In Ireland, people once lit candles in their windows on All Souls' Night, at the start of November, to light the "
     "souls of the dead on their way. Schoolchildren wrote it down for the National Folklore Collection in the 1930s "
     "[S12]. It is a Catholic folk custom, not a Wiccan one; some modern Pagans keep a candle in a window too [S4]. "
     "Nana took it into her own Samhain.\n"
     "Tonight four lanterns: Nana's window, for her mother Ada; the Okafors' porch, for Grandpa Ike; Mr. Hale's back "
     "step, for June; and the lychgate, for everyone in the churchyard.",
     true},
    {"Guising",
     "In Scotland, children in disguise visit houses at Halloween and have to perform, with a joke or a song, before "
     "they get a treat; the treats were once nuts and apples [S20]. Guising was first written down in 16th-century "
     "Scotland, and later in Ireland, the Isle of Man and Wales [S4].\n"
     "American trick-or-treating is younger: folklorists date it to the 1930s [S22]. There is no evidence that it "
     "comes from Pagan rites [S1].",
     true},
    {"Jack-o'-lanterns",
     "Before pumpkins there were turnips. In 19th- and early 20th-century Ireland and Britain, people carved turnips, "
     "potatoes and beets into lanterns with a candle inside [S19]. The National Museum of Ireland keeps a cast of a "
     "turnip lantern from Co. Donegal, of a kind made there around 1900 [S21].\n"
     "The name comes from the story of Stingy Jack, turned away from Heaven and Hell and left to wander with only a "
     "lantern [S19] [S21]. Carving pumpkins is an American way of keeping the custom [S21]. Nana keeps a turnip on her "
     "step beside the pumpkins.",
     true},
    {"Black cats",
     "Tonight's cat form is a fairy-tale gift for one night, borrowed from old stories about witches. It is not "
     "something Wiccans believe or do.\n"
     "History: in English witch trials of the 1500s and 1600s, accusers claimed that 'familiars', spirits in animal "
     "shape, served the people they accused, and that a witch could become a cat [S24]. Those stories were taken from "
     "real people under questioning, and many of them were then tried and put to death [S24].\n"
     "Black cats are not unlucky. Some animal shelters now hold Halloween adoption days for them [S25]. Smoke was just "
     "Nana's cat, and much loved.",
     true},
    {"The Rede",
     "\"An it harm none, do what ye will\": the Wiccan Rede, in the short form most Wiccans know [S6]. Doreen "
     "Valiente made it public in the 1960s [S6]. It is not permission to do anything at all: you are free, and you "
     "answer for what you do, so you take care to harm no one [S6] [S8].\n"
     "Nana: \"That's why it's an apple and a cup of cider, love. Nothing that costs anyone anything.\"\n"
     "Wicca is a recognised religion in the United States, and since 2007 the pentacle can be carved on a veteran's "
     "headstone [S9] [S11].",
     true},
};

const Page kSecrets[SecretCount] = {
    {"The bridge rail", "Someone carved 'A.R. 1942' in the bridge rail. Ada Rowan? Nana's mother was twelve then.", false},
    {"The owl in the old oak", "A barred owl in the old oak, watching. \"Who cooks for you?\" it asks, and nobody answers.", false},
    {"The fairy ring", "A ring of mushrooms at the edge of the woods. People once said fairies danced in rings like this; "
                       "really a fungus grows outward in a circle.", false},
    {"Nana's roof", "From the top of Nana's roof the whole of Elm Street is a string of porch lights.", false},
    {"The stone cat", "A little stone cat sits on the lychgate roof, worn smooth. Nobody knows who put it there.", false},
    {"The scarecrow", "The orchard scarecrow wears Mr. Novak's old hat. He says he wants it back.", false},
    {"The lamplighter", "An old man in a long coat walks the hill path with a lamp on a pole, lighting lamps that "
                        "are not there any more. He nods to the cat.", false},
    {"The candy-swap tree", "Under the big maple on the green, children swap candy. A note is pinned to the trunk: "
                            "'NO RAISINS'.", false},
};

constexpr Vec2 kInitials{52.0f, -1.25f};
constexpr Vec2 kStoneCat{23.5f, -29.6f};
constexpr int kDark = 18 * 60 + 14;  // the end of civil twilight

bool near(const World& w, Vec2 p, float r) {
    const float dx = w.player.x - p.x, dz = w.player.z - p.z;
    return dx * dx + dz * dz < r * r;
}

}  // namespace

const Page& page(int i) { return kPages[((i % PageCount) + PageCount) % PageCount]; }
const Page& secret(int i) { return kSecrets[((i % SecretCount) + SecretCount) % SecretCount]; }

bool secretHere(const World& w, int i) {
    const int minute = static_cast<int>(gameMinutes(w));
    switch (i) {
        case BridgeInitials: return near(w, kInitials, 0.9f);
        case OwlInOak: return minute >= kDark && near(w, T::kOldOak, 4.5f);
        case FairyRing: return near(w, T::kFairyRing, 2.5f);
        case RoofView: {
            const Box2 f = T::footprint(T::Nana);
            return w.form == Form::Cat && f.contains(w.player.x, w.player.z) && w.player.feet > 4.5f;
        }
        case StoneCat: return near(w, kStoneCat, 2.0f);
        case Scarecrow: return near(w, T::kScarecrow, 2.2f);
        case LamplighterSecret: return spiritNear(w) == Lamplighter;  // the cat sees him (sim/cat.h)
        case SwapTree: return near(w, T::kSwapTree, 2.2f);
        default: return false;
    }
}

int pagesFound(const World& w) {
    int n = 0;
    for (int i = 0; i < PageCount; ++i) n += (w.pages >> i) & 1u;
    return n;
}

int secretsFound(const World& w) {
    int n = 0;
    for (int i = 0; i < SecretCount; ++i) n += (w.secrets >> i) & 1u;
    return n;
}

std::vector<const Page*> notebook(const World& w) {
    std::vector<const Page*> out;
    for (int i = 0; i < PageCount; ++i)
        if (w.pages & (1u << i)) out.push_back(&kPages[i]);
    for (int i = 0; i < SecretCount; ++i)
        if (w.secrets & (1u << i)) out.push_back(&kSecrets[i]);
    return out;
}

void notebookTick(World& w) {
    bool knockedAny = false;
    for (bool k : w.knocked) knockedAny = knockedAny || k;
    const bool earned[PageCount] = {
        w.lit[NanaWindow],        // Samhain: the first thing Nana asks
        w.cider,                  // the Wheel: the harvest is in
        lanternsLit(w) >= 2,      // lanterns in windows
        w.jokes > 0,              // guising
        knockedAny,               // jack-o'-lanterns, on the first porch
        w.form == Form::Cat,      // black cats
        w.offered,                // the Rede, at the offering
    };
    for (int i = 0; i < PageCount; ++i)
        if (earned[i]) w.pages |= 1u << i;
    for (int i = 0; i < SecretCount; ++i)
        if (secretHere(w, i)) w.secrets |= 1u << i;
}

}  // namespace familiar
