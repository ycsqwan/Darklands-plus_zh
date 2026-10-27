/* teachers.c - the teachers a town can offer, and the card options that bring them (teachers.h). */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "../runtime/cpu.h"
#include "../runtime/game.h"
#include "../runtime/gamevars.h"
#include "gamemodel.h"
#include "fixes.h"
#include "teachers.h"


int teach_chance(int s)
{
    int t = (s * 20 / 100) * 33 / 10;                    /* practice_skill: rand(100) <= t */
    t += 1;
    return t < 0 ? 0 : t > 100 ? 100 : t;
}

static void add(TchSource *o, int skill, int fee) { if (o->n < TCH_MAXSK) { o->skill[o->n] = skill; o->fee[o->n] = fee; o->n++; } }

/* the guilds: quality index in shopq, the card's standing test ($MilCr00: seed offset, threshold; the
   artificers' street has none), the source */
static const struct { int src, q, off, need; const char *who, *ask; } GUILDS[5] = {
    { TCH_BLACKSMITHS, 0, 20, 100, "锻造公会", "白天前往锻造公会，选择“请求与公会大师会面”。" },
    { TCH_SWORDSMITHS, 2, 19, 90, "刀匠公会", "白天前往刀匠公会，选择“请求与公会大师会面”。" },
    { TCH_ARMORERS, 3, 18, 95, "盔甲匠公会", "白天前往盔甲匠公会，选择“请求与公会大师会面”。" },
    { TCH_BOWYERS, 5, 17, 85, "弓箭匠公会", "白天前往弓箭匠与枪匠公会，选择“请求与公会大师会面”。" },
    { TCH_ARTIFICERS, 6, 0, 0, "工匠公会", "白天前往工匠公会，选择“请求与公会大师会面”。" },
};
static const char GUILD_COND[] = "公会只接纳在本城拥有足够声望、名望以及{魅力}的队伍。"
                                 "（否则其街道仅通向商店）。当公会因争执而分裂时，其大师们首先希望得到解决这一问题的帮助。";

static int is_here(int loc) { return loc >= 0 && loc == (s16)g_rw(GV_cur_loc); }

/* the guild's standing test for the party now (the town it is in): score - threshold */
static int standing(int k, int seed, int loc)
{
    int fame = (s16)g_rw(GV_party_fame), rep = 0, sum = 0, n = 0;
    u32 lr = g_location(loc);
    if (lr) rep = (s16)rw((u16)(lr >> 16), (u16)((u16)lr + 0x12));
    for (int m = 0; m < 5; m++) if (g_rb((u16)(G_MEMBER(m) + G_M_STATUS))) { sum += (s8)g_rb((u16)(G_MEMBER(m) + G_M_ATTR_CUR + 5)); n++; }
    int score = (int)((u16)(seed + GUILDS[k].off) % 21) + fame / 20 + rep / 2 + sum / (n ? n : 1);
    return score - GUILDS[k].need;
}

static int party_best_skill(int k)
{
    int b = 0;
    for (int m = 0; m < 5; m++) if (g_rb((u16)(G_MEMBER(m) + G_M_STATUS))) { int v = (s8)g_rb((u16)(G_MEMBER(m) + G_M_SKILLS + k)); if (v > b) b = v; }
    return b;
}

/* a started event of kind k at loc (and type0 t, or any with t < 0): its index, -1 none */
static int started(u16 s, u16 o)
{
    int y = (s16)rw(s, (u16)(o + 0x10)), mo = (s16)rw(s, (u16)(o + 0x0e)), d = (s16)rw(s, (u16)(o + 0x0c)), h = (s16)rw(s, (u16)(o + 0x0a));
    int cy = (s16)g_rw(G_CUR_YEAR), cm = (s16)g_rw(G_CUR_MONTH), cd = (s16)g_rw(G_CUR_DAY), ch = (s16)g_rw(G_CUR_HOUR);
    if (y != cy) return y < cy;
    if (mo != cm) return mo < cm;
    if (d != cd) return d < cd;
    return h <= ch;
}
static int event_at(int kind, int loc, int t)
{
    for (int i = 0; i < G_NEVENTS; i++) {
        u32 e = g_event(i); if (!e) continue;
        u16 s = (u16)(e >> 16), o = (u16)e;
        if ((s16)rw(s, (u16)(o + 0x22)) != kind || (s16)rw(s, (u16)(o + 0x1c)) != loc) continue;
        if (t >= 0 && (s16)rw(s, o) != t) continue;
        if (started(s, o)) return i;
    }
    return -1;
}
static int dispute_at(int loc)              /* ev8_at(2, loc): a guild's dispute here (the guild cards' option 2 leads to it) */
{
    for (int i = 0; i < G_NEVENTS; i++) {
        u32 e = g_event(i); if (!e) continue;
        u16 s = (u16)(e >> 16), o = (u16)e;
        if ((s16)rw(s, (u16)(o + 0x22)) == 8 && (s16)rw(s, (u16)(o + 0x28)) == 2 && (s16)rw(s, (u16)(o + 0x1c)) == loc && started(s, o)) return 1;
    }
    return 0;
}

int teach_hired(int loc, TchHired *out, int max)
{
    int n = 0;
    for (int i = 0; i < G_NEVENTS && n < max; i++) {
        u32 e = g_event(i); if (!e) continue;
        u16 s = (u16)(e >> 16), o = (u16)e;
        if ((s16)rw(s, (u16)(o + 0x22)) != 0x28 || (s16)rw(s, (u16)(o + 0x1c)) != loc || !started(s, o)) continue;
        TchHired *h = &out[n++];
        h->skill = (s16)rw(s, o); h->fee = (s16)rw(s, (u16)(o + 0x1e)); h->strength = (s16)rw(s, (u16)(o + 0x20));
        h->day = (s16)rw(s, (u16)(o + 0x14)); h->month = (s16)rw(s, (u16)(o + 0x16)); h->year = (s16)rw(s, (u16)(o + 0x18));
    }
    return n;
}

int teach_town(int loc, const TchTown *t, TchSource *out, int max)
{
    int n = 0, here = is_here(loc);
    u16 seed = (u16)(t->seed + (s16)g_rw(G_PARTY_SEED));
    #define NEXT() (n < max ? memset(&out[n], 0, sizeof out[n]), &out[n++] : (TchSource *)0)
    for (int k = 0; k < 5; k++) {
        int q = t->shopq[GUILDS[k].q];
        if (q <= 0) continue;
        TchSource *o = NEXT(); if (!o) return n;
        o->src = GUILDS[k].src; o->who = GUILDS[k].who; o->ask = GUILDS[k].ask;
        o->cond = GUILDS[k].need ? GUILD_COND : "当公会因争执而分裂时，其大师们首先希望得到解决这一问题的帮助。";
        o->strength = q + (int)(seed % 15) - 5;
        switch (GUILDS[k].src) {
        case TCH_BLACKSMITHS: add(o, SKILL_EDGED, 50); add(o, SKILL_IMPACT, 50); break;
        case TCH_SWORDSMITHS: add(o, SKILL_EDGED, 50); break;
        case TCH_ARMORERS: add(o, SKILL_EDGED, 50); add(o, SKILL_ARTIFICE, 50); break;
        case TCH_BOWYERS: { int w = (int)((u16)((s16)g_rw(G_PARTY_SEED) + loc) % 3); add(o, w == 0 ? SKILL_BOW : w == 1 ? SKILL_THROWN : SKILL_MISSILE, 50); } break;
        default: add(o, SKILL_ARTIFICE, 50); break;
        }
        if (here) {
            if (dispute_at(loc)) snprintf(o->now, sizeof o->now, "此地的公会因争执而分裂，其大师们首先希望得到解决这一问题的帮助。");
            else if (GUILDS[k].need) {
                int d = standing(k, seed, loc);
                snprintf(o->now, sizeof o->now, d >= 0 ? "公会现在会在白天接纳该队伍。" : "公会暂时还不会接纳该队伍（还需要更多的声望、名望或{魅力}）。");
                o->off = d < 0;
            }
        }
    }
    if (t->buildings & 0x100) {
        TchSource *o = NEXT(); if (!o) return n;
        o->src = TCH_UNIVERSITY; o->who = "大学";
        o->ask = "白天前往大学，选择“雇佣一位教师”。";
        o->cond = "他们可能会拒绝（这取决于{感知}、{智力}、{拉丁语}、队伍的最佳{炼金术}以及声望）。一旦被拒绝，他们的大门将关闭一周，并使队伍在此地的声望略微下降。";
        o->strength = (int)(seed % 15) + 45;
        add(o, SKILL_ALCHEMY, 90); add(o, SKILL_RELIGION, 70); add(o, SKILL_LATIN, 50); add(o, SKILL_RW, 50); add(o, SKILL_HEALING, 110);
    }
    if (t->buildings & 0x80) {
        TchSource *o = NEXT(); if (!o) return n;
        o->src = TCH_MONASTERY; o->who = "修道院";
        o->ask = "前往修道院，选择“咨询各学科的辅导事宜”。";
        o->cond = "仅限于具有良好名声（{美德}、{魅力}、此处声望）的队伍，且必须是在本城尚未聘请其他教师的情况下。被拒绝后，请等待两天。";
        o->strength = 50;
        add(o, SKILL_RELIGION, 50); add(o, SKILL_LATIN, 25); add(o, SKILL_RW, 25);
    }
    if (t->physician) {
        TchSource *o = NEXT(); if (!o) return n;
        o->src = TCH_PHYSICIAN; o->who = "医生";
        o->ask = "白天前往医生处，选择“恳求他允许你成为他的学生”。";
        o->cond = "他不收那些已经比自己治愈得更好的人（他自己的{治疗}每周都会变化；亲自拜访才能了解情况）。有些年份他甚至不收任何学生。";
        o->strength = 50;
        add(o, SKILL_HEALING, 60);
        int year = (s16)g_rw(G_CUR_YEAR);
        /* his own skill this week: the game's roll (s050 1838:0000), or its range before a visit */
        int k = (int)(seed % 10), lo = k * (t->size - 3), hi = k * (t->size);
        if (lo < 1) lo = 1; if (hi < 1) hi = 1; if (lo > 99) lo = 99; if (hi > 99) hi = 99;
        int P = -1;
        if (here) { int e = event_at(0x37, loc, -1); if (e >= 0) { u32 p = g_event(e); P = (s16)rw((u16)(p >> 16), (u16)((u16)p + 0x1e)); } }
        int best = party_best_skill(SKILL_HEALING);
        /* (in the game's order: the party healing better first, then the year) */
        if (P > 1 && best > P) { snprintf(o->now, sizeof o->now, "他本周的{治疗:%d}，而队伍中的最佳是%d：他不会教你。", P, best); o->off = 1; }
        else if (P < 0 && best > hi) { snprintf(o->now, sizeof o->now, "他本周的{治疗:%d}最多，而队伍中的最佳是%d：他不会教你。", hi, best); o->off = 1; }
        else if ((s16)(seed + year) % 3 == 0) { snprintf(o->now, sizeof o->now, "他今年不收学生。"); o->off = 1; }
        else if (P >= 0) {
            if (P <= 1) { snprintf(o->now, sizeof o->now, "他目前不收学生。"); o->off = 1; }
            else if (best > P) { snprintf(o->now, sizeof o->now, "他本周的{治疗:%d}，而队伍中的最佳是%d：他不会教你。", P, best); o->off = 1; }
            else snprintf(o->now, sizeof o->now, "他本周的{治疗:%d}：他会教你。", P);
        } else if (best > hi) { snprintf(o->now, sizeof o->now, "他本周的{治疗:%d}最多，而队伍中的最佳是%d：他不会教你。", hi, best); o->off = 1; }
        else if (best > lo) snprintf(o->now, sizeof o->now, "他本周的{治疗:%d至%d}（拜访他时可见具体数值）：只有当数值至少达到队伍最佳的%d时，他才会授课。", lo, hi, best);
    }
    if (t->alchemist) {
        TchSource *o = NEXT(); if (!o) return n;
        o->src = TCH_ALCHEMIST; o->who = "炼金术士";
        o->ask = "白天时，前往占星师巷，选择“接受炼金术的额外指导”。";
        o->cond = "他们可能会拒绝（这取决于{魅力}、{通用语言}、队伍中最佳的{炼金术}以及这里的声望）。被拒绝后，请等待两天半。";
        o->strength = t->size * 3 + (int)((u16)(seed + 4) % 41) + ((t->buildings & 0x100) ? (int)((u16)(seed + 9) % 11) + 10 : 0);
        add(o, SKILL_ALCHEMY, o->strength / 4 + 12);
    }
    #undef NEXT
    /* what is hired here already: a guild, the physician make no second teacher of a skill; the monks
       none while any teacher is hired in the town */
    TchHired h[16]; int nh = teach_hired(loc, h, 16);
    for (int k = 0; k < n; k++) {
        TchSource *o = &out[k];
        if (o->off) continue;
        if (o->src == TCH_MONASTERY && nh) { snprintf(o->now, sizeof o->now, "镇上已聘请了另一位老师，因此僧侣们暂时不再授课。"); o->off = 1; continue; }
        if (o->src == TCH_ARTIFICERS || o->src == TCH_UNIVERSITY || o->src == TCH_ALCHEMIST) continue;
        int all = o->n > 0;
        for (int i = 0; i < o->n; i++) { int got = 0; for (int j = 0; j < nh; j++) if (h[j].skill == o->skill[i]) got = 1; if (!got) all = 0; }
        if (all) { snprintf(o->now, sizeof o->now, "已经聘请：老师现在就在旅店。"); o->off = 1; }
    }
    return n;
}

void teach_town_here(TchTown *t)
{
    memset(t, 0, sizeof *t);
    u16 off = g_rw(GV_city_rec), sg = g_rw((u16)(GV_city_rec + 2));
    if (!sg) return;
    t->size = (s16)rw(sg, (u16)(off + 0x40));
    t->buildings = rw(sg, (u16)(off + 0x5e));
    t->seed = (s16)rw(sg, (u16)(off + 0x56));
    for (int k = 0; k < 9; k++) t->shopq[k] = (s8)rb(sg, (u16)(off + 0x62 + k));
    t->physician = t->alchemist = 1;                     /* (on their own cards: there) */
}

/* the card's option and the source it brings */
static const struct { const char *card; int opt, src; } OPTS[] = {
    { "$Black00", 2, TCH_BLACKSMITHS }, { "$Sword00", 2, TCH_SWORDSMITHS }, { "$Armor00", 2, TCH_ARMORERS },
    { "$Bowye00", 2, TCH_BOWYERS }, { "$Artif00", 2, TCH_ARTIFICERS }, { "$Guild00", 0, TCH_GUILD },
    { "$Unive00", 4, TCH_UNIVERSITY }, { "$CityM00", 1, TCH_MONASTERY }, { "$Physi00", 2, TCH_PHYSICIAN },
    { "$Alche00", 4, TCH_ALCHEMIST },
};

int teach_card_option(const char *card, int opt, TchSource *out)
{
    int src = -1;
    for (size_t k = 0; k < sizeof OPTS / sizeof OPTS[0]; k++)
        if (OPTS[k].opt == opt && !strncasecmp(card, OPTS[k].card, 8)) { src = OPTS[k].src; break; }
    if (src < 0) return 0;
    int loc = (s16)g_rw(GV_cur_loc);
    TchTown t; teach_town_here(&t);
    if (src == TCH_GUILD) {                              /* $Guild00 (sub_s070_19af_0394): by the guild (D_e3d0) */
        int bit = (s16)g_rw(GV_party_x);
        u16 seed = (u16)(t.seed + (s16)g_rw(G_PARTY_SEED));
        memset(out, 0, sizeof *out);
        out->src = TCH_GUILD; out->who = "公会的师傅们";
        out->ask = "在这里：选择“询问特殊训练”。";
        out->cond = "他们可能会拒绝（这取决于{魅力}、这里的声望和名气）；如果公会欠队伍人情，他们总是同意。";
        int q = bit == 2 || bit == 0x10 ? t.shopq[0] : bit == 4 ? t.shopq[2] : bit == 8 ? t.shopq[5] : t.shopq[6];
        out->strength = q + (int)(seed % 15) - 5;
        if (bit == 8) snprintf(out->now, sizeof out->now, "原版游戏中的一个漏洞：这个公会的老师根本不教授任何技能。");
        else if (bit == 2 || bit == 0x10 || bit == 4) { add(out, SKILL_EDGED, 50); add(out, SKILL_IMPACT, 50); }
        else add(out, SKILL_ARTIFICE, 50);
        return 1;
    }
    TchSource all[12];
    int n = teach_town(loc, &t, all, 12);
    for (int k = 0; k < n; k++) if (all[k].src == src) { *out = all[k]; return 1; }
    return 0;
}

/* the teacher of a skill here who teaches best (the highest ability; the lower fee between equals):
   the one the inn uses with the "bestteacher" fix (several hired for one skill), else the first */
int teach_best_here(int skill)
{
    int loc = (s16)g_rw(GV_cur_loc), best = -1, bs = -1, bf = 0;
    if (!fix_is_on("bestteacher")) return event_at(0x28, loc, skill);
    for (int i = 0; i < G_NEVENTS; i++) {
        u32 e = g_event(i); if (!e) continue;
        u16 s = (u16)(e >> 16), o = (u16)e;
        if ((s16)rw(s, (u16)(o + 0x22)) != 0x28 || (s16)rw(s, (u16)(o + 0x1c)) != loc || (s16)rw(s, o) != skill || !started(s, o)) continue;
        int a = (s16)rw(s, (u16)(o + 0x20)), f = (s16)rw(s, (u16)(o + 0x1e));
        if (best < 0 || a > bs || (a == bs && f < bf)) { best = i; bs = a; bf = f; }
    }
    return best;
}
int teach_ability_here(int skill)
{
    int i = teach_best_here(skill);
    if (i < 0) return -1;
    u32 e = g_event(i);
    return (s16)rw((u16)(e >> 16), (u16)((u16)e + 0x20));
}

int teach_capped(int m, int skill, int a)
{
    if (a < 0 || m < 0 || m > 4 || skill < 0 || skill >= 19 || !fix_is_on("teachcap")) return 0;
    return (s8)g_rb((u16)(G_MEMBER(m) + G_M_SKILLS + skill)) >= a;
}
