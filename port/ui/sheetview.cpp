/* sheetview.cpp - draws a member's sheet (mods/sheetview.c runs it; docs/NATIVE_SCREENS.md),
 * the whole page anew beside the party panel.
 *
 * Top: the member's name, sex and age, the leader, the tabs, Close.
 * The one place for the party's things, skills, saints and formulae (the F12 window has no pages
 * of its own for them).
 * Things: the paper doll on its column (put together from the game's own pictures, as the
 *   original does: PICS\ARMSBACK.PIC and the armour, weapon, shield and missile weapon over it),
 *   the load under it; right, the things, those equipped first, the quality only where it can
 *   differ; Equip / Unequip / Drink and Drop on the row under the pointer, Undo for what was
 *   dropped; a thing dragged onto a member at the left is given to them (Shift: one of a stack);
 *   a weapon, armour or shield the pointer rests on shows all its numbers and who could use it.
 * Skills: the doll; right, the attributes and the skills on one page in the original's groups,
 *   each with its sign; Endurance, Strength and Divine Favor as current/maximum, the others
 *   their value (with the maximum, in red, when lowered); blue what a saint raised for a while;
 *   a gold dot where the member is the party's best. The strip: what it is for, how long a
 *   saint's change lasts, the whole party's values; with nothing under the pointer, what is at
 *   work on the member.
 * Saints: the whole party's - each saint someone knows (or all), who knows it, a choice of what
 *   they help with, "taught here" by the library the party is at; the saint under the pointer (or
 *   clicked) with the game's statue for it (its prayer screen's picture), who knows it or the
 *   libraries the party has been to that teach it, what it does (mods/saintinfo.h), where the
 *   text says otherwise, whether encounters call on it, and the legend.
 * Formulae: the whole party's - each formula someone knows (or all), who knows it, the batches
 *   its ingredients make, "sold here"; under the list the 23 ingredients the party has, those
 *   the formula shown calls for ringed; the formula under the pointer with its effect, mystic
 *   number and risk, its ingredients against what the party carries, and each alchemist's
 *   chance with one batch, or where an alchemist the party has been to sells it. */
#include "vtrace.h"
#include "imgui.h"
#include "skillicons.h"
#include "../runtime/config.h"
#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <regex>
#include "../runtime/cpu.h"
#include "../runtime/game.h"
#include "../runtime/gamevars.h"
#include "../mods/sheetview.h"
#include "../mods/gamemodel.h"
#include "../mods/saintinfo.h"
#include "../mods/fixes.h"
#include "nsdraw.h"
#include "marks.h"
#include "libraries.h"
#include "money.h"
#include "effects.h"
bool ui_item_tidy_less(int ida, int qa, int idb, int qb);    /* (inventory.cpp) */
#include "gamepics.h"

extern "C" u32 plat_ticks_ms(void);
extern "C" int partypanel_member_at(int gx, int gy);
extern int partypanel_focus;
void ui_item_icon(ImDrawList *dl, ImVec2 c, float s, int id);
const char *ui_attr_help(int a);
const char *ui_skill_help(int k);
std::string rules_weapons_of_skill(int k);
size_t saint_effect_at(const std::string &text);
std::string journal_where_sold(int id);                     /* (ui/journal.cpp) */
void ui_item_details(const ItemDef *d, int id, int q);      /* (ui/inventory.cpp) */

static float saint_effect(int s, int key);                     /* (the Saints page: how much a saint gives of a thing, from its chips) */
namespace {
enum { TAB_THINGS, TAB_SKILLS, TAB_SAINTS, TAB_FORMULAE };
/* what the saints help with: words of their effect */
struct Use { const char *name; const char *words[12]; };
const Use USES[] = {
    { "任何", { nullptr } },
    { "治疗", { "heal", "cure", "restor", "wound", "disease", "poison", "endurance", nullptr } },
    { "力量", { "strength", nullptr } },
    { "格斗", { "weapon", "attack", "combat", "battle", "fight", "defen", "damage", "edged", "impact", "flail", "polearm", nullptr } },
    { "弹幕", { "thrown", "bow", "missile", "arrow", nullptr } },
    { "防护", { "protect", "fire", "safe", "shield", "armo", nullptr } },
    { "旅行", { "travel", "journey", "water", "ship", "sea", "riding", "horse", "woodwise", nullptr } },
    { "心智", { "intellig", "perception", "latin", "read", "alchemy", "artifice", "knowledge", nullptr } },
    { "魅力", { "charisma", "speak", "reputation", "virtue", "streetwise", nullptr } },
    { "潜行", { "stealth", "agility", "hid", nullptr } },
};
const int NUSES = (int)(sizeof USES / sizeof USES[0]);

struct SheetView {
    int tab = TAB_THINGS;
    int scroll[4] = {};
    int last_who = -1;
    /* the strip: 1 an item, 2 a skill, 3 a formula, 4 a saint, 5 an attribute */
    int what = 0, which = -1; unsigned seq = 0;
    int drop_row = -1; unsigned act_seq = 0;      /* a row asking "sure?" */
    int pin_saint = -1, pin_formula = -1;         /* clicked: stays while the pointer is elsewhere */
    bool saints_all = false, formulae_all = false; int use = 0, use_k = -1;   /* (use_k: a shield chosen - saint_touches) */
    /* dragging a thing */
    int drag_row = -1; bool dragging = false;
} V;

std::string lower(std::string s) { for (char &c : s) c = (char)tolower((unsigned char)c); return s; }

const ImU32 BEST = IM_COL32(196, 146, 30, 255), RAISED = IM_COL32(26, 64, 132, 255);   /* the party's best; raised for a while */

/* what is at work on the party now (ui/effects.h), read once a frame */
std::vector<Eff> EFF;
std::string hours_left(long h)
{
    char b[32];
    if (h < 48) snprintf(b, sizeof b, "%ld 小时%s", h, h == 1 ? "" : "s");
    else snprintf(b, sizeof b, "%ld 天", (h + 12) / 24);
    return b;
}
/* a saint's (or its price's) change to member m's attribute (kind 0x48) or skill (0x49) k: the sum,
   and the soonest it ends */
int raised_by(int m, int kind, int k, long *left)
{
    int t = 0; long l = -1;
    for (const Eff &e : EFF) if (e.m == m && e.kind == kind && e.k == k) { t += e.amount; if (l < 0 || e.left < l) l = e.left; }
    if (left) *left = l;
    return t;
}

/* the doll's pictures (s031 1462:0b22) */
std::vector<std::string> doll_layers(const SheetModel &s)
{
    std::vector<std::string> v = { "PICS\\ARMSBACK.PIC" };
    switch (s.limb) {
    case 0x4c: case 0x4d: v.push_back("PICS\\PAD-LIMS.PIC"); break; case 0x4e: v.push_back("PICS\\LEAT-LIM.PIC"); break;
    case 0x4f: v.push_back("PICS\\STUDLIM.PIC"); break; case 0x50: v.push_back("PICS\\CUIRBLIM.PIC"); break;
    case 0x51: v.push_back("PICS\\SCALELIM.PIC"); break; case 0x52: v.push_back("PICS\\CHAINLIM.PIC"); break;
    case 0x53: v.push_back("PICS\\BRIGLIM.PIC"); break; case 0x54: v.push_back("PICS\\PLATELIM.PIC"); break;
    }
    switch (s.vital) {
    case 0x43: case 0x44: v.push_back("PICS\\PAD-VIT.PIC"); break; case 0x45: v.push_back("PICS\\LEAT-VIT.PIC"); break;
    case 0x46: v.push_back("PICS\\STUDVIT.PIC"); break; case 0x47: v.push_back("PICS\\CUIRBVIT.PIC"); break;
    case 0x48: v.push_back("PICS\\SCALEVIT.PIC"); break; case 0x49: v.push_back("PICS\\CHAINVIT.PIC"); break;
    case 0x4a: v.push_back("PICS\\BRIGVIT.PIC"); break; case 0x4b: v.push_back("PICS\\PLATEVIT.PIC"); break;
    }
    static const signed char WPN[34] = { 2, 3, 4, 5, 6, 7, 8, 0, 9, 1, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 24, 0,
                                         26, 27, 1, 28, 29, 30, 31, 32, 33, 34 };
    char b[32];
    if (s.weapon >= 0 && s.weapon < 34) { snprintf(b, sizeof b, "PICS\\WEAPON%d.PIC", WPN[s.weapon]); v.push_back(b); }
    if (s.shield >= 0) v.push_back(s.shield == 0x5f ? "PICS\\SMALLSH.PIC" : "PICS\\MEDIUMSH.PIC");
    if (s.missile >= 23 && s.missile <= 33) {
        static const signed char MSL[11] = { 23, 26, 27, 1, 28, 29, 30, 31, 32, 33, 34 };
        snprintf(b, sizeof b, "PICS\\WEAPON%d.PIC", MSL[s.missile - 23]); v.push_back(b);
    }
    return v;
}

bool shows_quality(const ItemDef *d, int q) { return d && ns_shows_quality((int)(d - &gamedata.items[0]), q); }

const char *LOAD[4] = { "轻负重", "正常负重", "重负重", "超载" };
/* what each band does, from the game's code (equip_weight s031 18e7:140a: the equipped things'
   weights; s014 18bf: attack_rate adds 30 to a melee blow's time between half and all of
   Endurance + Strength, 100 beyond half again - nothing between; the dodge keeps the whole
   Agility up to 5/8 of it, two thirds up to 1 1/4, a third up to 1 7/8, then none) */
/* the row of a thing under the pointer (the strip shows it), -1 none: 1..5 give it (mods/sheetview.c) */
extern "C" int sheetview_pointed_item(void);
extern "C" const char *ui_load_effect(int band)
{
    switch (band) {
    case 0: return "轻负重（不超过 {Endurance} + {Strength} 的一半）：完全无负担。仅计算装备物品（武器、盔甲、盾牌）；随身携带的物品不计重量。";
    case 1: return "正常负重（不超过 {Endurance} + {Strength}）：近身格斗速度约慢五分之一，超过五分之八后，用于闪避的 {Agility} 降至三分之二。";
    case 2: return "重负重（最多增加一半）：用于闪避的 {Agility} 降至三分之二，超过 {Endurance} + {Strength} 的一倍半后则降至三分之一。近身格斗速度与正常负重时相同（原版游戏中存在疏漏）。";
    default: return "超载（超过 {Endurance} + {Strength} 的一倍半）：近身格斗速度约慢三分之二，用于闪避的 {Agility} 先降至三分之一，超过 {Endurance} + {Strength} 的两倍左右后则完全失效。";
    }
}

/* a saint's description: legend, then what it does */
void saint_parts(const std::string &t, std::string &lore, std::string &eff)
{
    size_t a = saint_effect_at(t);
    lore = t.substr(0, a); size_t b = a; while (b < t.size() && t[b] == ' ') b++;
    eff = t.substr(b);
    while (!lore.empty() && lore.back() == ' ') lore.pop_back();
}
bool saint_fits(int s, int use)
{
    if (use <= 0) return true;
    std::string lore, eff; saint_parts(gamedata.saints[s].text, lore, eff);
    std::string e = lower((eff.empty() ? gamedata.saints[s].text : eff) + " " + saint_info_does(s));   /* (what it promises, what it does) */
    for (const char *const *w = USES[use].words; *w; w++) if (e.find(*w) != std::string::npos) return true;
    return false;
}
/* what a saint's help does (mods/saintinfo.c, the code's own account) names attribute or skill key
   (0..6 the attributes, 7..25 the skills, 26 the town's opinion): the shields' filter on the Saints page.
   The account says the skills shortly ("Edged", "Latin") and "every weapon skill" for the seven. */
static const char *const SAINT_SKILL_WORD[19] = { "锋利武器", "冲击", "棍棒类武器", "长柄武器", "投掷", "弓箭", "投射装置", "炼金术",
    "宗教", "美德", "通用语言", "拉丁文", "读写能力", "治疗", "工艺", "潜行", "精通街头", "骑术", "精通林地" };
enum { SK_KEYS = 27, KEY_REP = 26 };
static bool saint_touches(int s, int key) { return saint_effect(s, key) > 0; }
static int saint_key_count(int key)
{
    static int n[SK_KEYS], fixes = -1;
    int f = fix_is_on("sainttexts") | fix_is_on("armorday") << 1 | fix_is_on("fireward") << 2 | fix_is_on("pauls") << 3 | fix_is_on("saintrep") << 4;   /* (the account follows these fixes) */
    if (f != fixes) { fixes = f; for (int k = 0; k < SK_KEYS; k++) { n[k] = 0; for (int t = 0; t < (int)gamedata.saints.size() && t < SAINT_INFO_N; t++) n[k] += saint_touches(t, k); } }
    return key >= 0 && key < SK_KEYS ? n[key] : 0;
}
const GamePic *statue(int cls)
{
    int k = cls >= 0 && cls <= 9 ? cls : cls >= 10 && cls <= 13 ? 0 : 1;
    char b[32]; snprintf(b, sizeof b, "PICS\\SNTBACK%d.PIC", k);
    return gamepic_part(b, 70, 0, 70, 196);
}
}

extern "C" int sheetview_showing(void) { return ns_screen == NS_SHEET && sheet_model.open && !ns_hidden(); }

extern "C" void aging_forecast(int m, char *out, int n);         /* mods/aging.c */
static void header(NsDraw &P, const SheetModel &s)
{
    ImFont *f = P.f, *fo = P.fo;
    P.text(f, NsDraw::FS_HEAD, 63.5f, 2.2f, NsDraw::INK, ns_gtext(s.fullname), -1, 290);
    if (P.button(296, 2.5f, 318, 10.2f, "关闭", IM_COL32(112, 40, 22, 235), NsDraw::FS_SMALL, PB_Y)) ns_push(SV_CLOSE, 0, 0, 0);
    char sub[96]; snprintf(sub, sizeof sub, "%s，{年龄：%d}", s.female ? "女性" : "男性", s.age);   /* (the age as its hourglass: ui/richtext.h) */
    float aw = P.width(fo, NsDraw::FS_SMALL, sub);
    for (const Eff &e : EFF) if (e.m == s.who && e.kind == 0x43) { snprintf(sub + strlen(sub), sizeof sub - strlen(sub), "；将在 %s 离开队伍", hours_left(e.left).c_str()); break; }
    P.text(fo, NsDraw::FS_SMALL, 63.5f, 12.4f, NsDraw::INK_SOFT, sub);
    if (P.over(63.5f, 11.6f, 63.5f + aw, 18.8f)) {                /* the age: what the years will take (mods/aging.c) */
        char t[700]; aging_forecast(s.who, t, sizeof t);
        ns_tip(t, 24);
    }
    float lx = 63.5f + P.width(fo, NsDraw::FS_SMALL, sub) + 3;
    if (s.who == s.leader) {
        P.rect(lx, 11.6f, lx + 17, 18.8f, IM_COL32(214, 172, 60, 255), 1.2f);
        P.text(fo, NsDraw::FS_SMALL, lx + 2, 12.4f, NsDraw::INK, "领导者");
    } else if (P.button(lx, 11.6f, lx + 26, 18.8f, "成为领导者", IM_COL32(120, 84, 40, 235), 4.4f)) ns_push(SV_LEADER, s.who, 0, 0);
    if (P.over(lx, 11.6f, lx + (s.who == s.leader ? 17 : 26), 18.8f))
        ns_tip(config_get_int("assist.best_leader", 1) && !config_get_int("original", 0)
            ? "Darklands+（“最优秀的成员为队伍代言”，F12 > 设置 > 卡牌与城镇）：在卡牌上，每项测试由该领域最擅长的成员完成；商店则由最会讨价还价的人定价，无论谁带队。"
              "队长依然是卡牌上所指的“队长”（如矿山的陷阱、教堂的恩惠、交出的物品等）。"
              "在战斗和地下城中（非暂停状态），队长负责处理门、箱子、锁和陷阱，并指挥队伍行动。"
            : "队长在卡牌上代表队伍发言并行动，在商店里进行讨价还价。");
    static const char *TABS[4] = { "物品", "技能", "圣人", "配方" };
    if (sheet_model.tab_step) {                             /* (Tab, Shift+Tab, Right, Left: mods/sheetview.c) */
        V.tab = ((V.tab + sheet_model.tab_step) % 4 + 4) % 4; V.drop_row = -1; V.what = 0;
        sheet_model.tab_step = 0;
    }
    float tw = 25.5f, x = 318 - 4 * tw;
    for (int t = 0; t < 4; t++) {
        bool sel = t == V.tab, h = P.over(x, 11, x + tw - 0.8f, 19.5f);
        P.tab(x, 11, x + tw - 0.8f, 19.5f, sel, h);
        P.text(f, 5.2f, x + (tw - 0.8f - P.width(f, 5.2f, TABS[t])) * 0.5f, 12.3f, sel ? NsDraw::LIGHT : NsDraw::INK, TABS[t]);
        if (P.hit(x, 11, x + tw - 0.8f, 19.5f)) { V.tab = t; V.drop_row = -1; V.what = 0; }
        x += tw;
    }
    P.rect(62, 20.3f, 318, 20.7f, IM_COL32(120, 80, 36, 110));
}

/* what the load does, for this member now (the game's rules: member_eff_attr2 and the band, s029
   0e76:013a, for the tests on cards; to_hit_roll and attack_rate, s014 18bf, in melee) */
static std::string load_effects(const SheetModel &s)
{
    int L = s.load, cap = s.attr[0] + s.attr[1], capm = s.attr_max[0] + s.attr_max[1], agl = s.attr[2], m = 5 * capm / 4;
    auto part = [](int l, int c, int a, bool one) -> std::pair<int, const char *> {
        if (l <= c / 2) return { a, "全部" };
        if (l <= c) return { 2 * a / 3, "三分之二" };
        if (l <= 3 * c / 2) return { a / 3, "三分之一" };
        return { one ? 1 : 0, one ? "按1份" : "完全不携带" };
    };
    auto card = part(L, cap, agl, true), def = part(L, m, agl, false);
    const char *swing = (L > capm / 2 && L <= capm) ? "稍微慢一些（攻击速度下降）" : L > 3 * capm / 2 ? "慢很多（攻击速度大幅下降）" : "按正常节奏";
    char b[900];
    snprintf(b, sizeof b,
             "负重%d：指%s所装备物品的重量（武器、护甲、盾牌）。已携带但未装备的物品不计重量。\\n"
             "与{耐力}+{力量}相比（目前%d）：轻型最高%d，普通型最高%d，重型最高%d，超载则超过此限。\\n\\n"
             "Now:\n"
             "- 卡牌上需要{敏捷}的测试（攀墙、奔跑、陷阱）：{敏捷:%d}计入%s（%d）。\\n"
             "- 近战格挡：{敏捷}计入%s（%d）。\\n"
             "- 近战挥击：%s。\\n\\n"
             "旅行、战斗中的步行、投射物以及{耐力}均不受负重影响。",
             L, ns_gtext(s.names[s.who]).c_str(), cap, cap / 2, cap, 3 * cap / 2, agl, card.second, card.first, def.second, def.first, swing);
    return b;
}

static void doll(NsDraw &P, const SheetModel &s)
{
    if (false) {                                            /* (the stone niche drawn anew, ui/niche.cpp: set aside, the player preferring the game's own) */
        void ui_draw_doll_niche(ImDrawList *dl, ImVec2 o, float kx, float k);
        ui_draw_doll_niche(P.dl, NsDraw::w(62, 21.5f), P.kx, P.k);
        const GamePic *d = gamepic_layers_off_back(doll_layers(s), "PICS\\ARMBACK.PIC");
        if (d) P.dl->AddImage((ImTextureID)(intptr_t)d->tex, NsDraw::w(62, 21.5f), NsDraw::w(62 + d->w, 21.5f + d->h));
    } else {
    const GamePic *g = gamepic_layers(doll_layers(s), "PICS\\ARMBACK.PIC");
    if (g) P.dl->AddImage((ImTextureID)(intptr_t)g->tex, NsDraw::w(62, 21.5f), NsDraw::w(62 + g->w, 21.5f + g->h));
    else P.rect(62, 21.5f, 198, 191.5f, IM_COL32(200, 196, 190, 255));
    }
    /* the load */
    int band = s.load_band < 0 ? 0 : s.load_band > 3 ? 3 : s.load_band;
    ImU32 c = band >= 3 ? NsDraw::RED : band == 2 ? IM_COL32(150, 80, 20, 255) : NsDraw::INK;
    P.text(P.fo, NsDraw::FS_SMALL, 63.5f, 192.3f, c, LOAD[band]);
    if (P.over(62, 191, 198, 197.5f)) ns_tip(ui_load_effect(band), 26);
    char v[24]; snprintf(v, sizeof v, "%d的%d", s.load, s.capacity);
    P.text(P.fo, NsDraw::FS_SMALL, 0, 192.3f, NsDraw::INK_SOFT, v, 197);
    float f = s.capacity > 0 ? fminf(1.5f, (float)s.load / s.capacity) / 1.5f : 0;
    P.rect(100, 194.6f, 160, 196.4f, IM_COL32(214, 196, 160, 255), 0.6f);
    P.rect(100, 194.6f, 100 + 60 * f, 196.4f, c, 0.6f);
    P.rect(100 + 60 / 3.0f, 194.2f, 100 + 60 / 3.0f + 0.4f, 196.8f, IM_COL32(120, 80, 36, 160));
    P.rect(100 + 120 / 3.0f, 194.2f, 100 + 120 / 3.0f + 0.4f, 196.8f, IM_COL32(120, 80, 36, 160));
    if (P.over(62, 191.5f, 198, 198) && config_get_int("assist.shop_info", 1)) {   /* what the load does */
        ns_tip(load_effects(s).c_str(), 25);
    }
}

/* where the shown member stands in the party at attribute or skill idx: 1 the best (others lower),
   0 not, -1 nobody to compare with */

/* the strip's line on a saint's change to the attribute or skill under the pointer */
static float raised_line(NsDraw &P, const SheetModel &s, bool attr, int idx, float x, float y, float w)
{
    long left; int t = raised_by(s.who, attr ? 0x48 : 0x49, idx, &left);
    if (!t) return 0;
    char b[96]; snprintf(b, sizeof b, "%s按%d为%s再加（某位圣人的%s）。", t > 0 ? "提高" : "降低", t > 0 ? t : -t, hours_left(left).c_str(), t > 0 ? "help" : "price");
    return P.para(P.fo, NsDraw::FS_SMALL, x, y, w, t > 0 ? RAISED : NsDraw::RED, b) + 0.6f;
}
static int party_best(const SheetModel &s, bool attr, int idx)
{
    int mine = attr ? s.pattr[s.who][idx] : s.pskill[s.who][idx], n = 0, lower = 0;
    for (int m = 0; m < 5; m++) {
        if (!s.present[m] || m == s.who) continue;
        int v = attr ? s.pattr[m][idx] : s.pskill[m][idx];
        n++;
        if (v > mine) return 0;
        if (v < mine) lower++;
    }
    return n ? (lower ? 1 : 0) : -1;
}

/* the whole party at attribute or skill idx, in a line under the strip's text (what F12 > Party
   once showed side by side): each name and value, the best in gold, the shown member underlined */
static float party_line(NsDraw &P, const SheetModel &s, bool attr, int idx, float x0, float y, float x1)
{
    int hi = -1000, lo = 1000, n = 0;
    for (int m = 0; m < 5; m++) if (s.present[m]) { int v = attr ? s.pattr[m][idx] : s.pskill[m][idx]; hi = std::max(hi, v); lo = std::min(lo, v); n++; }
    if (n < 2) return 0;
    const float gs = NsDraw::FS_SMALL;
    float x = x0, lh = gs * 1.25f;
    P.text(P.fo, gs, x, y, NsDraw::INK_SOFT, "队伍："); x += P.width(P.fo, gs, "队伍：") + 1.5f;
    for (int m = 0; m < 5; m++) {
        if (!s.present[m]) continue;
        int v = attr ? s.pattr[m][idx] : s.pskill[m][idx], mx = attr ? s.pattr_max[m][idx] : v;
        std::string seg = ns_gtext(s.names[m]) + " " + (v < mx ? std::to_string(v) + "/" + std::to_string(mx) : std::to_string(v));
        float w = P.width(P.fo, gs, seg);
        if (x + w > x1) { x = x0 + 2; y += lh; }
        bool best = hi > lo && v == hi;
        if (best) P.dl->AddCircleFilled(NsDraw::w(x + 0.9f, y + gs * 0.62f), 0.8f * P.k, BEST);
        float tx = best ? x + 2.4f : x;
        P.text(P.fo, gs, tx, y, best ? IM_COL32(130, 92, 10, 255) : NsDraw::INK, seg);
        if (m == s.who) P.rect(tx, y + gs * 1.12f, tx + w, y + gs * 1.12f + 0.35f, NsDraw::INK_SOFT);
        x = tx + w + 3;
    }
    return y + lh;
}

/* the strip at the bottom right: what the pointer is on */
static void strip(NsDraw &P, const SheetModel &s, float SX0, float SX1, float SY0, float SY1)
{
    ImFont *f = P.f, *fo = P.fo;
    const ImU32 ink = NsDraw::INK, soft = NsDraw::INK_SOFT, dim = NsDraw::INK_DIM;
    float SW = SX1 - SX0 - 5, y = SY0;
    P.rect(SX0, SY0 - 1.5f, SX1, SY1, IM_COL32(230, 214, 178, 255), 1.5f);
    if (V.what == 1 && V.which >= 0 && V.which < s.nitems) {
        const SvItem &it = s.item[V.which];
        const ItemDef *d = ns_itemdef(it.id);
        y += P.para(f, NsDraw::FS_NAME, SX0 + 2.5f, y, SW, ink, ns_item_name(it.id)) + 0.5f;
        std::string l1 = std::string(d ? kind_name_one(d->kind) : "");
        if (shows_quality(d, it.qual)) l1 += ns_quality_words(it.id, it.qual);
        bool wearable = d && (d->kind == K_WEAPON || d->kind == K_MISSILE || d->kind == K_ARMOR || d->kind == K_SHIELD);   /* (only equipped things count toward the load: equip_weight) */
        if (wearable) l1 += "，重量" + std::to_string(it.weight) + (it.equipped ? "" : "装备时");
        if (it.count > 1) l1 += std::string(wearable ? " each" : "") + ", " + std::to_string(it.count) + "其中";
        if (it.equipped == 2) l1 += "；该成员的坐骑";
        else if (it.equipped) l1 += "；已装备";
        else if (it.holder < 0) l1 += it.count > 1 ? "在队伍的背包中" : "；在队伍的背包中";
        if (it.kept) l1 += "；保留（不售也不丢）";
        else if (it.sell) l1 += "；标记出售（出售多余装备即可卖出）";
        y += P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y, SW, soft, l1) + 0.5f;
        std::string l2 = ns_item_line(it.id, it.qual);
        if (!l2.empty()) y += P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y, SW, soft, l2) + 0.5f;
        std::string use = ns_item_use(it.id);
        if (!use.empty()) y += P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y, SW, soft, use) + 0.5f;
        if (d && d->kind == K_INGREDIENT) y += ns_ingredient_row(P, SX0 + 2.5f, y + 0.5f, SX0 + 2.5f + SW, it.id) + 1.0f;
        { ImU32 ec; std::string enc = ns_item_encounters(it.id, &ec); if (!enc.empty()) y += P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y, SW, ec, enc) + 0.5f; }
        if (it.type == 108) {                                       /* a horse or mule: how riding works (mods/pack.c: party_get(5), the chases) */
            std::string r = "每位成员都骑着自己携带的最好马或骡子；只有当每位成员都有坐骑时，队伍才会骑乘出行。";
            if (s.shared) r += "对于没有坐骑的成员，游戏需要时（上路、追击）会为其提供最好的备用坐骑。";
            y += P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y, SW, soft, r) + 0.5f;
            if (s.npar > 1) y += P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y, SW, NsDraw::INK_DIM,
                                        it.holder < 0 ? "要让某位成员骑乘，将其拖到该成员身上，或按下其编号（1至5）。"
                                                      : "要让另一位成员骑乘，将其拖到该成员身上，或按下其编号（1至5）。") + 0.5f;
        }
        else if (it.holder >= 0 && !it.equipped && s.npar > 1)          /* (how to hand it over: the pack's spares are anyone's already) */
            y += P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y, SW, NsDraw::INK_DIM, it.count > 1 ? "要将所有坐骑分配给队员，可将其拖到左侧的某位成员身上，或按下其编号（1至5）；按住Shift键则只需一人。" : "要分配坐骑，可将其拖到左侧的某位成员身上，或按下其编号（1至5）。") + 0.5f;
        if (d && d->type >= 0 && d->type <= 33 && (d->kind == K_WEAPON || d->kind == K_MISSILE)) {
            int sk = rb(G_FAR20A5, (u16)(0x75ac + d->type)), minsk = rb(G_FAR20A5, (u16)(0x7726 + d->type)), minst = rb(G_FAR20A5, (u16)(0x7765 + d->type));
            int have = sk < 7 ? s.skill[sk] : 0;
            std::string fit = (have < minsk ? "needs " + std::to_string(minsk - have) + "更多技能" : std::string("")) +
                              (s.attr[1] < minst ? std::string(have < minsk ? ", " : "") + "太弱了" + std::to_string(minst - s.attr[1]) : "");
            P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y, SW, fit.empty() ? NsDraw::GREEN : NsDraw::RED, fit.empty() ? ns_gtext(s.names[s.who]) + "可以很好地使用它" : ns_gtext(s.names[s.who]) + ": " + fit);
        } else if (d && d->type >= 95 && d->type <= 97) {     /* a shield: the defence it gives this member (mods/gamemodel.h) */
            int sb = gm_shield_defence(d->type, it.qual, 0), sh = gm_shield_defence(d->type, it.qual, 1), ag = gm_member_fight_agility(s.who), a = s.attr[2];
            char b[240];
            snprintf(b, sizeof b, "%s用它防御：面对打击时，%d加上其近战等级；面对射击或投掷时，%d加上两倍的{敏捷:%d}，即%d%s。", ns_gtext(s.names[s.who]).c_str(), sb, sh, ag, sh + 2 * ag,
                     ag < a ? "（负重会降低敏捷）" : "");
            P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y, SW, ink, b);
        } else if (l2.empty() && it.drinkable && it.id >= 95 && 3 * (it.id - 95) < (int)gamedata.formulas.size())
            P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y, SW, soft, game_text(gamedata.formulas[3 * (it.id - 95)].text.c_str(), gamedata.formulas[3 * (it.id - 95)].text.size()));
    } else if (V.what == 2 && V.which >= 0 && V.which < 19) {
        std::string help = ui_skill_help(V.which);
        if (V.which < 7) { std::string w = rules_weapons_of_skill(V.which); help = (V.which >= 4 ? "用…瞄准" : "用这些武器进行近身攻击与防御") + (w.empty() ? std::string("these weapons") : w) + "."; }
        y += P.para(f, NsDraw::FS_NAME, SX0 + 2.5f, y, SW, ink, gm_skill_name(V.which)) + 0.5f;
        y += P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y, SW, soft, help) + 0.8f;
        y += raised_line(P, s, false, V.which, SX0 + 2.5f, y, SW);
        party_line(P, s, false, V.which, SX0 + 2.5f, y, SX1 - 2);
    } else if (V.what == 5 && V.which >= 0 && V.which < 7) {
        y += P.para(f, NsDraw::FS_NAME, SX0 + 2.5f, y, SW, ink, gm_attr_name(V.which)) + 0.5f;
        y += P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y, SW, soft, ui_attr_help(V.which)) + 0.8f;
        y += raised_line(P, s, true, V.which, SX0 + 2.5f, y, SW);
        party_line(P, s, true, V.which, SX0 + 2.5f, y, SX1 - 2);
    } else if (V.tab == TAB_THINGS && s.shared)
        P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y + 1, SW, dim, "将鼠标指向某件物品即可在此查看；按F11可显示你能进行的操作。");
    else if (V.tab == TAB_THINGS)
        P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y + 1, SW, dim, "点击某项属性或技能，即可查看其效果及全队的相关数值。金色圆点：表示该队伍在该项上表现最佳；蓝色：表示曾由圣人短暂提升过。");
    else {
        y += P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y + 1, SW, dim, s.npar > 1 ? "点击某项属性或技能，即可查看其效果。蓝色：表示曾由圣人短暂提升过。"
                                                                                : "当前生效：") + 1.8f;
        std::vector<std::string> at;                     /* the blessings and potions at work on this member, as many as there is room for */
        for (const Eff &e : EFF) if (e.m == s.who && e.kind != 0x43) at.push_back(e.what + " (" + hours_left(e.left) + ")");
        if (!at.empty()) {
            std::string t = "；以及";
            size_t k = 0;
            auto tail = [&](size_t left) { return left ? "; and " + std::to_string(left) + "更多（请参阅页边的“祝福与药剂”）。" : std::string("."); };
            for (; k < at.size(); k++) {
                std::string next = t + (k ? "; " : "") + at[k];
                if (k && P.para_height(fo, NsDraw::FS_SMALL, SW, next + tail(at.size() - k - 1)) > SY1 - y - 1) break;
                t = next;
            }
            t += tail(at.size() - k);
            P.para(fo, NsDraw::FS_SMALL, SX0 + 2.5f, y, SW, RAISED, t);
        }
    }
    if (s.note[0] && plat_ticks_ms() - s.note_t < 6000) {
        P.rect(SX0 + 1, SY1 - 8.6f, SX1 - 1, SY1 - 0.6f, IM_COL32(230, 214, 178, 255));
        P.text(fo, NsDraw::FS_SMALL, SX0 + 2.5f, SY1 - 7.8f, IM_COL32(112, 40, 22, 255), ns_gtext(s.note), -1, SX1 - 1);
    }
}

/* the details of a thing in use in a fight (ui_item_details: the numbers, who in the party could
   use it, what armour stops), and about what a shop pays for it */
static void gear_tip(const SvItem &it)
{
    const ItemDef *d = ns_itemdef(it.id);
    if (!d || !((d->type >= 0 && d->type <= 33 && (d->kind == K_WEAPON || d->kind == K_MISSILE || d->kind == K_RELIC)) ||
                (d->type >= 67 && d->type <= 84) || (d->type >= 95 && d->type <= 97))) return;
    ns_tip_begin(30);
    ImGui::TextUnformatted(ns_item_name(it.id).c_str());
    if (d->value) ImGui::TextDisabled("价值约 %s", money_with_total(d->value).c_str());
    ui_item_details(d, it.id, it.qual);
    ns_tip_end();
}

static void things(NsDraw &P, const SheetModel &s, int &new_what, int &new_which)
{
    ImFont *f = P.f, *fo = P.fo;
    const ImU32 ink = NsDraw::INK, soft = NsDraw::INK_SOFT, dim = NsDraw::INK_DIM;
    const float RX0 = 200, RX1 = 318, LY0 = 21.5f, ROWH = 8.0f; const int NROWS = 16;
    const float LY1 = LY0 + NROWS * ROWH;
    /* the rows: a heading, then those in use; a heading, then the rest */
    std::vector<int> rows;                                 /* item index; -1 - section: a section's heading */
    /* each section in the tidy order of the party's inventory page: by kind, then name, the better first */
    auto tidy = [&](size_t from) { std::stable_sort(rows.begin() + from, rows.end(), [&](int a, int b) { return ui_item_tidy_less(s.item[a].id, s.item[a].qual, s.item[b].id, s.item[b].qual); }); };
    /* with the shared pack: what is in use, then the member's own things not in use (the potions
       kept with them: SvItem.potion) in their sections, then the pack under its own heading */
    auto section = [&](int sec, int part) {               /* part 0 in use, 1 the member's own, 2 the pack (or all the rest) */
        size_t head = rows.size(); rows.push_back(-1 - sec);
        for (int i = 0; i < s.nitems; i++) {
            const SvItem &it = s.item[i];
            bool in = part == 0 ? it.equipped != 0
                    : it.equipped == 0 && ns_item_section(it.id) == sec && (!s.shared || (part == 1) == (it.holder >= 0));
            if (in) rows.push_back(i);
        }
        if (rows.size() == head + 1) rows.pop_back(); else tidy(head + 1);
    };
    section(0, 0);
    if (s.shared) {
        for (int sec = 1; sec < 6; sec++) section(sec, 1);
        bool more = false; for (int i = 0; i < s.nitems; i++) if (!s.item[i].equipped && s.item[i].holder < 0) more = true;
        if (more) rows.push_back(-10);                     /* (the pack's sections under one heading) */
        for (int sec = 1; sec < 6; sec++) section(sec, 2);
    } else for (int sec = 1; sec < 6; sec++) section(sec, 2);
    int n = (int)rows.size();
    int &sc = V.scroll[TAB_THINGS];
    if (ns_in.wheel != 0) { sc -= (int)(ns_in.wheel * 2); ns_in.wheel = 0; }
    int maxs = n - NROWS; if (maxs < 0) maxs = 0;
    if (sc > maxs) sc = maxs;
    if (sc < 0) sc = 0;
    P.rect(RX0, LY0 - 0.5f, RX1, LY1 + 0.5f, IM_COL32(250, 242, 222, 255), 1.5f);
    if (!s.nitems) P.text(fo, NsDraw::FS_BODY, RX0 + 3, LY0 + 2, dim, "什么都没携带。");
    /* a press on a row may become a drag */
    if (ns_in.pressed) {
        ns_in.pressed = 0;
        V.drag_row = -1;
        if (ns_in.px >= RX0 && ns_in.px < RX1 - 40 && ns_in.py >= LY0 && ns_in.py < LY1) {
            int r = (int)((ns_in.py - LY0) / ROWH) + sc;
            if (r >= 0 && r < n && rows[r] >= 0) V.drag_row = rows[r];   /* (the shared pack's too: a mount goes to whoever it is dropped on; the rest is everyone's already) */
        }
    }
    if (V.drag_row >= 0 && ns_in.down && !V.dragging && (abs(ns_in.x - ns_in.px) + abs(ns_in.y - ns_in.py)) > 4) { V.dragging = true; sheetview_dragging = 1; }
    if (!ns_in.down && !P.click && V.dragging) { V.dragging = false; sheetview_dragging = 0; V.drag_row = -1; }
    if (V.dragging && P.click) {                         /* let go: on a member at the left, a give */
        int to = partypanel_member_at(P.cx, P.cy);
        if (to >= 0 && V.drag_row < s.nitems) {
            const SvItem &it = s.item[V.drag_row];
            if (it.holder >= 0 && to != s.who) ns_push(SV_GIVE, it.idx, to, P.shift ? 1 : 255);   /* (the member's record, not the row) */
            else if (it.holder < 0 && it.type == 108) ns_push(SV_PTAKE, it.id, it.qual, to);     /* (a spare mount of the pack: theirs to ride) */
        }
        P.click = false; V.dragging = false; sheetview_dragging = 0; V.drag_row = -1;
    }
    int tip_i = -1;
    const bool cmp_on = config_get_int("display.shop_compare", 1) != 0;
    static int hover_carried = -1;                         /* (the carried row pointed at last frame: what it would replace is lit) */
    const int cmp_hover = hover_carried;
    int cmp_slot = cmp_on && hover_carried >= 0 && hover_carried < s.nitems && !s.item[hover_carried].equipped ? ns_slot_of(s.item[hover_carried].id) : 0;
    hover_carried = -1;
    for (int r = 0; r < NROWS && sc + r < n; r++) {
        int i = rows[sc + r];
        float y = LY0 + r * ROWH;
        if (i < 0) {                                       /* a heading */
            int sec = -1 - i;
            std::string hds = ns_item_section_name(sec);
            if (i == -10) {                                   /* the shared pack's own heading, once, above its sections: a band across, a pack and the words in the middle */
                const char *pk = "队伍的背包";
                float fs = NsDraw::FS_HEAD * 0.85f, tw = P.width(f, fs, pk), ih = 6.6f, iw = ih * 0.9f * P.k / P.kx, gapw = 2.0f;
                float cx = (RX0 + RX1) * 0.5f, x0 = cx - (iw + gapw + tw) * 0.5f;
                P.rect(RX0 + 1, y + 0.6f, RX1 - 1, y + ROWH - 0.6f, IM_COL32(222, 198, 150, 255));       /* (the band) */
                P.rect(RX0 + 1, y + 0.6f, RX1 - 1, y + 1.1f, IM_COL32(120, 80, 36, 200));
                P.rect(RX0 + 1, y + ROWH - 1.1f, RX1 - 1, y + ROWH - 0.6f, IM_COL32(120, 80, 36, 200));
                ns_backpack_icon(P, x0 + iw * 0.5f, y + ROWH * 0.5f, ih);
                P.text(f, fs, x0 + iw + gapw, y + 1.2f, ink, pk);
                continue;
            }
            const char *hd = hds.c_str();
            P.text(fo, NsDraw::FS_SMALL, RX0 + 3, y + 2.2f, soft, hd);
            float lw = 0;
            if (sec == 0) {                                 /* Equipped: the load, and what the thing pointed at would make it */
                int after = cmp_hover >= 0 && cmp_hover < s.nitems && !s.item[cmp_hover].equipped ? ns_load_with(s.item[cmp_hover].id, s.who) : -1;
                lw = ns_load_note(P, RX1 - 3, y + 2.2f, s.who, after) + 3;
            }
            P.rect(RX0 + 3 + P.width(fo, NsDraw::FS_SMALL, hd) + 2, y + 4.8f, RX1 - 3 - lw, y + 5.1f, IM_COL32(120, 80, 36, 90));
            continue;
        }
        const SvItem &it = s.item[i];
        bool h = !V.dragging && P.over(RX0, y, RX1, y + ROWH), busy = i == V.drop_row;
        if (h) { new_what = 1; new_which = i; hover_carried = i; }
        if (h || busy || (V.dragging && i == V.drag_row)) P.rect(RX0 + 0.5f, y, RX1 - 0.5f, y + ROWH, IM_COL32(226, 204, 160, 255));
        P.icon(RX0 + 5.5f, y + ROWH * 0.5f, 6.3f, it.id, it.qual);
        std::string nm = ns_item_name(it.id);
        if (it.count > 1) nm += " (" + std::to_string(it.count) + ")";
        const ItemDef *d = ns_itemdef(it.id);
        float name_end = (h || busy) ? RX1 - 59 : RX1 - 2;
        if (h && P.mx < name_end) tip_i = i;
        if (it.kept || it.sell) {                          /* kept from selling and dropping: a small red seal at the right; marked to sell: a coin */
            float kx = (h || busy) ? RX1 - 61 : RX1 - 4;
            ImVec2 kc = NsDraw::w(kx, y + ROWH * 0.5f);
            if (it.kept) P.dl->AddCircleFilled(kc, 1.5f * P.k, IM_COL32(150, 40, 24, 230));
            else { P.dl->AddCircleFilled(kc, 1.5f * P.k, IM_COL32(214, 168, 60, 255)); P.dl->AddCircle(kc, 1.5f * P.k, IM_COL32(120, 84, 30, 255), 0, fmaxf(1.0f, 0.3f * P.k)); }
            name_end -= 4;
        }
        /* (display.shop_compare) a weapon, armour or shield carried: better or worse than what this member has there now,
           as in a shop; the one pointed at lights what it would replace */
        int vs = !it.equipped && cmp_on ? ns_against_worn(it.id, it.qual, s.who) : 0;
        if (it.equipped == 1 && cmp_slot && ns_slot_of(it.id) == cmp_slot) {
            float pl = 0.5f + 0.5f * sinf(plat_ticks_ms() * 0.008f);
            P.rect(RX0 + 0.5f, y, RX1 - 0.5f, y + ROWH, IM_COL32(214, 170, 60, (int)(40 + 50 * pl)));
        }
        float nw = P.width(f, NsDraw::FS_NAME, nm), qend = RX0 + 11 + nw;
        P.text(f, NsDraw::FS_NAME, RX0 + 11, y + 1.0f, ink, nm, -1, name_end - (vs ? 6 : 0));
        if (!ns_quality_tag(it.id, it.qual).empty()) {
            char qs[16]; snprintf(qs, sizeof qs, "%s", ns_quality_tag(it.id, it.qual).c_str());
            float qx = RX0 + 11 + nw + 2;
            if (qx + P.width(fo, NsDraw::FS_SMALL, qs) < name_end - (vs ? 6 : 0)) { P.text(fo, NsDraw::FS_SMALL, qx, y + 1.9f, soft, qs); qend = qx + P.width(fo, NsDraw::FS_SMALL, qs); }
        }
        if (vs) ns_compare_arrow(P.dl, fminf(qend + 4, name_end - 3), y + ROWH * 0.5f, vs, P.k);
        if (!(h || busy)) continue;
        /* the actions */
        bool gear = d && (d->kind == K_WEAPON || d->kind == K_MISSILE || d->kind == K_ARMOR || d->kind == K_SHIELD);
        bool pack = it.holder < 0;                         /* a row of the shared pack: the actions by kind and quality */
        float x = RX1 - 58;
        if (it.drinkable && !it.equipped) { if (P.button(x, y + 1, x + 18, y + ROWH - 1, "装备卸下", IM_COL32(52, 70, 120, 235), 4.4f)) { if (pack) ns_push(SV_PDRINK, it.id, it.qual, 0); else ns_push(SV_DRINK, it.idx, 0, 0); } }
        else if (it.equipped == 1) { if (P.button(x, y + 1, x + 18, y + ROWH - 1, "装备穿戴", IM_COL32(120, 84, 40, 235), 4.4f)) ns_push(SV_PUTAWAY, it.idx, 0, 0); }
        else if (it.equipped == 2) P.text(fo, NsDraw::FS_SMALL, x + 2, y + 2.4f, soft, "骑乘它");   /* (the shared pack: their own mount) */
        else if (pack && it.type == 108) {                 /* a spare mount: this member rides it (a better one than theirs) */
            int rq = -1; for (int k = 0; k < s.nitems; k++) if (s.item[k].equipped == 2 && s.item[k].holder == s.who) rq = s.item[k].qual;
            if (it.qual > rq && P.button(x, y + 1, x + 18, y + ROWH - 1, "骑术", IM_COL32(40, 92, 34, 235), 4.4f)) ns_push(SV_PTAKE, it.id, it.qual, s.who);
        }
        else if (gear) { if (P.button(x, y + 1, x + 18, y + ROWH - 1, "保留", IM_COL32(40, 92, 34, 235), 4.4f)) { if (pack) ns_push(SV_PREADY, it.id, it.qual, 0); else ns_push(SV_READY, it.idx, 0, 0); } }
        x += 19;
        /* Keep / Kept, and for what Sell spare gear would not sell by itself (potions, tools...) a third state, Sell:
           a click goes Keep, Kept, Sell, Keep */
        {   int mark = it.kept ? 1 : it.sell ? 2 : 0, next = mark == 0 ? 1 : mark == 1 && !gear ? 2 : 0;
            const char *lab = mark == 1 ? "保留" : mark == 2 ? "出售" : "保留";
            ImU32 bc = mark == 1 ? IM_COL32(150, 40, 24, 245) : mark == 2 ? IM_COL32(150, 112, 30, 245) : IM_COL32(120, 84, 40, 235);
            if (P.button(x, y + 1, x + 18, y + ROWH - 1, lab, bc, 4.4f)) ns_push(SV_KEEP, it.id, it.qual, next);
            if (P.over(x, y + 1, x + 18, y + ROWH - 1))
                ns_tip(mark == 1 ? (gear ? "保留不售也不掉落（出售多余装备时会跳过此物品）。点击可标记为改为出售。"
                                         : "标记为出售：在商店中出售多余装备时，会连同备用武器与护甲一并出售。点击可取消标记。")
                     : mark == 2 ? "保留：Darklands+不会出售（无论是单独出售还是通过“出售多余装备”）或掉落，直到您再次点击为止。"
                     : "掉落");
        }
        x += 19;
        if (it.kept) P.button(x, y + 1, x + 18, y + ROWH - 1, "确定？", IM_COL32(170, 150, 130, 200), 4.4f);   /* (greyed: kept) */
        else if (busy) {
            if (P.button(x, y + 1, x + 18, y + ROWH - 1, P.shift || it.count == 1 ? "确实要掉落1件吗？" : "为了", IM_COL32(150, 30, 18, 255), 4.4f)) {
                if (pack) ns_push(SV_PDROP, it.id, it.qual, P.shift ? 255 : 1); else ns_push(SV_DROP, it.idx, P.shift ? 255 : 1, 0);
                V.drop_row = -1;
            }
        } else if (P.button(x, y + 1, x + 18, y + ROWH - 1, "确定？", IM_COL32(120, 44, 24, 235), 4.4f)) { V.drop_row = i; V.act_seq = s.seq; }
    }
    if (n > NROWS) {
        float th = (LY1 - LY0) * NROWS / n, ty = LY0 + (LY1 - LY0 - th) * sc / (float)(n - NROWS);
        P.rect(RX1 - 1.4f, ty, RX1 - 0.3f, ty + th, IM_COL32(120, 80, 36, 140), 0.5f);
    }
    /* a weapon, armour or shield the pointer rests on: all its numbers, and the whole party's fit (after
       the Tooltip delay, as every tooltip: third_party/imgui BeginTooltipEx) */
    if (tip_i >= 0 && !V.dragging) gear_tip(s.item[tip_i]);
    if (s.ndrops > 0) {                                  /* take back what was left behind here */
        const SvDrop &d = s.drops[s.ndrops - 1];
        std::string l = "Undo: " + ns_gtext(d.name) + (d.count > 1 ? " (" + std::to_string(d.count) + ")" : "");
        float bw = P.width(P.fo, 4.4f, l) + 4;
        if (P.button(RX1 - bw, LY1 + 1.2f, RX1, LY1 + 8.2f, l, IM_COL32(120, 84, 40, 235), 4.4f)) ns_push(SV_UNDO, 0, 0, 0);
    }
    /* the thing being dragged, at the pointer */
    if (V.dragging && V.drag_row >= 0 && V.drag_row < s.nitems) {
        const SvItem &it = s.item[V.drag_row];
        int to = partypanel_member_at(ns_in.x, ns_in.y);
        bool packrow = it.holder < 0, mount = it.type == 108;
        bool ok = to >= 0 && (packrow ? mount : to != s.who);
        if (ok) P.dl->AddRect(NsDraw::w(1.2f, 40.0f * to + 1.2f), NsDraw::w(58.8f, 40.0f * to + 38.8f), IM_COL32(120, 220, 110, 255), 2 * P.k, 0, fmaxf(2.0f, 0.9f * P.k));
        float gx = ns_in.x + 5, gy = ns_in.y + 9;                  /* (below the pointer, not under it) */
        std::string l = ok ? (packrow ? "以便骑行" + ns_gtext(s.names[to]) + "赠予"
                                      : "：队伍的背包早已属于所有人" + ns_gtext(s.names[to]) + (P.shift || it.count == 1 ? "" : " (all " + std::to_string(it.count) + ")"))
                           : packrow && !mount ? ns_item_name(it.id) + "：将其丢给左侧的队员"
                           : ns_item_name(it.id) + (to == s.who ? "" : "知识");
        float lw = P.width(P.fo, NsDraw::FS_SMALL, l);
        if (gx + 12 + lw > 318) gx = 318 - 12 - lw;
        P.rect(gx, gy - 4, gx + 12 + lw + 2, gy + 4.5f, ok ? IM_COL32(222, 244, 214, 240) : IM_COL32(250, 242, 222, 235), 1.5f);
        P.icon(gx + 5, gy + 0.3f, 7, it.id, it.qual);
        P.text(P.fo, NsDraw::FS_SMALL, gx + 11, gy - 2.6f, NsDraw::INK, l);
    }
}

static void skills(NsDraw &P, const SheetModel &s, int &new_what, int &new_which)
{
    ImFont *fo = P.fo;
    const ImU32 ink = NsDraw::INK, soft = NsDraw::INK_SOFT;
    const float ROWH = 7.9f;
    P.rect(200, 21, 318, 150, IM_COL32(250, 242, 222, 255), 1.5f);
    struct Group { const char *name; int first, n; bool attr; };
    const Group COL1[2] = { { "属性", 0, 7, true }, { "能力", 7, 6, false } };
    const Group COL2[2] = { { "武器", 0, 7, false }, { "：美德已低于该圣人的要求", 13, 6, false } };
    for (int c = 0; c < 2; c++) {
        float x0 = c ? 259.5f : 201, x1 = c ? 317 : 257.5f, y = 21.6f;
        for (const Group &g : c ? COL2 : COL1) {
            P.text(fo, 4.3f, x0 + 1, y + 1.6f, soft, g.name);
            y += ROWH * 0.85f;
            for (int k = 0; k < g.n; k++) {
                int idx = g.first + k;
                bool h = P.over(x0, y, x1, y + ROWH);
                if (h) { P.rect(x0, y, x1, y + ROWH, IM_COL32(226, 204, 160, 255), 1); new_what = g.attr ? 5 : 2; new_which = idx; }
                ImVec2 ic = NsDraw::w(x0 + 3.2f, y + ROWH * 0.5f);
                extern int ui_icon_disc; ui_icon_disc = 0;                 /* (on the page: no disc behind) */
                if (g.attr) ui_attr_icon(P.dl, ic, 4.6f * P.k, idx); else ui_skill_icon(P.dl, ic, 4.6f * P.k, idx);
                ui_icon_disc = 1;
                const char *name = g.attr ? gm_attr_name(idx) : gm_skill_name(idx);
                char v[24];
                ImU32 vc = ink;
                if (g.attr) {
                    int cur = s.attr[idx], mx = s.attr_max[idx];
                    if (idx == 0 || idx == 1 || idx == 6) { snprintf(v, sizeof v, "%d/%d", cur, mx); if (cur < mx) vc = NsDraw::RED; }
                    else if (cur < mx) { snprintf(v, sizeof v, "%d/%d", cur, mx); vc = NsDraw::RED; }
                    else snprintf(v, sizeof v, "%d", cur);
                } else {
                    snprintf(v, sizeof v, "%d", s.skill[idx]);
                    /* a skill's number coloured by how high it is: faded at the bottom, deepening to a
                       strong green at 60 and over (a master's) */
                    float t = fminf(1.0f, fmaxf(0.0f, s.skill[idx] / 60.0f));
                    auto mix = [&](int a0, int a1) { return (int)(a0 + (a1 - a0) * t); };
                    vc = IM_COL32(mix(150, 30), mix(128, 110), mix(100, 40), 255);
                }
                if (raised_by(s.who, g.attr ? 0x48 : 0x49, idx, nullptr) > 0) vc = RAISED;   /* (a saint's help, for a while) */
                float vw = P.width(fo, 4.8f, v);
                P.text(fo, 4.8f, x0 + 7, y + 1.4f, ink, name, -1, x1 - vw - 4);
                P.text(fo, 4.8f, 0, y + 1.4f, vc, v, x1 - 1.2f);
                if (party_best(s, g.attr, idx) == 1)          /* the party's best at it: a gold dot */
                    P.dl->AddCircleFilled(NsDraw::w(x1 - 1.2f - vw - 1.8f, y + ROWH * 0.5f), 0.95f * P.k, BEST);
                y += ROWH;
            }
            y += 1.2f;
        }
    }
}

/* a small outlined tag in a list's row, ending at x_end (the initials' size); returns its width */
/* who knows a saint or a formula: each one's head, from the figure beside a battle (no initials: two names may
   share a letter); the one shown on the sheet underlined, a grey one faded (dim) */
const GamePic *ns_figure_head(int cache, const char *code, const unsigned char *mp, ImVec2 *uv0, ImVec2 *uv1);
static float head_tag(NsDraw &P, int m, float x1, float ry, float rh, bool who, bool dim)
{
    char code[8]; gm_member_figure_code(m, code, sizeof code);
    unsigned char mp[24]; gm_member_palette(m, mp);
    ImVec2 u0, u1;
    const GamePic *g = ns_figure_head(60 + m, code, mp, &u0, &u1);
    float sz = rh - 1.2f, x0 = x1 - sz;
    if (g) P.dl->AddImage((ImTextureID)(intptr_t)g->tex, NsDraw::w(x0, ry + 0.6f), NsDraw::w(x1, ry + 0.6f + sz), u0, u1, dim ? IM_COL32(255, 255, 255, 90) : IM_COL32(255, 255, 255, 255));
    if (who) P.dl->AddLine(NsDraw::w(x0 + 0.5f, ry + rh - 0.3f), NsDraw::w(x1 - 0.5f, ry + rh - 0.3f), NsDraw::RED, 0.8f * P.k);
    if (P.over(x0, ry, x1, ry + rh)) ns_tip((ns_gtext(sheet_model.names[m]) + (dim ? "只有其最大值的一半" : "")).c_str());
    return sz;
}

static float small_tag(NsDraw &P, float x_end, float ry, float rowh, const char *label)
{
    float w = P.width(P.fo, 4.2f, label) + 2.4f;
    P.frame(x_end - w, ry + 1.3f, x_end, ry + rowh - 1.2f, NsDraw::GREEN, 1.1f);
    P.text(P.fo, 4.2f, x_end - w + 1.2f, ry + 1.7f, NsDraw::GREEN, label);
    return w;
}

/* a saint's help in short, for its row on the Saints page: what saint_info_does says, a chip a part -
   a shield and how much it gives, from + to +++ against what the other saints give of it (- to --- what it
   costs), healing (a green cross before the shield), the town's opinion, the weapon skills (all of them:
   three shields; those of the weapons in hand: "in hand"), a few words for the rest; the most first, the
   part in full under the pointer, the whole account in the panel at the right */
enum { CH_RAISE, CH_HEAL, CH_REP, CH_WEAPONS, CH_INHAND, CH_WORDS };
struct SaintChip {
    int kind, key; std::string val; bool bad; std::string part;
    bool raw = false;                                         /* words the parse did not know (usaintchips) */
    float mag = 0; int tier = 0;                              /* how much (chip_mag); 1..3 against the other saints */
};
/* a value written "6 至 11" becomes "6\x0111": the two ends of a range, as chip_mag reads them */
/* a value written "6 至 11" becomes "6\x0111": the two ends of a range, as chip_mag reads them */
static std::string span_of(const std::string &t)
{
    std::string s;
    for (char c : t) if (c != ' ' && c != '\t') s += c;
    static const char *const sep[] = { "至", "到", "~", "－", "—", "–" };
    for (const char *x : sep) {
        size_t p = s.find(x);
        if (p != std::string::npos) return s.substr(0, p) + "\x01" + s.substr(p + strlen(x));
    }
    for (size_t i = 1; i + 1 < s.size(); i++)                       /* 6-11 */
        if (s[i] == '-' && s[i - 1] >= '0' && s[i - 1] <= '9' && s[i + 1] >= '0' && s[i + 1] <= '9')
            return s.substr(0, i) + "\x01" + s.substr(i + 1);
    return s;
}

/* the attribute or skill a Chinese name in a saint's help stands for */
static const struct { const char *word; int key; } SAINT_KEYS[] = {
    { "通用语", 17 }, { "拉丁语", 18 }, { "读写能力", 19 }, { "读写", 19 },
    { "炼金术", 14 }, { "炼金", 14 }, { "木系知识", 25 }, { "木艺", 25 },
    { "街头智慧", 23 }, { "巧思", 21 }, { "工艺", 21 },
    { "投射装置", 13 }, { "投射", 13 }, { "弓箭", 12 }, { "弓", 12 },
    { "长柄武器", 10 }, { "长柄", 10 }, { "投掷", 11 }, { "连枷", 9 }, { "刃器", 7 }, { "冲击", 8 },
    { "潜行", 22 }, { "骑术", 24 }, { "宗教", 15 }, { "美德", 16 }, { "治疗", 20 },
    { "耐力", 0 }, { "体力", 0 }, { "力量", 1 }, { "敏捷", 2 }, { "感知", 3 }, { "智力", 4 },
    { "魅力", 5 }, { "神恩", 6 }, { "虔诚", 6 }, { "声望", KEY_REP },
    { 0, 0 }
};

static int saint_key_of(const std::string &name)
{
    if (name.empty()) return -1;
    for (int i = 0; SAINT_KEYS[i].word; i++)
        if (name.find(SAINT_KEYS[i].word) != std::string::npos) return SAINT_KEYS[i].key;
    return -1;
}

static bool saint_digit(const std::string &s)
{
    for (char c : s) if (c >= '0' && c <= '9') return true;
    return false;
}

static void saint_trim(std::string &s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(0, 1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
}

static size_t saint_utf8_len(unsigned char c)
{
    return c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
}

/* a saint's help, one sentence at a time */
static void saint_clauses(const std::string &t, std::vector<std::string> &out)
{
    static const char *const sep[] = { "，", "；", "。", "：", "（", "）" };
    std::string cur;
    for (size_t i = 0; i < t.size();) {
        unsigned char c = (unsigned char)t[i];
        size_t adv = saint_utf8_len(c);
        bool cut = false;
        if (c < 0x80) {
            if (c == '\n' || c == ',' || c == ';' || c == '.' || c == '(' || c == ')') cut = true;
        } else {
            for (const char *x : sep)
                if (t.compare(i, strlen(x), x) == 0) { cut = true; adv = strlen(x); break; }
        }
        if (cut) { saint_trim(cur); if (!cur.empty()) out.push_back(cur); cur.clear(); i += adv; }
        else { cur.append(t, i, adv); i += adv; }
    }
    saint_trim(cur);
    if (!cur.empty()) out.push_back(cur);
}

/* "刃器、冲击与长柄武器" is three names */
static void saint_names(const std::string &t, std::vector<std::string> &out)
{
    static const char *const sep[] = { "以及", "、", "与", "和", "及" };
    std::string cur;
    for (size_t i = 0; i < t.size();) {
        unsigned char c = (unsigned char)t[i];
        size_t adv = saint_utf8_len(c);
        bool cut = false;
        if (c >= 0x80) {
            for (const char *x : sep)
                if (t.compare(i, strlen(x), x) == 0) { cut = true; adv = strlen(x); break; }
        }
        if (cut) { saint_trim(cur); if (!cur.empty()) out.push_back(cur); cur.clear(); i += adv; }
        else { cur.append(t, i, adv); i += adv; }
    }
    saint_trim(cur);
    if (!cur.empty()) out.push_back(cur);
}

/* the numbers of a clause, with the range separator kept: "6至11" */
static std::string saint_numbers(const std::string &c)
{
    std::string r;
    for (size_t i = 0; i < c.size();) {
        unsigned char ch = (unsigned char)c[i];
        size_t adv = saint_utf8_len(ch);
        if (ch >= '0' && ch <= '9') r += (char)ch;
        else if (c.compare(i, strlen("至"), "至") == 0) { r += "至"; adv = strlen("至"); }
        else if (c.compare(i, strlen("到"), "到") == 0) { r += "到"; adv = strlen("到"); }
        i += adv;
    }
    return r;
}

/* what a saint's help does, as a list of chips: which stat, how much */
static void saint_chips_parse(const std::string &t, std::vector<SaintChip> &out)
{
    std::vector<std::string> cl;
    saint_clauses(t, cl);
    for (const std::string &raw : cl) {
        std::string c = raw;
        saint_trim(c);
        if (c.empty()) continue;
        if (c.size() > 1 && c[0] == '@') {                          /* markup the mod resolves itself */
            size_t sp = c.find(' ');
            if (sp == std::string::npos) continue;
            c = c.substr(sp + 1);
            saint_trim(c);
            if (c.empty()) continue;
        }
        bool down = c.find("降低") != std::string::npos || c.find("减少") != std::string::npos;

        if (c.find("治愈") != std::string::npos || c.find("恢复") != std::string::npos) {  /* healing says how much of which stat */
            std::string v;
            size_t pc = c.find('%');
            if (pc != std::string::npos) {
                size_t b = pc;
                while (b > 0 && c[b - 1] >= '0' && c[b - 1] <= '9') b--;
                v = "%" + c.substr(b, pc - b);
            } else if (c.find("所有") != std::string::npos) {
                v = "all";
            } else if (c.find("一半") != std::string::npos || c.find("半数") != std::string::npos) {
                v = "half";
            } else {
                v = span_of(saint_numbers(c));
            }
            bool any = false;
            for (int k = 0; k <= 6; k++) {
                const char *kn = ui_key_name(k);
                if (kn && *kn && c.find(kn) != std::string::npos) { out.push_back({ CH_HEAL, k, v, false, raw }); any = true; }
            }
            if (!any) out.push_back({ CH_HEAL, 0, v, false, raw });
            continue;
        }

        size_t pm = std::string::npos;                              /* "魅力+6至11", "力量-3" */
        for (size_t i = c.size(); i-- > 0;) if (c[i] == '+' || c[i] == '-') { pm = i; break; }
        if (pm != std::string::npos && pm + 1 < c.size() && saint_digit(c.substr(pm + 1))) {
            std::string names = c.substr(0, pm), range = c.substr(pm + 1);
            std::string val = std::string(down || c[pm] == '-' ? "-" : "+") + span_of(range);
            if (names.find("武器技能") != std::string::npos || names.find("每种武器") != std::string::npos
                || names.find("每项武器") != std::string::npos || names.find("武器的技能") != std::string::npos) {
                if (names.find("手中") != std::string::npos) out.push_back({ CH_INHAND, -1, val, down, raw });
                else out.push_back({ CH_WEAPONS, -1, val, down, raw });
                continue;
            }
            if (names.find("属性") != std::string::npos) {
                for (int k = 0; k <= 6; k++) out.push_back({ CH_RAISE, k, val, down, raw });
                continue;
            }
            std::vector<std::string> ns;
            saint_names(names, ns);
            bool any = false;
            for (const std::string &n : ns) {
                int k = saint_key_of(n);
                if (k >= 0) { out.push_back({ CH_RAISE, k, val, down, raw }); any = true; }
            }
            if (any) continue;
        }

        if (c.find("声望") != std::string::npos && saint_digit(c)) {  /* reputation is spoken of in words */
            out.push_back({ CH_REP, KEY_REP, std::string(down ? "-" : "+") + span_of(saint_numbers(c)), down, raw });
            continue;
        }

        if (down && saint_digit(c)) {                               /* "感知降低5至9", no plus sign */
            std::string names = c.substr(0, c.find("降低") != std::string::npos ? c.find("降低") : c.find("减少"));
            std::string val = "-" + span_of(saint_numbers(c));
            std::vector<std::string> ns;
            saint_names(names, ns);
            bool any = false;
            for (const std::string &n : ns) {
                int k = saint_key_of(n);
                if (k >= 0) { out.push_back({ CH_RAISE, k, val, true, raw }); any = true; }
            }
            if (any) continue;
        }

        SaintChip w{};                                              /* a word the parse does not know */
        w.kind = CH_WORDS; w.key = -1; w.val = c; w.bad = false; w.part = raw; w.raw = true;
        out.push_back(w);
    }
}

/* one line of a saint's help, in words */
static std::string chip_line(const SaintChip &c)
{
    std::string v = c.val, range;
    size_t p = v.find('\x01');
    if (p != std::string::npos) range = v.substr(0, p) + " 至 " + v.substr(p + 1);
    else range = v;
    switch (c.kind) {
    case CH_WEAPONS: return "所有武器技能 " + range;
    case CH_INHAND:  return "手中武器技能 " + range;
    case CH_REP:     return "声望 " + range;
    case CH_HEAL: {
        const char *kn = ui_key_name(c.key);
        return std::string("治愈 ") + (kn && *kn ? kn : "") + " " + range;
    }
    case CH_RAISE: {
        const char *kn = ui_key_name(c.key);
        return std::string(kn && *kn ? kn : "") + " " + range;
    }
    default:         return c.part;
    }
}

/* what saint s's help does, an effect a line */
std::string saint_effects_text(int s)
{
    std::vector<SaintChip> cs;
    saint_chips_parse(saint_info_does(s), cs);
    std::string out;
    for (const SaintChip &c : cs) {
        std::string line = chip_line(c);
        saint_trim(line);
        if (line.empty()) continue;
        if (!out.empty()) out += "\n";
        out += line;
    }
    return out;
}

static float chip_mag(const SaintChip &c)
{
    if (c.kind == CH_WORDS) return 0;
    std::string v = c.val; float sign = 1;
    if (!v.empty() && (v[0] == '-' || v[0] == '+')) { if (v[0] == '-') sign = -1; v.erase(0, 1); }
    float m;
    if (v.empty()) m = 10;
    else if (v == "all") m = 100;
    else if (v == "half") m = 50;
    else if (v.rfind("to ", 0) == 0) m = 20;
    else if (v.back() == '%') m = (float)atoi(v.c_str());
    else {
        size_t d = v.find('\x01');
        int a = atoi(v.c_str()), b = d == std::string::npos ? a : atoi(v.c_str() + d + 1);
        m = (a + b) * 0.5f * (c.kind == CH_HEAL ? 3 : 1);
    }
    return sign * m;
}
/* which chips a chip is set against: the same attribute or skill (healing apart; all the weapon skills
   together), or where fewer than 4 saints give it, its kind (the attributes, the weapon skills, the other
   skills, healing); the costs together */
static int chip_pool(const SaintChip &c, bool group)
{
    if (c.mag < 0) return 900;
    switch (c.kind) {
    case CH_HEAL: return group ? 303 : 100 + c.key;
    case CH_REP: return KEY_REP;
    case CH_WEAPONS: case CH_INHAND: return group ? 301 : 200;
    case CH_RAISE: return !group ? (c.key >= 7 && c.key < 14 ? 200 : c.key) : c.key < 7 ? 300 : c.key < 14 ? 301 : 302;
    }
    return -1;
}
/* every saint's chips, set against one another (again when a fix changes the account) */
static std::vector<std::vector<SaintChip>> chip_sets;
static void saint_chips_update(void)
{
    static int last = -1;
    int sig = fix_is_on("sainttexts") | fix_is_on("armorday") << 1 | fix_is_on("fireward") << 2 | fix_is_on("pauls") << 3 | fix_is_on("saintrep") << 4 | (sheetview_shared() ? 32 : 0);
    if (sig == last && (int)chip_sets.size() == SAINT_INFO_N) return;
    last = sig;
    chip_sets.assign(SAINT_INFO_N, std::vector<SaintChip>());
    std::map<int, std::vector<float>> pool;
    for (int t = 0; t < SAINT_INFO_N; t++) {
        saint_chips_parse(saint_info_does(t), chip_sets[t]);
        for (SaintChip &c : chip_sets[t]) {
            c.mag = chip_mag(c);
            if (c.kind == CH_WORDS) continue;
            pool[chip_pool(c, false)].push_back(fabsf(c.mag));
            pool[chip_pool(c, true)].push_back(fabsf(c.mag));
        }
    }
    for (std::vector<SaintChip> &v : chip_sets) {
        for (SaintChip &c : v) {
            if (c.kind == CH_WORDS) continue;
            const std::vector<float> *pl = &pool[chip_pool(c, false)];
            if (pl->size() < 4) pl = &pool[chip_pool(c, true)];
            float less = 0, same = 0, m = fabsf(c.mag);
            for (float x : *pl) { if (x < m) less++; else if (x == m) same++; }
            float at = (less + 0.5f * same) / (float)pl->size();
            c.tier = at < 1.0f / 3 ? 1 : at < 2.0f / 3 ? 2 : 3;
        }
        std::stable_sort(v.begin(), v.end(), [](const SaintChip &a, const SaintChip &b) {   /* the most first, then the costs, the words last */
            int ga = a.kind == CH_WORDS ? 2 : a.mag < 0, gb = b.kind == CH_WORDS ? 2 : b.mag < 0;
            if (ga != gb) return ga < gb;
            if (a.tier != b.tier) return a.tier > b.tier;
            return fabsf(a.mag) > fabsf(b.mag);
        });
    }
}
static const std::vector<SaintChip> &saint_chips(int s)
{
    static const std::vector<SaintChip> none;
    saint_chips_update();
    return s >= 0 && s < (int)chip_sets.size() ? chip_sets[s] : none;
}
/* how much saint s gives of attribute or skill key (KEY_REP: the town's opinion), to put the saints in order: the
   best of its chips that raise or heal it (the weapon skills: every one, and those in hand) - its +, then a rise
   before healing, then how much; 0 none */
static float saint_effect(int s, int key)
{
    float best = 0;
    for (const SaintChip &c : saint_chips(s)) {
        if (c.mag <= 0) continue;
        bool hit = c.kind == CH_REP ? key == KEY_REP : c.kind == CH_WEAPONS || c.kind == CH_INHAND ? key >= 7 && key < 14 : (c.kind == CH_RAISE || c.kind == CH_HEAL) && c.key == key;
        float score = c.tier * 1000.0f + (c.kind == CH_HEAL ? 0 : 500) + fminf(c.mag, 499);
        if (hit && score > best) best = score;
    }
    return best;
}
/* test (usaintchips): every saint's help through the parse, with the saints' fixes on and then off; the parts
   it did not know are logged, with a count at the end */
extern "C" void sheetview_check_saint_chips(void)
{
    static const char *IDS[] = { "sainttexts", "armorday", "fireward", "pauls", "saintrep" };
    int idx[5], was[5], n = 0, raw = 0;
    for (int f = 0; f < 5; f++) { idx[f] = -1; for (int i = 0; i < FIX_COUNT; i++) if (!strcmp(FIXES[i].id, IDS[f])) idx[f] = i; was[f] = idx[f] >= 0 ? FIXES[idx[f]].on : 0; }
    for (int pass = 1; pass >= 0; pass--) {
        for (int f = 0; f < 5; f++) if (idx[f] >= 0) FIXES[idx[f]].on = pass;   /* (the texts only: no hook, no setting touched) */
        for (int t = 0; t < SAINT_INFO_N; t++) {
            std::vector<SaintChip> ch; saint_chips_parse(saint_info_does(t), ch); n++;
            for (const SaintChip &c : ch) if (c.raw) { raw++; fprintf(stderr, "saintchips: %s (fixes %s) not understood: %s\n", gamedata.saints[t].name.c_str(), pass ? "on" : "off", c.part.c_str()); }
        }
    }
    for (int f = 0; f < 5; f++) if (idx[f] >= 0) FIXES[idx[f]].on = was[f];
    fprintf(stderr, "saintchips: %d helps, %d parts not understood\n", n, raw);
}
/* a chip's words, "\x01" a short dash drawn between two numbers (the typeface has no en dash); the width */
static float chip_text(NsDraw &P, float fs, float x, float y, ImU32 col, const std::string &t, bool draw)
{
    float w = 0;
    for (size_t a = 0; a <= t.size(); ) {
        size_t e = t.find('\x01', a);
        std::string part = t.substr(a, e == std::string::npos ? std::string::npos : e - a);
        if (draw) P.text(P.fo, fs, x + w, y, col, part);
        w += P.width(P.fo, fs, part);
        if (e == std::string::npos) break;
        if (draw) P.dl->AddRectFilled(NsDraw::w(x + w + 0.3f, y + fs * 0.5f), NsDraw::w(x + w + 1.5f, y + fs * 0.5f + 0.32f), col);
        w += 1.8f; a = e + 1;
    }
    return w;
}
/* the chips in a row from x to x1 at y (a line's middle); the part under the pointer, or "" */
static std::string saint_chips_draw(NsDraw &P, int s, float x, float x1, float y)
{
    std::string tip;
    const float fs = 3.9f, is = 4.4f;
    const ImU32 ink = NsDraw::INK_SOFT, bad = NsDraw::RED;
    extern int ui_icon_disc;
    /* the pictures' room, in the page's columns (a picture of size s rows is s * k / kx columns wide): a shield
       UI_SHIELD_W times its size (ui/skillicons.h), the cross before a healed one 0.7 of it, three fanned
       shields (the weapon skills: all of them, or those of the weapons in hand) each 0.35 of the size right of
       the one before; how much as chevronels on the shield (ui_icon_rank); the words only where there is no shield */
    const float ix = is * P.k / P.kx, sw = UI_SHIELD_W * ix, cw = ix * 0.7f, fw = UI_SHIELD_W * ix * 0.85f;
    for (const SaintChip &c : saint_chips(s)) {
        bool words = c.kind == CH_WORDS, fan = c.kind == CH_WEAPONS || c.kind == CH_INHAND;
        float w = words ? chip_text(P, fs, 0, 0, 0, c.val, false) : 0;
        float iw = fan ? fw + ix * 0.7f : c.kind == CH_HEAL ? cw + sw : words ? 0 : sw;
        if (x + iw + w > x1) { P.text(P.fo, fs, x, y - 2.1f, ink, "..."); break; }
        float x0 = x;
        int rank = c.tier < 1 ? 1 : c.tier; if (c.mag < 0) rank = -rank;
        ui_icon_disc = 0;
        if (c.kind == CH_HEAL) {                                  /* a green cross, then the shield */
            float cx = x + ix * 0.3f, rx = ix * 0.28f, r = is * 0.28f;
            P.dl->AddRectFilled(NsDraw::w(cx - rx * 0.33f, y - r), NsDraw::w(cx + rx * 0.33f, y + r), IM_COL32(40, 130, 50, 255));
            P.dl->AddRectFilled(NsDraw::w(cx - rx, y - r * 0.33f), NsDraw::w(cx + rx, y + r * 0.33f), IM_COL32(40, 130, 50, 255));
            ui_icon_rank = rank; ui_attr_icon(P.dl, NsDraw::w(x + cw + sw * 0.5f, y), is * P.k, c.key);
        } else if (c.kind == CH_RAISE) {
            ui_icon_rank = rank; ui_key_icon(P.dl, NsDraw::w(x + sw * 0.5f, y), is * P.k, c.key);
        } else if (c.kind == CH_REP) { ui_icon_rank = rank; ui_standing_icon(P.dl, NsDraw::w(x + sw * 0.5f, y), is * 0.92f * P.k, 0); }
        else if (fan) {                                           /* three of their shields, fanned; the front one ranked */
            ui_skill_icon(P.dl, NsDraw::w(x + fw * 0.5f, y), is * 0.85f * P.k, SKILL_BOW);
            ui_skill_icon(P.dl, NsDraw::w(x + fw * 0.5f + ix * 0.35f, y), is * 0.85f * P.k, SKILL_IMPACT);
            ui_icon_rank = rank; ui_skill_icon(P.dl, NsDraw::w(x + fw * 0.5f + ix * 0.7f, y), is * 0.85f * P.k, SKILL_EDGED);
        }
        ui_icon_rank = 0;
        ui_icon_disc = 1;
        if (words) chip_text(P, fs, x, y - 2.1f, c.bad ? bad : ink, c.val, true);
        x += iw + w + 1.4f;
        if (P.over(x0, y - 3.4f, x - 1.0f, y + 3.4f)) tip = c.part;
    }
    return tip;
}

static void saints(NsDraw &P, const SheetModel &s)
{
    ImFont *f = P.f, *fo = P.fo;
    const ImU32 ink = NsDraw::INK, soft = NsDraw::INK_SOFT, dim = NsDraw::INK_DIM;
    int ns = (int)gamedata.saints.size();
    /* the choices: known / all, what they help with */
    float x = 63;
    for (int a = 0; a < 2; a++) {
        const char *l = a ? "所有圣人" : "队伍中已知的";
        float w = P.width(fo, 4.4f, l) + 4;
        bool sel = V.saints_all == (a == 1);
        P.rect(x, 22, x + w, 28.5f, sel ? IM_COL32(112, 40, 22, 240) : IM_COL32(222, 204, 168, 255), 1.2f);
        P.text(fo, 4.4f, x + 2, 22.8f, sel ? NsDraw::LIGHT : ink, l);
        if (P.hit(x, 22, x + w, 28.5f)) { V.saints_all = a == 1; V.scroll[TAB_SAINTS] = 0; }
        x += w + 1;
    }
    x += 2;
    P.text(fo, 4.4f, x, 22.8f, soft, "有助于："); x += P.width(fo, 4.4f, "有助于：") + 1.5f;
    float y = 22;
    for (int u = 0; u < NUSES; u++) {
        float w = P.width(fo, 4.4f, USES[u].name) + 4;
        if (x + w > 318) { x = 63; y += 7.3f; }
        bool sel = V.use == u && !(u == 0 && V.use_k >= 0);   /* ("Any" not lit while a shield is chosen) */
        P.rect(x, y, x + w, y + 6.5f, sel ? IM_COL32(40, 70, 112, 240) : IM_COL32(222, 204, 168, 255), 1.2f);
        P.text(fo, 4.4f, x + 2, y + 0.8f, sel ? NsDraw::LIGHT : ink, USES[u].name);
        if (P.hit(x, y, x + w, y + 6.5f)) { V.use = u; V.use_k = -1; V.scroll[TAB_SAINTS] = 0; }
        x += w + 1;
    }
    {   /* or by a shield: the saints whose help touches that attribute or skill, or the town's opinion */
        y += 7.6f; x = 63;
        P.text(fo, 4.4f, x, y + 0.8f, soft, "或者它提升的："); x += P.width(fo, 4.4f, "或者它提升的：") + 1.5f;
        const float SW = 6.6f;
        for (int key = 0; key < SK_KEYS; key++) {
            int cnt = saint_key_count(key);
            if (!cnt) continue;                                  /* (Divine Favor: no saint's help raises it) */
            if (x + SW > 318) { x = 63; y += 7.3f; }
            bool sel = V.use_k == key;
            if (sel) P.rect(x - 0.3f, y - 0.2f, x + SW - 0.3f, y + 6.8f, IM_COL32(40, 70, 112, 240), 1.2f);
            ImVec2 ic = NsDraw::w(x + SW * 0.45f, y + 3.3f);
            extern int ui_icon_disc; ui_icon_disc = 0;
            if (key == KEY_REP) ui_standing_icon(P.dl, ic, 5.2f * P.k, 0);
            else if (key < 7) ui_attr_icon(P.dl, ic, 5.6f * P.k, key); else ui_skill_icon(P.dl, ic, 5.6f * P.k, key - 7);
            ui_icon_disc = 1;
            if (P.over(x - 0.3f, y - 0.2f, x + SW - 0.3f, y + 6.8f)) {
                char t[200]; snprintf(t, sizeof t, "%s：那位 %d 圣人%s，其帮助 %s 最为首要%s", key == KEY_REP ? "城镇对队伍的看法（声望）" : key < 7 ? gm_attr_name(key) : gm_skill_name(key - 7),
                                      cnt, cnt == 1 ? "" : "s", key == ATTR_END || key == ATTR_STR ? "提升或治愈" : "raises", sel ? "（再次点击以查看全部）" : "");
                ns_tip(t, 22);
            }
            if (P.hit(x - 0.3f, y - 0.2f, x + SW - 0.3f, y + 6.8f)) { V.use_k = sel ? -1 : key; V.use = 0; V.scroll[TAB_SAINTS] = 0; }
            x += SW;
        }
    }
    float LY0 = y + 8.5f, LX0 = 62, LX1 = 168, ROWH = 12.6f;   /* (a row: the name and who knows it, then the help in short) */
    int NROWS = (int)((197 - LY0) / ROWH);
    std::vector<int> rows;
    for (int t = 0; t < ns; t++) {
        bool known = false; for (int m = 0; m < 5; m++) if (s.present[m] && s.saint[m][t]) known = true;
        if (!V.saints_all && !known) continue;
        if (!saint_fits(t, V.use)) continue;
        if (V.use_k >= 0 && !saint_touches(t, V.use_k)) continue;
        rows.push_back(t);
    }
    if (V.use_k >= 0)                                             /* by a shield: who gives the most of it first */
        std::stable_sort(rows.begin(), rows.end(), [](int a, int b) { return saint_effect(a, V.use_k) > saint_effect(b, V.use_k); });
    int n = (int)rows.size();
    int &sc = V.scroll[TAB_SAINTS];
    if (ns_in.wheel != 0 && ns_in.x < LX1) { sc -= (int)(ns_in.wheel * 2); ns_in.wheel = 0; }
    int maxs = n - NROWS; if (maxs < 0) maxs = 0;
    if (sc > maxs) sc = maxs;
    if (sc < 0) sc = 0;
    P.rect(LX0, LY0 - 0.5f, LX1, LY0 + NROWS * ROWH + 0.5f, IM_COL32(250, 242, 222, 255), 1.5f);
    if (!n) P.para(fo, NsDraw::FS_SMALL, LX0 + 3, LY0 + 2, LX1 - LX0 - 6, dim, V.saints_all ? "没有圣人能帮助此事。" : "队伍中无人知晓能帮助此事的圣人。（显示所有圣人时也会列出其他人。）");
    int hover = -1; std::string chip_tip;
    int cl = (s16)g_rw(G_CUR_LOC);
    const Library *here = cl >= 0 && cl < G_NLOC ? library_at(cl) : nullptr;   /* (the library of the place the party is in: what it teaches, tagged) */
    for (int r = 0; r < NROWS && sc + r < n; r++) {
        int t = rows[sc + r];
        float ry = LY0 + r * ROWH;
        bool h = P.over(LX0, ry, LX1, ry + ROWH);
        if (h) hover = t;
        bool sel = t == (V.pin_saint >= 0 ? V.pin_saint : rows[0]);
        if (h || sel) P.rect(LX0 + 0.5f, ry, LX1 - 0.5f, ry + ROWH, sel ? IM_COL32(218, 196, 150, 255) : IM_COL32(226, 204, 160, 255));
        if (P.hit(LX0, ry, LX1, ry + ROWH)) V.pin_saint = t;
        /* who knows it: their heads, right */
        const float NH = 7.4f;                                   /* (the first line's height) */
        float ix = LX1 - 2;
        for (int m = 4; m >= 0; m--) {
            if (!s.present[m] || !s.saint[m][t]) continue;
            bool lowv = t < s.nsaints && s.saint_vmin[t] > 0 && s.pskill[m][9] < s.saint_vmin[t];   /* (Virtue below what the saint asks: faded) */
            ix -= head_tag(P, m, ix, ry, NH, m == s.who, lowv) + 0.6f;
        }
        bool known = false; for (int m = 0; m < 5; m++) if (s.present[m] && s.saint[m][t]) known = true;
        if (here && !known) for (int k = 0; k < 4; k++) if (here->saint[k] == t) {
            ix -= small_tag(P, ix, ry, NH, "此处教授") + 0.8f; break;
        }
        bool any_can = false, knowers = false;                   /* nobody who knows it has the Virtue it asks: slate grey */
        for (int m = 0; m < 5; m++) if (s.present[m] && s.saint[m][t]) { knowers = true; if (!(t < s.nsaints && s.saint_vmin[t] > 0 && s.pskill[m][9] < s.saint_vmin[t])) any_can = true; }
        int vmin = t < s.nsaints ? s.saint_vmin[t] : 0;
        std::string vtxt = vmin > 0 ? std::to_string(vmin) : "";
        const float VIS = 3.9f, VGS = 4.4f;
        float vw = vmin > 0 ? P.shield_value_width(VIS, vtxt, VGS) + 1.5f : 0;
        float nx1 = fminf(LX0 + 2.5f + P.width(f, 5.4f, gamedata.saints[t].name), ix - 1 - vw);
        P.text(f, 5.4f, LX0 + 2.5f, ry + 0.9f, knowers && !any_can ? IM_COL32(110, 116, 128, 255) : ink, gamedata.saints[t].name, -1, nx1);
        if (vmin > 0) {                                           /* the Virtue it asks, after the name: greyed when no member has it */
            bool met = false; std::string who;
            for (int m = 0; m < 5; m++) if (s.present[m]) { met |= s.pskill[m][9] >= vmin; who += (who.empty() ? "" : ", ") + ns_gtext(s.names[m]) + " " + std::to_string(s.pskill[m][9]); }
            int v0 = P.dl->VtxBuffer.Size;
            float bx = nx1 + 1.5f, bw = P.shield_value(bx, ry + 0.9f + 3.3f, VIS, UI_KEY_SKILL + SKILL_VIRTUE, vtxt, VGS, soft);
            if (!met) ui_dim_since(P.dl, v0, 0.38f);
            if (P.over(bx, ry, bx + bw, ry + NH)) {
                char b[240]; snprintf(b, sizeof b, "这位圣人向祈祷者索取%d点{美德}%s。\\n每位成员的{美德}：%s。", vmin, met ? "" : "，比队伍中任何人拥有的都多", who.c_str());
                chip_tip = b;
            }
        }
        std::string ct = saint_chips_draw(P, t, LX0 + 2.5f, LX1 - 2, ry + 9.4f);   /* the help, in short */
        if (!ct.empty()) chip_tip = ct;
    }
    if (!chip_tip.empty()) ns_tip(chip_tip.c_str(), 22);
    if (n > NROWS) {
        float LY1 = LY0 + NROWS * ROWH, th = (LY1 - LY0) * NROWS / n, ty = LY0 + (LY1 - LY0 - th) * sc / (float)(n - NROWS);
        P.rect(LX1 - 1.4f, ty, LX1 - 0.3f, ty + th, IM_COL32(120, 80, 36, 140), 0.5f);
    }
    /* the saint: the statue, the name, who knows it (or where it is taught), what the help does
       (mods/saintinfo.h) and where the text says otherwise, whether encounters call on it, the legend */
    (void)hover;
    bool pinned_shown = false; for (int r : rows) if (r == V.pin_saint) pinned_shown = true;
    int t = pinned_shown ? V.pin_saint : n ? rows[0] : -1;      /* (the one clicked; else the first) */
    sheet_model.pray_ask = t;                                     /* (each member's chance with it: mods/sheetview.c) */
    const float DX0 = 171, DX1 = 318, DY0 = LY0 - 0.5f, DY1 = 197;
    P.rect(DX0, DY0, DX1, DY1, IM_COL32(236, 224, 196, 255), 1.5f);
    if (t < 0 || t >= ns) return;
    std::string who;
    for (int m = 0; m < 5; m++) if (s.present[m] && s.saint[m][t]) who += (who.empty() ? "" : ", ") + ns_gtext(s.names[m]);
    std::string where;                                   /* nobody knows it: the libraries that teach it (of the places the party knows: ui/libraries.h) */
    if (who.empty()) {
        std::vector<LibraryHit> lh = libraries_teaching(t);
        if (lh.empty()) where = "队伍去过的任何图书馆都不教授这位圣人。";
        else {
            where = "在……的图书馆教授";
            size_t k = std::min(lh.size(), (size_t)3);
            for (size_t i = 0; i < k; i++) {
                int d = (int)(lh[i].days + 0.5);
                std::string ds = lh[i].days < 0 ? "" : lh[i].days < 0.6 ? " (here)" : " (" + std::to_string(d) + (d == 1 ? " day)" : " days)");
                where += (i ? (i + 1 == k && lh.size() <= 3 ? " and " : ", ") : "") + lh[i].lib->name + ds;
            }
            if (lh.size() > 3) where += " and " + std::to_string(lh.size() - 3) + " more";
            where += ".";
        }
    }
    const std::string &lore = gamedata.saints[t].text;
    float lore_h = P.para_height(fo, 4.4f, DX1 - DX0 - 5, lore) + 6;   /* (and the note under it) */
    const GamePic *g = t < s.nsaints ? statue(s.saint_class[t]) : nullptr;
    float tx = DX0 + 2, sh = fmaxf(56, fminf(116, DY1 - DY0 - 4 - lore_h));   /* the statue smaller for a long legend */
    if (g) {
        float sw = g->w * sh / g->h;                        /* (the statue in its niche) */
        P.dl->AddImageRounded((ImTextureID)(intptr_t)g->tex, NsDraw::w(DX0 + 2, DY0 + 1.5f), NsDraw::w(DX0 + 2 + sw, DY0 + 1.5f + sh), ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, 1.5f * P.k);
        tx = DX0 + 4 + sw;
    }
    P.dl->PushClipRect(NsDraw::w(DX0, DY0), NsDraw::w(DX1, DY1), true);
    float ty = DY0 + 2, tw = DX1 - tx - 2;
    ty += P.para(f, NsDraw::FS_HEAD, tx, ty, tw, ink, gamedata.saints[t].name) + 1.5f;
    ty += P.para(fo, NsDraw::FS_SMALL, tx, ty, tw, who.empty() ? NsDraw::RED : soft, who.empty() ? "队伍中无人知晓这位圣人。" : "由…所知" + who + ".") + 1;
    if (t < s.nsaints && s.saint_vmin[t] > 0) {                /* the Virtue it asks; who lacks it (Virtue rises slowly: nothing restores it) */
        std::string low;
        for (int m = 0; m < 5; m++) if (s.present[m] && s.saint[m][t] && s.pskill[m][9] < s.saint_vmin[t]) low += (low.empty() ? "" : ", ") + ns_gtext(s.names[m]) + " (" + std::to_string(s.pskill[m][9]) + ")";
        char b[200]; snprintf(b, sizeof b, low.empty() ? "索取%d点{美德}。" : "索取%d点{美德}：对于无法召唤此圣人的%s而言过低。", s.saint_vmin[t], low.c_str());
        ty += P.para(fo, 4.4f, tx, ty, tw, low.empty() ? soft : IM_COL32(90, 98, 112, 255), b) + 1;
    }
    if (!where.empty()) ty += P.para(fo, 4.4f, tx, ty, tw, soft, where) + 1.5f;
    else {                                               /* each who knows it: the best chance, the favor it takes, Pray (the original: a click on the saint) */
        const float bh = 7.5f;
        for (int m = 0; m < 5; m++) {
            if (!s.present[m] || !s.saint[m][t]) continue;
            float hx = tx + head_tag(P, m, tx + 6.5f, ty, bh + 0.6f, false, false) + 1.2f;
            char b[96];
            bool asked = s.pray_saint == t && s.pray_best[m] >= 0;
            if (!asked) snprintf(b, sizeof b, "%s", ns_gtext(s.names[m]).c_str());
            else if (s.pray_best[m] > 0) snprintf(b, sizeof b, "%s：%d%%，拥有%d分之%d的青睐", ns_gtext(s.names[m]).c_str(), s.pray_best[m], s.pray_df[m], s.df[m]);
            else snprintf(b, sizeof b, "%s：目前无机会", ns_gtext(s.names[m]).c_str());
            P.text(P.fo, NsDraw::FS_SMALL, hx, ty + 1.2f, asked && !s.pray_best[m] ? NsDraw::RED : ink, b, -1, DX1 - 16);
            if (P.over(hx, ty, DX1 - 16, ty + bh))
                ns_tip(asked && s.pray_best[m] > 0 ? "被听到的最佳几率，以及赋予它的最低神圣青睐：花费更多反而徒劳。祈祷界面决定青睐度。"
                                                   : "对于这位圣人而言，美德过低或神圣青睐不足：请参见上文及祈祷界面。", 22);
            if (s.able[m] && P.button(DX1 - 15, ty, DX1 - 2, ty + bh, "祈祷", IM_COL32(40, 70, 130, 235))) ns_push(SV_PRAY, m, t, 0);
            if (P.over(DX1 - 15, ty, DX1 - 2, ty + bh)) ns_tip("现在召唤这位圣人：通过祈祷界面，选择投入多少神圣青睐。", 20);
            ty += bh + 1.2f;
        }
        ty += 1;
    }
    ty += P.para(fo, NsDraw::FS_SMALL, tx, ty, tw, IM_COL32(26, 64, 132, 255), saint_effects_text(t)) + 1.5f;
    const char *un = saint_info_unlike(t);
    if (*un) ty += P.para(fo, 4.4f, tx, ty, tw, IM_COL32(140, 50, 30, 255), "与描述不符：" + rich_auto(un)) + 1.5f;
    ty += P.para(fo, 4.2f, tx, ty, tw, ns_tier_colour(saint_info_cards(t) >= 10 ? 20 : saint_info_cards(t) >= 3 ? 5 : saint_info_cards(t)), saint_encounters_line(t)) + 1.5f;
    float ly = fmaxf(ty, g ? DY0 + 1.5f + sh + 2 : ty);
    ly += P.para(fo, 4.4f, DX0 + 2.5f, ly, DX1 - DX0 - 5, soft, lore) + 1.5f;
    if (*saint_info_note(t)) P.para(fo, 4.0f, DX0 + 2.5f, fmaxf(ly, DY1 - 5.5f), DX1 - DX0 - 5, dim, saint_info_note(t));   /* (at the foot) */
    P.dl->PopClipRect();
}

/* the ingredients of formula k the party has enough of, as batches */
static int batches_of(const SheetModel &s, int k)
{
    if (k < 0 || k >= (int)gamedata.formulas.size()) return 0;
    int b = INT_MAX;
    for (const FormulaIngr &g : gamedata.formulas[k].ingr) {
        int have = g.item >= 0 && g.item < 200 ? s.have[g.item] : 0;
        if (g.count > 0) b = std::min(b, have / g.count);
    }
    return b == INT_MAX ? 0 : b;
}

/* the potions of formula k the party carries: of its kind, at its grade */
static int carried_of(const SheetModel &s, int k)
{
    int ui_potion_tier(int q, int base);
    int id = 95 + k / 3, n = 0;
    const ItemDef *d = ns_itemdef(id);
    for (int i = 0; i < s.npots; i++) if (s.pots[i].id == id && ui_potion_tier(s.pots[i].qual, d ? d->quality : 0) == k % 3 + 1) n += s.pots[i].count;
    return n;
}

/* an ingredient on the shelf: what the party has of it, who carries it, where it is sold, what uses it */
static void ingredient_tip(const SheetModel &s, int id)
{
    ns_tip_begin(22);
    ImGui::TextUnformatted(ns_item_name(id).c_str());
    int n = s.have[id];
    std::string by;
    for (int m = 0; m < 5; m++) if (s.present[m] && s.ingr[m][id - 72]) by += (by.empty() ? "" : ", ") + ns_gtext(s.names[m]) + " " + std::to_string(s.ingr[m][id - 72]);
    if (!n) ImGui::TextDisabled("队伍没有。");
    else if (s.shared) ImGui::Text("队伍拥有%d。", n);
    else ImGui::Text("队伍拥有%d（%s）。", n, by.c_str());
    std::string w = journal_where_sold(id);
    if (!w.empty()) ImGui::TextDisabled("购买地点：%s", w.c_str());
    std::string uses;                                   /* the formulae the party knows that call for it */
    int nu = 0;
    for (int k = 0; k < (int)gamedata.formulas.size() && k < 66; k++) {
        bool kn = false; for (int m = 0; m < 5; m++) if (s.present[m] && s.formula[m][k]) kn = true;
        if (!kn) continue;
        for (const FormulaIngr &g : gamedata.formulas[k].ingr) if (g.item == id) { if (nu < 4) uses += (uses.empty() ? "" : ", ") + gamedata.formula_names[k] + " (" + std::to_string(g.count) + ")"; nu++; }
    }
    if (nu > 4) uses += " and " + std::to_string(nu - 4) + " more";
    if (!uses.empty()) ImGui::TextWrapped("用于%s。", uses.c_str());
    ns_tip_end();
}

static void formulae(NsDraw &P, const SheetModel &s)
{
    ImFont *f = P.f, *fo = P.fo;
    const ImU32 ink = NsDraw::INK, soft = NsDraw::INK_SOFT, dim = NsDraw::INK_DIM;
    int nf = (int)std::min(std::min(gamedata.formula_names.size(), gamedata.formulas.size()), (size_t)66);
    /* the alchemists: everyone who knows a formula */
    std::vector<int> al;
    for (int m = 0; m < 5; m++) { if (!s.present[m]) continue; for (int k = 0; k < nf; k++) if (s.formula[m][k]) { al.push_back(m); break; } }
    auto known = [&](int k) { for (int m : al) if (s.formula[m][k]) return true; return false; };
    const float LX0 = 62, LX1 = 200, LY0 = 30.5f, ROWH = 7.6f; const int NROWS = 14;
    /* the choice: the formulae the party knows, or all of them (to judge one before buying it) */
    float x = LX0 + 1;
    for (int a = 0; a < 2; a++) {
        const char *l = a ? "所有配方" : "队伍中已知的";
        float w = P.width(fo, 4.4f, l) + 4;
        bool sel = V.formulae_all == (a == 1);
        P.rect(x, 22, x + w, 28.5f, sel ? IM_COL32(112, 40, 22, 240) : IM_COL32(222, 204, 168, 255), 1.2f);
        P.text(fo, 4.4f, x + 2, 22.8f, sel ? NsDraw::LIGHT : ink, l);
        if (P.hit(x, 22, x + w, 28.5f)) { V.formulae_all = a == 1; V.scroll[TAB_FORMULAE] = 0; }
        x += w + 1;
    }
    /* who knows each: their heads at the right of the row, as on the Saints page; when one member
       alone knows formulae, none (the heading says who) */
    bool tags = al.size() > 1;
    if (al.size() == 1) { std::string w = "由…所知" + ns_gtext(s.names[al[0]]); P.text(fo, 4.2f, 0, 24.2f, soft, w, LX1 - 2); }
    std::vector<int> rows;
    for (int k = 0; k < nf; k++) if (V.formulae_all || known(k)) rows.push_back(k);
    int n = (int)rows.size();
    int &sc = V.scroll[TAB_FORMULAE];
    if (ns_in.wheel != 0 && ns_in.x < LX1 && ns_in.y < LY0 + NROWS * ROWH) { sc -= (int)(ns_in.wheel * 2); ns_in.wheel = 0; }
    int maxs = n - NROWS; if (maxs < 0) maxs = 0;
    if (sc > maxs) sc = maxs;
    if (sc < 0) sc = 0;
    P.rect(LX0, LY0 - 0.5f, LX1, LY0 + NROWS * ROWH + 0.5f, IM_COL32(250, 242, 222, 255), 1.5f);
    if (!n) P.para(fo, NsDraw::FS_SMALL, LX0 + 3, LY0 + 2, LX1 - LX0 - 6, dim, "队伍中无人知晓任何配方。显示所有配方，包括其成分及销售地点。");
    int hover = -1;
    int cl = (s16)g_rw(G_CUR_LOC), sold[4] = { -1, -1, -1, -1 }, price[4];
    if (!(cl >= 0 && cl < (int)gamedata.cities.size() && alchemist_formulae(cl, sold, price))) sold[0] = sold[1] = sold[2] = sold[3] = -1;   /* (the town's alchemist: what it sells, tagged) */
    for (int r = 0; r < NROWS && sc + r < n; r++) {
        int k = rows[sc + r];
        float ry = LY0 + r * ROWH;
        bool h = P.over(LX0, ry, LX1, ry + ROWH), kn = known(k);
        if (h) hover = k;
        if (h || k == V.pin_formula) P.rect(LX0 + 0.5f, ry, LX1 - 0.5f, ry + ROWH, k == V.pin_formula ? IM_COL32(218, 196, 150, 255) : IM_COL32(226, 204, 160, 255));
        if (P.hit(LX0, ry, LX1, ry + ROWH)) V.pin_formula = V.pin_formula == k ? -1 : k;
        P.icon(LX0 + 4.5f, ry + ROWH * 0.5f, 5.8f, 95 + k / 3, -1, k % 3 + 1);
        float end = LX1 - 2;
        if (tags) for (int m = 4; m >= 0; m--) {              /* (who knows it: heads, as Saints) */
            if (!s.present[m] || !s.formula[m][k]) continue;
            end -= head_tag(P, m, end, ry, ROWH, m == s.who, false) + 0.6f;
        }
        end -= 1;
        int b = kn ? batches_of(s, k) : 0, have = carried_of(s, k);
        if (kn || have) {                                /* how many carried, and how many batches the ingredients carried make */
            std::string bs = "brew " + std::to_string(b);
            P.text(fo, NsDraw::FS_SMALL, 0, ry + 1.5f, b ? NsDraw::GREEN : dim, bs, end);
            end -= P.width(fo, NsDraw::FS_SMALL, bs) + 2.5f;
            std::string hs = "have " + std::to_string(have);
            P.text(fo, NsDraw::FS_SMALL, 0, ry + 1.5f, have ? ink : dim, hs, end);
            end -= P.width(fo, NsDraw::FS_SMALL, hs) + 1.5f;
        }
        if (!kn && (sold[0] == k || sold[1] == k || sold[2] == k || sold[3] == k)) {
            end -= small_tag(P, end, ry, ROWH, "在此出售") + 1;
        }
        P.text(f, 5.4f, LX0 + 9, ry + 0.9f, kn && have ? ink : dim, gamedata.formula_names[k], -1, end);   /* (grey: none carried, or not known) */
    }
    if (n > NROWS) {
        float LY1 = LY0 + NROWS * ROWH, th = (LY1 - LY0) * NROWS / n, ty = LY0 + (LY1 - LY0 - th) * sc / (float)(n - NROWS);
        P.rect(LX1 - 1.4f, ty, LX1 - 0.3f, ty + th, IM_COL32(120, 80, 36, 140), 0.5f);
    }
    int k = V.pin_formula >= 0 ? V.pin_formula : hover >= 0 ? hover : n ? rows[sc] : -1;   /* (one clicked stays shown until clicked again; then the one under the pointer) */
    if (k >= nf) k = -1;

    /* the party's ingredients: all 23 always in the same places, those it has none of faded; what
       the formula shown calls for ringed, green when there is enough */
    const float SY0 = LY0 + NROWS * ROWH + 2.5f, GY0 = SY0 + 6, cellw = (LX1 - LX0 - 2) / 8.0f, cellh = (197 - GY0) / 3.0f;
    P.text(fo, 4.3f, LX0 + 2, SY0, soft, "队伍的原料");
    int tip = -1;
    for (int i = 0; i < 23; i++) {
        int id = 72 + i, have = s.have[id], need = 0;
        if (k >= 0) for (const FormulaIngr &g : gamedata.formulas[k].ingr) if (g.item == id) need += g.count;
        float x0 = LX0 + 1 + (i % 8) * cellw, y0 = GY0 + (i / 8) * cellh, cxm = x0 + cellw * 0.5f;
        if (need) P.rect(x0 + 0.3f, y0 + 0.3f, x0 + cellw - 0.3f, y0 + cellh - 0.3f, have >= need ? IM_COL32(204, 226, 184, 255) : IM_COL32(238, 196, 180, 255), 1.5f);
        if (P.over(x0, y0, x0 + cellw, y0 + cellh)) { tip = id; P.rect(x0 + 0.3f, y0 + 0.3f, x0 + cellw - 0.3f, y0 + cellh - 0.3f, IM_COL32(226, 204, 160, 160), 1.5f); }
        float is = std::min(cellw * 0.5f, cellh * 0.46f);         /* (a picture is drawn up to a third wider than its size) */
        float iy = y0 + cellh * 0.36f;
        P.icon(cxm, iy, is, id);
        if (!have) P.dl->AddCircleFilled(NsDraw::w(cxm, iy), is * 0.7f * P.k, need ? IM_COL32(238, 196, 180, 160) : IM_COL32(242, 230, 200, 165));   /* (none: the sign faded) */
        std::string c = need ? std::to_string(have) + "/" + std::to_string(need) : have ? std::to_string(have) : "";   /* (none: the faded sign says it) */
        ImU32 cc = need ? (have >= need ? NsDraw::GREEN : NsDraw::RED) : ink;
        if (!c.empty()) P.text(fo, NsDraw::FS_SMALL, cxm - P.width(fo, NsDraw::FS_SMALL, c) * 0.5f, y0 + cellh * 0.68f, cc, c);
    }
    if (tip >= 0) ingredient_tip(s, tip);

    /* the formula: what it does, how hard and risky, its ingredients, the batches; who can brew
       it and their chance, or (nobody knows it) where it is sold */
    const float DX0 = 203, DX1 = 318, DY1 = 197, DW = DX1 - DX0 - 5;
    P.rect(DX0, 22, DX1, DY1, IM_COL32(236, 224, 196, 255), 1.5f);
    if (k < 0) return;
    const Formula &fm = gamedata.formulas[k];
    bool kn = known(k);
    P.dl->PushClipRect(NsDraw::w(DX0, 22), NsDraw::w(DX1, DY1), true);
    float y = 24;
    P.icon(DX0 + 6, y + 4, 8, 95 + k / 3, -1, k % 3 + 1);
    y += P.para(f, NsDraw::FS_HEAD, DX0 + 12, y, DW - 9, ink, gamedata.formula_names[k]) + 1.5f;
    static const char *RISK[3] = { "低风险", "存在一定风险", "高风险" };
    char mr[64]; snprintf(mr, sizeof mr, "神秘数字%d，%s。", fm.mystic, fm.risk >= 0 && fm.risk < 3 ? RISK[fm.risk] : "风险未知");
    float mh = P.para(fo, NsDraw::FS_SMALL, DX0 + 2.5f, y, DW, soft, mr);
    if (P.over(DX0, y, DX1, y + mh))
        ns_tip("神秘数字：数值越高，配方越难调制（与炼金术师的{炼金术}和{智力}相对应）。风险：失败后可能造成的严重后果。");
    y += mh + 1;
    y += P.para(fo, NsDraw::FS_SMALL, DX0 + 2.5f, y, DW, IM_COL32(26, 64, 132, 255), game_text(fm.text.c_str(), fm.text.size())) + 0.5f;
    { ImU32 ec; std::string enc = ns_item_encounters(95 + k / 3, &ec); if (!enc.empty()) y += P.para(fo, NsDraw::FS_SMALL, DX0 + 2.5f, y, DW, ec, enc); }
    y += 1.5f;
    P.text(fo, 4.3f, DX0 + 2.5f, y, soft, "成分：队伍现有/所需"); y += 6;
    for (const FormulaIngr &g : fm.ingr) {
        int have = g.item >= 0 && g.item < 200 ? s.have[g.item] : 0;
        P.icon(DX0 + 5, y + 3, 5.5f, g.item);
        P.text(fo, NsDraw::FS_SMALL, DX0 + 9.5f, y + 0.6f, ink, ns_item_name(g.item), -1, DX1 - 20);
        char v[16]; snprintf(v, sizeof v, "%d / %d", have, g.count);
        P.text(fo, NsDraw::FS_SMALL, 0, y + 0.6f, have >= g.count ? NsDraw::GREEN : NsDraw::RED, v, DX1 - 3);
        y += 6.6f;
    }
    int b = batches_of(s, k);
    y += 0.5f;
    if (b) { char bl[64]; snprintf(bl, sizeof bl, "足够的材料来制作%d个药水%s。", b, b == 1 ? "" : "s"); y += P.para(fo, NsDraw::FS_SMALL, DX0 + 2.5f, y, DW, NsDraw::GREEN, bl) + 1.5f; }
    else y += P.para(fo, NsDraw::FS_SMALL, DX0 + 2.5f, y, DW, dim, kn ? "队伍缺少某种成分：酿造需等待。" : "队伍缺少某种成分。") + 1.5f;
    if (kn) {
        P.text(fo, 4.3f, DX0 + 2.5f, y, soft, "制作一批的几率"); y += 6;
        for (int m : al) {
            if (!s.formula[m][k]) continue;
            int pc = s.intel[m] + (s.alch[m] - fm.mystic) + 100 + (s16)g_rw(GV_philosophers_stone);  /* (the alchemy screen's, one batch, with the party's Philosopher's Stone) */
            pc = pc < 0 ? 0 : pc > 99 ? 99 : pc;
            char v[64]; snprintf(v, sizeof v, "%s：%d%%", ns_gtext(s.names[m]).c_str(), pc);
            P.text(fo, NsDraw::FS_SMALL, DX0 + 4, y, ink, v); y += 6;
        }
    } else {                                             /* nobody knows it: where an alchemist sells it (of the towns the party knows: ui/libraries.h) */
        y += P.para(fo, NsDraw::FS_SMALL, DX0 + 2.5f, y, DW, NsDraw::RED, "队伍中无人知晓。") + 1;
        std::vector<SellerHit> sh = formula_sellers(k);
        std::string w;
        if (k >= 22) w = "没有炼金术士出售它：它是用来交易或发现的。";
        else if (sh.empty()) w = "队伍去过的所有炼金术士都不卖它。";
        else {
            int d = (int)(sh[0].days + 0.5);
            bool here = sh[0].days >= 0 && sh[0].days < 0.6;
            w = here ? "在这里出售，" + money_str(sh[0].price_fl * 240L) + "."
                     : "由……的炼金术士出售" + sh[0].name + (sh[0].days < 0 ? "" : ", " + std::to_string(d) + (d == 1 ? "距离……还有1天" : "天")) + ", " + money_str(sh[0].price_fl * 240L) + ".";
            if (sh.size() > 1) {
                w += "同时也在……";
                for (size_t i = 1; i < sh.size() && i < 4; i++) w += (i > 1 ? ", " : "") + sh[i].name;
                if (sh.size() > 4) w += " and " + std::to_string(sh.size() - 4) + " more";
                w += ".";
            }
        }
        P.para(fo, NsDraw::FS_SMALL, DX0 + 2.5f, y, DW, soft, w);
    }
    P.dl->PopClipRect();
}

extern "C" void sheetview_draw(void)
{
    const SheetModel &s = sheet_model;
    partypanel_focus = -1;
    if (!sheetview_showing()) { if (sheetview_dragging) { sheetview_dragging = 0; V.dragging = false; } return; }
    NsDraw P;
    if (!P.begin()) return;
    partypanel_focus = s.who;
    if (s.who != V.last_who) { V.scroll[TAB_THINGS] = 0; V.drop_row = -1; V.what = 0; V.last_who = s.who; }
    if (V.act_seq != s.seq) V.drop_row = -1;
    int new_what = 0, new_which = -1;
    EFF = effects_now(5);
    VT(P.page());
    VT(header(P, s));
    if (V.tab == TAB_THINGS || V.tab == TAB_SKILLS) {
        VT(doll(P, s));
        if (V.tab == TAB_THINGS) VT(things(P, s, new_what, new_which));
        else VT(skills(P, s, new_what, new_which));
        if (new_what) { V.what = new_what; V.which = new_which; V.seq = s.seq; }
        else if (V.seq != s.seq) V.what = 0;
        else if (!P.over(200, 21, 318, 197)) V.what = 0;
        VT(strip(P, s, 200, 318, V.tab == TAB_THINGS ? 159.5f : 152, 197));
    } else if (V.tab == TAB_SAINTS) VT(saints(P, s));
    else VT(formulae(P, s));
}

/* for the prayer screen (ui/prayview.cpp): the saint's statue, its legend and effect */
const GamePic *ns_saint_statue(int cls) { return statue(cls); }

extern "C" int sheetview_pointed_item(void) { return V.what == 1 ? V.which : -1; }
