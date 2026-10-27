/* fixgroups.c - the fixes (fixes.c) sorted by kind, for the settings' list: from what the game
 * plainly gets wrong to what is open to argument, as players have talked of them over the years
 * (the GOG and Steam forums, the old Usenet posts and the patch notes MicroProse left). Each fix
 * stays its own switch; this only orders and heads them. A fix not listed here falls under
 * "Plain mistakes". */
#include <string.h>
#include "fixgroups.h"

const FixGroup FIX_GROUPS[FIX_NGROUPS] = {
    { "破坏游戏平衡", "崩溃、卡顿、物品或进度丢失、存档损坏、任务永远无法完成。没人想要这些。" },
    { "普通错误", "游戏本该做到却做错的事：选错了队员、去了错的城镇、检查了错误的选项、数字在途中丢失。" },
    { "正如说明书和游戏文本所言", "游戏做了与说明书或自身文本所述不同的事情。已修正以符合原文表述。" },
    { "老把戏被封禁", "玩家长期利用的漏洞（防火保护失效、忏悔时时间倒退、重复领取同一赏金）。为保留老把戏，暂不修复。" },
    { "1992年的遗留问题", "不是漏洞，但也没人怀念：队伍首次出发前，印刷版说明书中的那个问题。" },
};

static const struct { const char *id; int g; } G[] = {
    /* game-breaking */
    { "nullitem", 0 }, { "itemcount", 0 }, { "testcard", 0 }, { "fullpack", 0 }, { "castlefreeze", 0 }, { "lootcount", 0 },
    { "innsave", 0 }, { "offmap", 0 }, { "altar", 0 }, { "castle", 0 }, { "armorwear", 0 }, { "potions", 0 }, { "standings", 0 },
    { "battledead", 0 }, { "blessing", 0 }, { "colors", 0 },
    /* as the manual and the texts say */
    { "greatpower", 2 }, { "oilweapons", 2 }, { "armorday", 2 }, { "pauls", 2 }, { "sainttexts", 2 }, { "teachcap", 2 }, { "alchrisk", 2 },
    /* exploits */
    { "fireward", 3 }, { "confession", 3 }, { "rkonce", 3 }, { "bounty", 3 },
    /* not a bug */
    { "copyprot", 4 },
    /* (hitsound: a plain mistake, the default) */
};

int fix_group(const char *id)
{
    for (size_t i = 0; i < sizeof G / sizeof G[0]; i++) if (!strcmp(G[i].id, id)) return G[i].g;
    return 1;
}
