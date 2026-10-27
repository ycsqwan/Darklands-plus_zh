// cardhints.cpp - why an option of a card is greyed: the cards' scripts (src/c) show some
// options without letting them be chosen - something done already, the time of day, a fact the
// party has yet to learn, money - and the game never says which. One line for each such option,
// read from the scripts; the card view shows it when the pointer rests on the greyed option.
// (Options greyed because nobody knows the saint or carries the potion are explained by the
// card view itself, from the party.)
#include <strings.h>

struct CardHint { const char *card; int opt; const char *why; };

static const CardHint HINTS[] = {
#include "cardhints_data.h"           /* (a .h: the build remakes the ui files when one changes) */
};

/* the same options with a fix on (mods/fixes.c: "cardopts" unless named): as the cards were meant to be */
struct FixedHint { const char *fix; const char *card; int opt; const char *why; };
static const FixedHint FIXED[] = {
    { "university", "$UNIVE00", 0, "在这里徒劳寻找宗教图书馆约一周后，不再适用。" },
    { "university", "$UNIVE00", 1, "在这里徒劳寻找炼金术配方约一周后，不再适用。" },
    { "cardopts", "$SELEC01", 1, "在这座城镇的一处下水道井盖击败你约三周后，不再适用。" },
    { "cardopts", "$SELEC01", 2, "需要你钱包里的贿赂；在守卫认出你后的两天内以及守卫发现你在墙边偷窥后的一天内，都不适用。" },
    { "cardopts", "$REVOL14", 4, "需要一根绳子，且不能在你等待一天后再返回城墙时使用。" },
    { "cardopts", "$CITYW02", 0, "需要一根绳子。" },
    { "cardopts", "$CITYW04", 2, "" },                      /* (the climb: never greyed) */
    { "cardopts", "$CITYW04", 3, "" },
    { "cardopts", "$CITYW04", 4, "在这里一处下水道井盖击败你约三周后，不再适用。" },
    { "cardopts", "$CITYW04", 5, "" },
    { "cardopts", "$CITYW04", 8, "在这里守卫拒绝你的贿赂后的两天内，以及你无力支付时，都不适用。" },
};
extern "C" int fix_is_on(const char *id);

extern "C" const char *card_hint(const char *card, int opt)
{
    if (!card || !*card) return nullptr;
    for (const FixedHint &h : FIXED)
        if (h.opt == opt && !strcasecmp(h.card, card) && fix_is_on(h.fix)) return *h.why ? h.why : nullptr;
    for (const CardHint &h : HINTS)
        if (h.opt == opt && !strcasecmp(h.card, card)) return h.why;
    return nullptr;
}
