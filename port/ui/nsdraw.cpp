/* nsdraw.cpp - see nsdraw.h */
#include "../runtime/config.h"
#include "nsdraw.h"
#include "../runtime/cpu.h"
#include "../runtime/game.h"
#include <set>
#include <vector>
#include <algorithm>
extern "C" {
#include "../mods/gamemodel.h"
}

std::string ns_quality_words(int id, int qual)
{
    const ItemDef *d = ns_itemdef(id);
    if (!d || !ns_shows_quality(id, qual)) return "";        /* (as the lists: a potion's dots tell its grade; gear's quality only its price) */
    return ", " + std::to_string(qual) + "q";
}

extern "C" const char *gm_skill_short(int k);
std::string ns_item_line(int id, int q)
{
    const ItemDef *d = ns_itemdef(id);
    if (!d) return "";
    if (d->type >= 95 && d->type <= 97) {                  /* a shield: its defence, by size and quality (mods/gamemodel.h) */
        if (q < 0) q = d->quality;
        return "防御" + std::to_string(gm_shield_defence(d->type, q, 0)) + "抵抗打击，" + std::to_string(gm_shield_defence(d->type, q, 1)) +
               "抵抗射击与投掷；双手武器无法使用";
    }
    if (d->type >= 0 && d->type <= 33 && (d->kind == K_WEAPON || d->kind == K_MISSILE)) {
        int w = d->type, sk = rb(G_FAR20A5, (u16)(0x75ac + w)), cp = rb(G_FAR20A5, (u16)(0x76a8 + w)), dmg = rb(G_FAR20A5, (u16)(0x76e7 + w));
        int tm = rb(G_FAR20A5, (u16)(0x7669 + w)), rl = rb(G_FAR20A5, (u16)(0x77e3 + w));
        std::string r = (sk < 7 ? "{" + std::string(gm_skill_short(sk)) + "}" : std::string("?")) + ", " + ((cp >> 4) >= 2 ? "双手" : "一手") + "，伤害" + std::to_string(dmg) + "，穿透" + std::to_string(cp & 15);
        if (tm) r += "，攻击所需时间" + std::to_string(tm);
        if (rl) r += std::string("，准备与释放所需时间") + (w >= 30 ? "reload " : w >= 27 ? "覆盖身体与手臂" : "throw ") + std::to_string(rl);
        return r;
    }
    if (d->type >= 67 && d->type <= 84) {
        static const char *HN[6] = { "none", "soft", "stiff", "scale", "mail", "plate" };
        int h = rb(G_FAR20A5, (u16)(0x781e + d->type));
        return std::string(d->type <= 75 ? "，硬度" : "覆盖腿部") + "制作更佳" + std::to_string(h) + " (" + (h >= 0 && h < 6 ? HN[h] : "?") + ")";
    }
    return "";
}

/* a weapon or an armour against what member m has in that place now (the equipment slots of the
 * member record: weapon +0x3d/+0x44, missile +0x0e/+0x46, body +0x37/+0x3b, legs +0x38/+0x3c,
 * shield +0x48/+0x47: type and quality); "" for other things */
std::string ns_item_compare(int id, int q, int m)
{
    const ItemDef *d = ns_itemdef(id);
    if (!d || m < 0 || m >= 5 || !gm_member_present(m)) return "";
    char nm[24]; gm_member_name(m, nm, sizeof nm);
    std::string who = ns_gtext(nm);
    u16 b = G_MEMBER(m);
    auto slot = [&](int toff, int qoff, int &t, int &wq) { t = (s8)g_rb((u16)(b + toff)); wq = g_rb((u16)(b + qoff)); return t >= 0; };
    auto name_of = [&](int t, int wq) {                     /* the record in use of that type */
        GmItem it;
        for (int i = 0; i < gm_item_count(m); i++)
            if (gm_item(m, i, &it) && it.type == t && it.qual == wq) { const ItemDef *w = ns_itemdef(it.id); return w ? ns_gtext(w->plain.c_str()) : ns_item_name(it.id); }   /* (armour: "Scale") */
        return std::string("?");
    };
    auto cmpq = [&](int a, int c) { return a > c ? "制作较差" : a < c ? "制作尚可" : "制作良好"; };
    static const char *HN[6] = { "none", "soft", "stiff", "scale", "mail", "plate" };
    if (d->type >= 67 && d->type <= 84) {
        bool vital = d->type <= 75;
        int t, wq, h = rb(G_FAR20A5, (u16)(0x781e + d->type));
        if (!slot(vital ? 0x37 : 0x38, vital ? 0x3b : 0x3c, t, wq) || t < 67 || t > 84) return who + "在那里不穿任何装备。";
        int wh = rb(G_FAR20A5, (u16)(0x781e + t));
        std::string n = name_of(t, wq);
        int s1 = (h > wh) - (h < wh), s2 = (q > wq) - (q < wq);
        return who + " wears " + n + " (" + (wh >= 0 && wh < 6 ? HN[wh] : "?") + ", " + std::to_string(wq) + "q): 这是" +
               (s1 > 0 ? "harder" : s1 < 0 ? "softer" : "如此坚硬") + (s1 == s2 ? " and " : " but ") + cmpq(q, wq) + ".";
    }
    if (d->kind == K_SHIELD) {
        int t, wq;
        if (!slot(0x48, 0x47, t, wq)) return who + "不携带盾牌。";
        return who + " carries " + name_of(t, wq) + " (" + std::to_string(wq) + "q): 这是" + cmpq(q, wq) + ".";
    }
    if (d->type >= 0 && d->type <= 62 && (d->kind == K_WEAPON || d->kind == K_MISSILE)) {
        int w = d->type, sk = rb(G_FAR20A5, (u16)(0x75ac + w)), smin = rb(G_FAR20A5, (u16)(0x7765 + w));
        bool missile = sk == 5 || sk == 6;
        std::string r = who + "'s " + (sk < 7 ? "{" + std::string(gm_skill_short(sk)) + ":" + std::to_string(gm_member_skill(m, sk)) + "}" : std::string("技能：？")) + ".";
        int str, strmax; gm_member_attr(m, 1, &str, &strmax);
        if (smin > 0 && strmax < smin) r += "太重：需要 {力量：" + std::to_string(smin) + "}.";
        int t, wq;
        if (slot(missile ? 0x0e : 0x3d, missile ? 0x46 : 0x44, t, wq) && t <= 62) {
            int cp = rb(G_FAR20A5, (u16)(0x76a8 + t)), dmg = rb(G_FAR20A5, (u16)(0x76e7 + t));
            r += "现在手中持有：" + name_of(t, wq) + " (damage " + std::to_string(dmg) + "，穿透" + std::to_string(cp & 15) + ", " + std::to_string(wq) + "q).";
        } else r += missile ? "未装备远程武器。" : "现在手中空无一物。";
        return r;
    }
    return "";
}

/* a weapon or an armour against the whole party (the shared pack: whom it would suit) */
std::string ns_item_compare_party(int id, int q)
{
    const ItemDef *d = ns_itemdef(id);
    if (!d) return "";
    std::vector<int> ms;
    for (int m = 0; m < gm_party_size() && m < 5; m++) if (gm_member_present(m) && gm_member_status(m) != 3) ms.push_back(m);
    auto nm = [](int m) { char b[24]; gm_member_name(m, b, sizeof b); return ns_gtext(b); };
    auto join = [](const std::vector<std::string> &v) {
        std::string r;
        for (size_t i = 0; i < v.size(); i++) r += (i == 0 ? "" : i + 1 == v.size() ? " and " : ", ") + v[i];
        return r;
    };
    if (d->type >= 67 && d->type <= 84) {
        bool vital = d->type <= 75;
        int h = rb(G_FAR20A5, (u16)(0x781e + d->type));
        std::vector<std::string> better, none;
        for (int m : ms) {
            u16 b = G_MEMBER(m);
            int t = (s8)g_rb((u16)(b + (vital ? 0x37 : 0x38))), wq = g_rb((u16)(b + (vital ? 0x3b : 0x3c)));
            if (t < 67 || t > 84) { none.push_back(nm(m)); continue; }
            int wh = rb(G_FAR20A5, (u16)(0x781e + t));
            if (h > wh || (h == wh && q > wq)) better.push_back(nm(m));
        }
        std::string r;
        if (!better.empty()) r = "比……更坚硬或制作更好" + join(better) + (better.size() == 1 ? " wears" : " wear") + " there.";
        if (!none.empty()) r += (r.empty() ? "" : " ") + join(none) + (none.size() == 1 ? " wears" : " wear") + "在那里什么都没有。";
        return r.empty() ? "每个人在那里都穿着同样好或更好的装备。" : r;
    }
    if (d->kind == K_SHIELD) {
        std::vector<std::string> better, none;
        for (int m : ms) {
            u16 b = G_MEMBER(m);
            int t = (s8)g_rb((u16)(b + 0x48)), wq = g_rb((u16)(b + 0x47));
            if (t < 0) none.push_back(nm(m)); else if (q > wq) better.push_back(nm(m));
        }
        std::string r;
        if (!better.empty()) r = "比……的盾牌制作更好" + join(better) + ".";
        if (!none.empty()) r += (r.empty() ? "" : " ") + join(none) + (none.size() == 1 ? " carries" : " carry") + "没有盾牌。";
        return r.empty() ? "每个人的盾牌都同样好或更好。" : r;
    }
    if (d->type >= 0 && d->type <= 62 && (d->kind == K_WEAPON || d->kind == K_MISSILE)) {
        int w = d->type, sk = rb(G_FAR20A5, (u16)(0x75ac + w)), smin = rb(G_FAR20A5, (u16)(0x7765 + w));
        if (sk >= 7) return "";
        std::vector<std::pair<int, std::string>> v; std::vector<std::string> heavy;
        for (int m : ms) {
            v.push_back({ gm_member_skill(m, sk), nm(m) });
            int str, strmax; gm_member_attr(m, 1, &str, &strmax);
            if (smin > 0 && strmax < smin) heavy.push_back(nm(m));
        }
        std::sort(v.begin(), v.end(), [](const std::pair<int, std::string> &a, const std::pair<int, std::string> &b) { return a.first > b.first; });
        std::string r = "{" + std::string(gm_skill_short(sk)) + "}: ";
        for (size_t i = 0; i < v.size(); i++) r += (i ? ", " : "") + v[i].second + " " + std::to_string(v[i].first);
        r += ".";
        if (!heavy.empty()) r += "对……来说太重了" + join(heavy) + "（需要 {力量：" + std::to_string(smin) + "}).";
        return r;
    }
    return "";
}

extern "C" int fix_is_on(const char *id);
#include "../mods/fixes.h"
std::string ns_potion_effect(int id);
/* how much the encounters ask for a thing (a potion: the card scripts that list it among an
   option's potions, card_items, counted from the game's code) or a saint (mods/saintinfo.h):
   the line, and its colour - many green, some blue, a few brown, none grey */
ImU32 ns_tier_colour(int n) { return n >= 20 ? IM_COL32(34, 110, 30, 255) : n >= 5 ? IM_COL32(36, 78, 150, 255) : n >= 1 ? IM_COL32(150, 92, 30, 255) : IM_COL32(140, 124, 104, 255); }
std::string ns_item_encounters(int id, ImU32 *col)
{
    if (id < 95 || id > 116) return "";
    static const unsigned char CARDS[22] = { 61, 42, 54, 5, 15, 2, 73, 41, 11, 7, 0, 1, 0, 0, 1, 6, 1, 1, 0, 0, 1, 6 };
    int n = CARDS[id - 95];
    if (col) *col = ns_tier_colour(n);
    return n >= 20 ? "许多遭遇战允许你使用它。" : n >= 5 ? "有些遭遇战允许你使用它。" : n >= 1 ? "少数遭遇战允许你使用它。" : "没有任何遭遇战要求它。";
}
/* what a thing is good for, where that is not plain from its kind (ingredients: the potions made
 * with it; the general goods: what in the game asks for them). Read from the game's code: of the
 * general goods only rope and lockpicks are asked for by cards (count_items, s029 0e76:0c76: the
 * party's, so one is enough), and an instrument by St. Cecilia (the type flag 0x20000000, s039
 * 165c:1024); nothing asks for a torch, candle, lantern, iron spike, grappling hook or clock. */
std::string ns_item_use(int id)
{
    const ItemDef *d = ns_itemdef(id);
    if (!d) return "";
    if (d->kind == K_INGREDIENT) return "";                 /* (the potions it goes into are pictures: ns_ingredient_row) */
    if (id >= 95 && id <= 116) return ns_potion_effect(id);
    switch (id) {
    case 0x3b: return "用于攀爬：在夜间翻越城镇的城墙、进入城堡等场合，最擅长攀爬的队员会率先上去，再把绳索放下来供其他人使用（如果没有绳索，每个人只能独自攀爬）。队伍中有一人即可。";
    case 0x40: return "用于开锁：夜间店铺、房门、箱子、牢房等。队伍中有一人即可。";
    case 0x41: case 0x42: return "音乐守护圣人圣塞西莉亚，会让携带乐器的队员在城镇中的声望大幅提升。";
    case 0x39: case 0x3a: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
        return "游戏中没有任何东西需要它（既不要卡片，也不要圣人）：它的价值仅取决于出售价格。";
    /* the quest things and trophies (read from the cards' scripts: add_item, count_items, take_item) */
    case 0x8c: case 0x8d: case 0x8e: case 0x8f: case 0x90: case 0x91:
        return "六把钥匙之一：巴佛墨特巢穴最后一间密室的入口，只有携带全部六把钥匙的队伍才能打开。请妥善保管。";
    case 0x92: return "一件战利品：野猪獠牙，被认为能带来好运。游戏中没有任何东西要求它，也没有商店收购。";
    case 0x93: return "一件战利品。游戏中没有任何东西要求它。";
    case 0x94: return "墓地旁的一具骷髅想要找回自己的骨头：把它交给骷髅，它就会与你交谈。";
    case 0x95: return "从盗贼手中夺回的龙之金杯：在龙面前挥舞它，龙就不会对其宝藏喷火。";
    }
    if (id >= 129 && id <= 139) return "被派你去取回的人所追寻：把它送还给他们（世界地图上的任务，M标记处会注明是谁以及在哪里）。";
    return "";
}

/* an ingredient: the potions it goes into, as pictures (each potion has three formulae, of grade 1,
   2 and 3, the dots on its flask; under each picture three pips, one a grade: filled where that
   grade's formula takes this ingredient, gold where someone in the party knows that formula), and
   under them a line about the one under the pointer; from x0 to x1 at y, the height it takes */
float ns_ingredient_row(NsDraw &P, float x0, float y, float x1, int id)
{
    const ImU32 ink = NsDraw::INK, soft = NsDraw::INK_SOFT;
    struct Use { int potion; int need[3]; bool known[3]; };
    std::vector<Use> uses;
    for (int f = 0; f < (int)gamedata.formulas.size(); f++) {
        int need = 0;
        for (const FormulaIngr &g : gamedata.formulas[f].ingr) if (g.item == id) need += g.count;
        if (!need) continue;
        int potion = 95 + f / 3; size_t k = 0;
        while (k < uses.size() && uses[k].potion != potion) k++;
        if (k == uses.size()) uses.push_back({ potion, { 0, 0, 0 }, { false, false, false } });
        uses[k].need[f % 3] = need;
        for (int m = 0; m < gm_party_size() && m < 5; m++) if (gm_member_present(m) && gm_knows_formula(m, f)) uses[k].known[f % 3] = true;
    }
    if (uses.empty()) { P.text(P.fo, NsDraw::FS_BODY, x0, y, soft, "没有配方使用它。"); return 7; }
    const char *lab = "进入";
    P.text(P.fo, NsDraw::FS_SMALL, x0, y + 3.0f, soft, lab);
    float x = x0 + P.width(P.fo, NsDraw::FS_SMALL, lab) + 3, sz = 9.0f;
    float step = fminf(11.0f, (x1 - x) / (float)uses.size());
    int hov = -1;
    for (size_t k = 0; k < uses.size(); k++) {
        const Use &u = uses[k];
        float cx = x + k * step + step * 0.5f, cy = y + sz * 0.5f;
        bool any_known = u.known[0] || u.known[1] || u.known[2];
        P.icon(cx, cy, sz, u.potion);
        if (!any_known) P.rect(cx - sz * 0.55f, cy - sz * 0.62f, cx + sz * 0.55f, cy + sz * 0.62f, IM_COL32(230, 214, 178, 185));   /* (no formula with it known: washed out) */
        for (int g = 0; g < 3; g++) {                     /* the grades' pips */
            ImVec2 pc = NsDraw::w(cx + (g - 1) * 2.4f, cy + sz * 0.62f + 1.6f);
            float r = 0.85f * P.k;
            if (u.need[g]) P.dl->AddCircleFilled(pc, r, u.known[g] ? IM_COL32(196, 146, 30, 255) : IM_COL32(70, 48, 26, 230));
            else P.dl->AddCircle(pc, r, IM_COL32(120, 96, 66, 150), 0, 0.25f * P.k);
        }
        if (P.over(cx - step * 0.5f, cy - sz * 0.6f, cx + step * 0.5f, cy + sz * 0.6f + 3.5f)) hov = (int)k;
    }
    std::string line;
    if (hov >= 0) {                                         /* the potion's grades that take it, how many a batch, which are known */
        const Use &u = uses[hov];
        line = ns_item_name(u.potion) + ": ";
        bool first = true;
        for (int g = 0; g < 3; g++) if (u.need[g]) {
            char b[64]; snprintf(b, sizeof b, "%s等级 %d (%d 个配方%s)", first ? "" : ", ", g + 1, u.need[g], u.known[g] ? "，已知" : "");
            line += b; first = false;
        }
        int other = 0; for (int g = 0; g < 3; g++) if (!u.need[g]) other++;
        if (other) line += other == 2 ? "；另外两个等级不使用它" : "；另一个等级也不使用它";
    } else {
        int nk = 0; for (const Use &u : uses) nk += u.known[0] || u.known[1] || u.known[2];
        line = "点击：那些配方使用它的等级（1至3级），以及队伍已知的黄金所在位置。点击药剂可查看更多信息。";
        if (!nk) line = "队伍中无人知晓含有它的配方。点击：那些使用它的等级（1至3级）；点击药剂可查看更多信息。";
    }
    return sz + 4.2f + P.para(P.fo, NsDraw::FS_SMALL, x0, y + sz + 4.2f, x1 - x0, hov >= 0 ? ink : NsDraw::INK_DIM, line);
}

/* what a potion does, as the game's code does it (use_potion s031 1462:43b2; thrown: effect_spell
 * s019; hit_damage s014 18bf), the grades 1/2/3 being the qualities 25/35/45 of a brew. The weapon
 * oils are the member's, not a weapon's: the game keeps them in one byte of the member (+0x34:
 * damage and penetration) that every hit the member lands uses, whatever the weapon (bows and
 * fists too), whatever the potion's label says. */
std::string ns_potion_effect(int id)
{
    bool fixed = fix_is_on("potions") != 0;
    std::string day = fixed ? "可持续约一天（19至21小时）。" : "永久生效，并与下一次剂量叠加。";
    switch (id) {
    case 95: return "在战斗中投掷：一团恶臭的烟雾（最大范围可达4乘4格）。被击中的目标将失去一半的{敏捷}、{感知}和武器技能，而烟雾中的所有人——无论敌我——都将损失四分之一，持续时间随等级提高而延长。";
    case 96: return "投掷：致盲粉末。目标将失去三分之一的{敏捷}、{感知}和武器技能，并陷入失明状态（无法射击、投掷或良好执行指令）；其身旁的人也将损失四分之一。高等级时持续时间更长。";
    case 97: return "投掷：一团黑烟。穿透任何烟雾的命中率都会大幅降低（仅为原来的五分之一），而在烟雾内部作战则需额外付出15点命中代价。高等级时持续时间更长。（战斗菜单从不向队伍提供此技能：它只用于卡片，且由敌人投掷。）";
    case 98: return "投掷：瘙痒粉末。武器技能会因身体及四肢上穿戴的护甲重量而下降五倍：护甲越重，瘙痒越严重。高等级时持续时间更长。";
    case 99: return "投掷：腐蚀性酸液，会侵蚀目标的护甲，使其质量降低4、6或8级（对应1、2、3级），并随机额外降低最多8级；当护甲被完全侵蚀后，目标将被摧毁，若无护甲（或仍有损伤），还会对目标造成伤害。同时还能溶解锁具。";
    case 100: return "投掷：毒雾（最大范围可达2乘2格），其伤害不受护甲阻挡，对较弱目标尤为致命；高等级时效果更强。";
    case 101: return "投掷：耀眼的闪光。目标以及该楼层内所有面向它的投掷者之敌都将暂时失明（高等级时持续时间更长）；投掷者的盟友则免受影响。";
    case 102: return "投掷：以巨大威力爆发：伤害为4、6或8级（对应1、2、3级），并随机额外增加最多8级，对目标的穿透力为6、7或8级，周边区域则相应减弱；同时还会使护甲因伤害而损耗四分之一。";
    case 103: return "投掷：燃烧的焦油，护甲无法阻挡：伤害为4、6或8级（对应1、2、3级），并随机额外增加最多6级，远处的伤害减半，护甲质量（以及防火墙保护）亦随之降低四分之一；同时还会进一步磨损护甲。";
    case 104: return "投掷：一团黏稠的泥浆（最大范围可达4乘4格），会使穿越者速度降至平时的1/2、3/8或1/4（对应1、2、3级），且持续时间较长。";
    case 105: if (fix_is_on("oilweapons")) return std::string("由队员使用（库存栏中P标记）：每次使用任何非撞击类武器或连枷的攻击，均可获得+3、+4或+5的伤害（对应1、2、3级）。仅计算伤害，不计命中几率；") + day;
              return "由队员使用（库存栏中P标记）：每次使用任何武器——弓箭、枪械、撞击类武器乃至拳头——均可获得+3、+4或+5的伤害（对应1、2、3级）。仅计算伤害，不计命中几率；" + day;
    case 106: if (fix_is_on("oilweapons")) return "由队员使用：每次使用刀剑或长柄武器的攻击，均可获得+3、+4或+5的穿透力（对应1、2、3级），" + std::string(fixed ? "总计最高可达15" : "超过15后，穿透力将重新从0开始计算") + ";" + day;
              return "由队员使用：每次使用任何武器的攻击，均可获得+3、+4或+5的穿透力（对应1、2、3级），" + std::string(fixed ? "总计最高可达15" : "过了15点后，穿透力会重新从0开始（弓的6+10次命中算作0）") + ";" + day;
    case 107: if (fix_is_on("greatpower")) return std::string("队伍成员使用时：手中的武器品质提升14、22或30点（等级1、2、3；最高99），") + (fix_is_on("oilweapons") ? "如果是冲击武器、链枷或手枪" : "包括弓、枪和拳头") + "，持续约一天（19至21小时）。每提升10点品质，伤害就增加一级。第二次服用会叠加第一次的效果。";
              return "队伍成员每次命中时，无论使用何种武器：等级1增加14点穿透力；等级2增加1点伤害和6点穿透力；等级3增加1点伤害和14点穿透力。" + std::string(fixed ? "穿透力在所有情况下最多累计到15点" : "超过15点后穿透力会重新从0开始，因此等级1和3会降低弓的穿透力") + ";" + day;
    case 108: return "队伍成员使用时：对弓、投掷武器和投射装置，品质提升14、22或30点（等级1、2、3；最高99），持续约一天（19至21小时）。第二次服用会叠加第一次的效果。";
    case 109: if (fix_is_on("armorday")) return "队伍成员使用时：其护甲品质提升20、25或30点，厚度增加1、2或2级（等级1、2、3），能抵御每一次打击和射击，持续约一天（19至21小时）。在效果生效期间再次服用，可保持更强的效果。";
              return "队伍成员使用时：对其护甲的每一次击打、打击或射击，永久增加1、2或3点防护（等级1、2、3）；剂量可叠加。";
    case 110: return "喝下它只能得到一枚弗罗林：它专门用于卡牌，当遭遇需要时（用于净化或炼金术）。";
    case 111: case 112: case 113: case 114: {
        static const char *const ATT[4] = { "{Perception}", "{Endurance}", "{Strength}", "{Agility}" };
        static const char *const AMT[4] = { "9-14、15-21或22-29", "6-11、8-13或10-15", "6-11、8-13或10-15", "6-11、8-15或10-17" };
        int a = id - 111;
        std::string t = std::string("Drunk: ") + ATT[a] + "提升" + AMT[a] + "（等级1、2、3）持续约一天：当前数值会上升，必要时甚至超过最大值；但最大值本身不会改变。";
        if (a == 1 || a == 2) t += std::string("对受伤的队员有治愈作用：") + (fix_is_on("blessing") ? "效果消退时，只会失去仍高于最大值的部分，因此治愈效果得以保留。" : "但当效果完全消退时，全部数值都会被收回，无论是否已治愈。");
        else t += "效果消退时，所增加的部分将不复存在。";
        return t + "第二次服用在第一次效果仍在时，只会叠加超出部分。";
    }
    case 115: return "喝下后：永久治愈因伤损失的{力量}的50%、70%或90%（等级1、2、3），并使{耐力}提升9-14、15-21或22-29，持续约30小时。";
    case 116: {
        if (fix_is_on("armorday")) return "喝下后：约一天（19至21小时）的防火保护：阿拉伯之火造成的伤害减少5、7或9点（等级1、2、3）。第二次服用可保持更强的效果。";
        return std::string("喝下后：永久防火保护：阿拉伯之火造成的伤害减少5、7或9点（等级1、2、3）。") + (fix_is_on("fireward") ? "" : "同时还永久为护甲增加16点防护。"); }
    }
    return "";
}

float ns_item_strip(NsDraw &P, float y0, const NsStripThing &t)
{
    const ItemDef *d = ns_itemdef(t.id);
    const ImU32 ink = NsDraw::INK, soft = NsDraw::INK_SOFT;
    P.icon(71, y0 + 8, 11.0f, t.id, t.qual);
    float rw = t.right.empty() ? 0 : P.width(P.fo, NsDraw::FS_BODY, t.right) + 6;
    P.text(P.f, NsDraw::FS_HEAD, 79, y0 + 1.5f, ink, ns_item_name(t.id), -1, 314 - rw);
    if (!t.right.empty()) P.text(P.fo, NsDraw::FS_BODY, 0, y0 + 1.8f, t.right_col, t.right, 314);
    /* (the weight only of what can be worn or wielded: the load is the equipped things' alone,
       equip_weight s031 18e7:140a; what is merely carried weighs nothing) */
    bool wearable = d && (d->kind == K_WEAPON || d->kind == K_MISSILE || d->kind == K_ARMOR || d->kind == K_SHIELD);
    std::string l1 = std::string(d ? kind_name_one(d->kind) : "") + ns_quality_words(t.id, t.qual) + (wearable ? "，重量" + std::to_string(t.weight) + "装备时" : std::string());
    if (t.count > 1) l1 += " each, " + std::to_string(t.count) + "其中";
    if (!t.status.empty()) l1 += "; " + t.status;
    float y = y0 + 10;
    P.text(P.fo, NsDraw::FS_BODY, 79, y, soft, l1); y += 7;
    std::string l2 = ns_item_line(t.id, t.qual);
    if (!l2.empty()) { P.text(P.fo, NsDraw::FS_BODY, 79, y, soft, l2); y += 7; }
    if (d && d->kind == K_INGREDIENT) return y + ns_ingredient_row(P, 79, y, 314, t.id);   /* an ingredient: the potions it goes into, as pictures */
    std::string use = ns_item_use(t.id);                    /* what it is good for (rope, lockpicks ...) */
    if (!use.empty()) {
        y += P.para(P.fo, NsDraw::FS_BODY, 79, y, 235, soft, use);
        ImU32 ec; std::string enc = ns_item_encounters(t.id, &ec);
        if (!enc.empty() && (t.y_max <= 0 || y + 0.5f + P.para_height(P.fo, NsDraw::FS_BODY, 235, enc) <= t.y_max))   /* (no room: the use says enough) */
            y += 0.5f + P.para(P.fo, NsDraw::FS_BODY, 79, y + 0.5f, 235, ec, enc);
    }
    else if (t.cmp_who >= 0 || t.cmp_who == -2) {          /* a weapon or an armour: against what the member (-2: the party) has now */
        std::string cmp = t.cmp_who == -2 ? ns_item_compare_party(t.id, t.qual) : ns_item_compare(t.id, t.qual, t.cmp_who);
        if (!cmp.empty()) y += P.para(P.fo, NsDraw::FS_BODY, 79, y, 235, ink, cmp) + 0.5f;
        /* the member's load were it worn instead of what is in that slot (ns_load_with), and the band
           it comes to (sheetview's bands and what they do: ui_load_effect) */
        int m = t.cmp_who, after = wearable && m >= 0 ? ns_load_with(t.id, m) : -1;
        if (after >= 0 && after != gm_member_load(m)) {
            int e, st, mx; gm_member_attr(m, 0, &e, &mx); gm_member_attr(m, 1, &st, &mx);
            int cap = e + st, cur = gm_member_load(m);
            auto band = [&](int l) { return 2 * l <= cap ? 0 : l <= cap ? 1 : 2 * l <= 3 * cap ? 2 : 3; };
            static const char *BAND[4] = { "轻负荷：无阻碍", "正常负荷：徒手格斗速度慢五分之一",
                                           "重负荷：格斗速度再慢五分之一，闪避受阻", "超负荷：格斗速度慢三分之二，几乎无法闪避" };
            int b0 = band(cur), b1 = band(after);
            char nm[24]; gm_member_name(m, nm, sizeof nm);
            char line[200];
            snprintf(line, sizeof line, "%s穿着：负载%d » %d / %d，%s%s", ns_gtext(nm).c_str(), cur, after, cap, BAND[b1],
                     b1 > b0 ? "（现在 " : b1 < b0 ? "（现在 " : "");
            std::string l = line;
            if (b1 != b0) { static const char *SHORT[4] = { "轻", "正常", "重", "超载" }; l += SHORT[b0]; l += "）"; }
            ImU32 c = b1 >= 3 ? NsDraw::RED : b1 == 2 ? IM_COL32(150, 80, 20, 255) : after < cur ? NsDraw::GREEN : soft;   /* (as the heading's note: ns_load_note) */
            y += P.para(P.fo, NsDraw::FS_BODY, 79, y, 235, c, l);
        }
    }
    return y;
}

std::string ns_quality_tag(int id, int qual)
{
    char b[24] = "";
    const ItemDef *d = ns_itemdef(id);
    if (ns_shows_quality(id, qual)) snprintf(b, sizeof b, "%dq", qual);
    else if (d && d->kind == K_POTION && config_get_int("display.potion_grade", 4) == 2) {
        int ui_potion_tier(int q, int base);
        int t = ui_potion_tier(qual, d->quality);
        if (t > 0) snprintf(b, sizeof b, "等级%d", t);
    }
    return b;
}

/* the place a thing is worn or held: 1 body armour, 2 the limbs' armour, 3 shield, 4 the weapon in
   hand, 5 the missile weapon; 0 none */
int ns_slot_of(int id)
{
    const ItemDef *d = ns_itemdef(id);
    if (!d) return 0;
    if (d->type >= 67 && d->type <= 84) return d->type <= 75 ? 1 : 2;
    if (d->kind == K_SHIELD) return 3;
    if (d->type >= 0 && d->type <= 62 && (d->kind == K_WEAPON || d->kind == K_MISSILE)) {
        int sk = rb(G_FAR20A5, (u16)(0x75ac + d->type));
        return sk == 4 || sk == 5 || sk == 6 ? 5 : 4;   /* (thrown weapons are readied as the missile weapon) */
    }
    return 0;
}
/* a ware against what member m has there now: 1 better, -1 worse, 0 alike or not to be weighed
   (another kind of weapon or shield), 2 nothing there. Armour: harder first, then better made
   (as ns_item_compare says it); a weapon or shield: better made, of the same kind */
/* member m's load were thing id worn in its slot instead of what is there (only equipped things
   weigh: equip_weight); -1 when it is not worn */
int ns_load_with(int id, int m)
{
    int sl = ns_slot_of(id);
    if (!sl || m < 0 || m >= 5 || !gm_member_present(m)) return -1;
    static const u16 TOFF[6] = { 0, 0x37, 0x38, 0x48, 0x3d, 0x0e };
    int t = (s8)g_rb((u16)(G_MEMBER(m) + TOFF[sl])), old = 0;
    if (t >= 0) for (int k = 0; k < (int)gamedata.items.size(); k++) if (gamedata.items[k].type == t) { old = gm_itemdef_weight(k); break; }
    int v = gm_member_load(m) - old + gm_itemdef_weight(id);
    return v < 0 ? 0 : v;
}
/* "Load 42 of 76", or with a thing pointed at "Load 42 > 47 of 76", right-aligned to x1 on a heading
   line at y, coloured by the band it comes to; its width (the heading's rule stops short of it) */
float ns_load_note(NsDraw &P, float x1, float y, int m, int after)
{
    if (m < 0 || m >= 5 || !gm_member_present(m)) return 0;
    int e, s, mx; gm_member_attr(m, 0, &e, &mx); gm_member_attr(m, 1, &s, &mx);
    int cap = e + s, cur = gm_member_load(m), v = after >= 0 ? after : cur;
    auto band = [&](int l) { return 2 * l <= cap ? 0 : l <= cap ? 1 : 2 * l <= 3 * cap ? 2 : 3; };
    int b = band(v);
    ImU32 c = b >= 3 ? NsDraw::RED : b == 2 ? IM_COL32(150, 80, 20, 255) : NsDraw::INK_SOFT;
    char t[64];
    if (after >= 0 && after != cur) snprintf(t, sizeof t, "负载%d » %d / %d", cur, after, cap);
    else snprintf(t, sizeof t, "负载%d / %d", cur, cap);
    if (after >= 0 && after != cur && b == band(cur)) c = after < cur ? NsDraw::GREEN : NsDraw::INK;   /* (the same band: lighter green, heavier ink) */
    float w = P.width(P.fo, NsDraw::FS_SMALL, t);
    P.text(P.fo, NsDraw::FS_SMALL, 0, y, c, t, x1);
    return w;
}

int ns_against_worn(int id, int q, int m)
{
    int sl = ns_slot_of(id);
    if (!sl || m < 0 || m >= 5 || !gm_member_present(m)) return 0;
    static const u16 TOFF[6] = { 0, 0x37, 0x38, 0x48, 0x3d, 0x0e }, QOFF[6] = { 0, 0x3b, 0x3c, 0x47, 0x44, 0x46 };
    u16 b = G_MEMBER(m);
    int t = (s8)g_rb((u16)(b + TOFF[sl])), wq = g_rb((u16)(b + QOFF[sl]));
    if (t < 0) return 2;
    const ItemDef *d = ns_itemdef(id);
    if (sl <= 2) {
        if (t < 67 || t > 84) return 2;
        int h = rb(G_FAR20A5, (u16)(0x781e + d->type)), wh = rb(G_FAR20A5, (u16)(0x781e + t));
        if (h != wh) return h > wh ? 1 : -1;
        return q > wq ? 1 : q < wq ? -1 : 0;
    }
    if (sl >= 4 && t > 62) return 2;
    if (t != d->type) return 0;
    return q > wq ? 1 : q < wq ? -1 : 0;
}


void ns_compare_arrow(ImDrawList *dl, float ax, float ay, int vs, float k)
{
    if (!vs) return;
    ImU32 ac = vs == 1 ? NsDraw::GREEN : vs == 2 ? IM_COL32(40, 96, 34, 170) : NsDraw::RED;
    ImVec2 p0 = NsDraw::w(ax - 2, vs < 0 ? ay - 1.4f : ay + 1.4f), p1 = NsDraw::w(ax + 2, vs < 0 ? ay - 1.4f : ay + 1.4f), p2 = NsDraw::w(ax, vs < 0 ? ay + 1.8f : ay - 1.8f);
    if (vs == 2) dl->AddTriangle(p0, p1, p2, ac, 0.3f * k); else dl->AddTriangleFilled(p0, p1, p2, ac);
}

/* the section of the items list a thing goes in (the character sheet, the shops): 0 equipped (the
   caller's), 1 potions, 2 spare equipment (weapons, armour, shields, mounts), 3 utility (rope,
   lockpicks, instruments, tools ...), 4 trophies and quest things (relics, documents), 5 ingredients */
int ns_item_section(int id)
{
    const ItemDef *d = ns_itemdef(id);
    if (!d) return 3;
    switch (d->kind) {
    case K_POTION: return 1;
    case K_WEAPON: case K_MISSILE: case K_ARMOR: case K_SHIELD: case K_HORSE: return 2;
    case K_RELIC: case K_DOCUMENT: case K_QUEST: return 4;
    case K_INGREDIENT: return 5;
    default: return 3;
    }
}
const char *ns_item_section_name(int sec)
{
    static const char *N[6] = { "已装备", "药剂", "备用装备", "工具及其他物品", "奖杯与任务物品", "配料" };
    return sec >= 0 && sec < 6 ? N[sec] : "";
}

/* ---------------------------------------------------------------- a merchant's counter
   The screens where things change hands (the shop, the store room): the back wall of dark timber
   with an awning's scalloped edge along the top, the oak counter below it (the alchemy desk's
   table), the lists laid on it as sheets of paper with their shadows (ns_sheet). Drawn once and
   kept (ui/drawcache.h). */
#include "alchemydesk.h"
#include "drawcache.h"
int ns_page_frame = -10;                 /* the frame NsDraw::page was last drawn */
void ns_counter_backdrop(NsDraw &P, float wall_to, ImU32 awning, bool stone)
{
    ImVec2 o = NsDraw::w(60, 0), e = NsDraw::w(320, 200);
    unsigned long long key = 0xc0417e5ull;
    key = dc_key(key, wall_to); key = dc_key(key, (double)awning); key = dc_key(key, stone);
    dc_draw(P.dl, o, e, key, [&](ImDrawList *dl) {
        desk_table(dl);
        /* the wall: vertical boards, darker, with their seams, the counter's front edge under it */
        ImVec2 a = NsDraw::w(60, 0), b = NsDraw::w(320, wall_to);
        dl->AddRectFilledMultiColor(a, b, IM_COL32(58, 36, 20, 255), IM_COL32(52, 32, 18, 255), IM_COL32(40, 24, 12, 255), IM_COL32(44, 26, 14, 255));
        if (stone) {                                           /* a cellar's wall: courses of stone blocks, lit from the upper left */
            dl->AddRectFilledMultiColor(a, b, IM_COL32(96, 90, 80, 255), IM_COL32(86, 80, 72, 255), IM_COL32(58, 54, 48, 255), IM_COL32(64, 60, 54, 255));
            for (int r = 0; r * 7.5f < wall_to; r++) {
                float y0 = r * 7.5f, y1 = fminf(wall_to, y0 + 7.5f);
                for (int c = -1; c < 14; c++) {
                    float x0 = 60 + c * 20 + (r & 1) * 10 + (float)((r * 7 + c * 13) % 5), x1 = x0 + 19.4f;
                    unsigned h = (unsigned)(r * 131 + c * 71) * 2654435761u; int v = (int)((h >> 24) % 30) - 15;
                    dl->AddRectFilled(NsDraw::w(fmaxf(60, x0 + 0.6f), y0 + 0.6f), NsDraw::w(fminf(320, x1), y1 - 0.4f), IM_COL32(110 + v, 104 + v, 92 + v, 120), 1.2f * P.k);
                    dl->AddLine(NsDraw::w(fmaxf(60, x0 + 0.6f), y0 + 0.8f), NsDraw::w(fminf(320, x1), y0 + 0.8f), IM_COL32(170, 160, 140, 70), 1.0f);
                }
                dl->AddLine(NsDraw::w(60, y1), NsDraw::w(320, y1), IM_COL32(30, 26, 22, 200), 1.2f);
            }
        } else
        for (int i = 0; i < 18; i++) {
            float x = 60 + i * 14.6f + (i * 37 % 5);
            dl->AddLine(NsDraw::w(x, 0), NsDraw::w(x, wall_to), IM_COL32(20, 10, 4, 200), 1.4f);
            dl->AddLine(NsDraw::w(x + 0.6f, 0), NsDraw::w(x + 0.6f, wall_to), IM_COL32(110, 74, 40, 60), 1.0f);
        }
        dl->AddRectFilledMultiColor(NsDraw::w(60, wall_to - 1.2f), NsDraw::w(320, wall_to + 2.2f), IM_COL32(120, 78, 40, 255), IM_COL32(120, 78, 40, 255), IM_COL32(60, 36, 16, 255), IM_COL32(60, 36, 16, 255));
        dl->AddRectFilledMultiColor(NsDraw::w(60, wall_to + 2.2f), NsDraw::w(320, wall_to + 6), IM_COL32(0, 0, 0, 110), IM_COL32(0, 0, 0, 110), 0, 0);
        /* the awning's edge: scallops in two colours */
        if (awning) {
            ImU32 alt = IM_COL32(236, 224, 196, 255);
            for (int i = 0; i < 26; i++) {
                float x0 = 60 + i * 10, xm = x0 + 5;
                ImU32 c = i & 1 ? alt : awning;
                dl->AddRectFilled(NsDraw::w(x0, 0), NsDraw::w(x0 + 10, 1.4f), c);
                dl->PathClear(); dl->PathLineTo(NsDraw::w(x0, 1.4f));
                for (int k = 0; k <= 8; k++) { float t = 3.14159f * k / 8; dl->PathLineTo(NsDraw::w(xm - cosf(t) * 5, 1.4f + sinf(t) * 2.2f)); }
                dl->PathFillConvex(c);
            }
        }
    });
}
/* a sheet of paper lying on the counter: its shadow, the paper (then the list is written on it) */
void ns_sheet(NsDraw &P, float x0, float y0, float x1, float y1, ImU32 paper)
{
    P.dl->AddRectFilled(NsDraw::w(x0 + 1.2f, y0 + 1.8f), NsDraw::w(x1 + 1.6f, y1 + 2.2f), IM_COL32(0, 0, 0, 90), 2 * P.k);
    P.dl->AddRectFilled(NsDraw::w(x0, y0), NsDraw::w(x1, y1), paper, 1.5f * P.k);
    P.dl->AddRectFilledMultiColor(NsDraw::w(x0, y1 - 4), NsDraw::w(x1, y1), 0, 0, IM_COL32(120, 90, 50, 40), IM_COL32(120, 90, 50, 40));
}

/* the party's pack: a leather pack with its flap, buckle and shoulder strap (drawn, as the order and
   skill pictures, so it keeps sharp at any size) */
void ns_backpack_icon(NsDraw &P, float cx, float cy, float s)
{
    ImDrawList *dl = P.dl;
    auto W = [&](float x, float y) { return NsDraw::w(cx + x * s * P.k / P.kx, cy + y * s); };   /* (square on screen: a column is narrower than a row) */
    float r = 0.12f * s * P.k;
    const ImU32 strap = IM_COL32(92, 52, 22, 255), body = IM_COL32(150, 92, 44, 255), flap = IM_COL32(118, 68, 30, 255),
                edge = IM_COL32(60, 34, 14, 255), gold = IM_COL32(214, 172, 74, 255);
    dl->AddBezierCubic(W(-0.22f, -0.30f), W(-0.22f, -0.58f), W(0.22f, -0.58f), W(0.22f, -0.30f), strap, fmaxf(1.0f, 0.09f * s * P.k));   /* (the carrying loop) */
    dl->AddRectFilled(W(-0.40f, -0.34f), W(0.40f, 0.50f), body, r);                      /* (the bag) */
    dl->AddRectFilled(W(-0.40f, 0.18f), W(0.40f, 0.50f), IM_COL32(132, 80, 36, 255), r, ImDrawFlags_RoundCornersBottom);   /* (its pocket) */
    dl->AddRect(W(-0.40f, -0.34f), W(0.40f, 0.50f), edge, r, 0, fmaxf(1.0f, 0.05f * s * P.k));
    dl->AddRectFilled(W(-0.42f, -0.36f), W(0.42f, 0.06f), flap, r, ImDrawFlags_RoundCornersAll);   /* (the flap over it) */
    dl->AddRect(W(-0.42f, -0.36f), W(0.42f, 0.06f), edge, r, 0, fmaxf(1.0f, 0.05f * s * P.k));
    dl->AddRectFilled(W(-0.05f, -0.06f), W(0.05f, 0.30f), strap);                        /* (the strap down the flap) */
    dl->AddRect(W(-0.10f, 0.02f), W(0.10f, 0.16f), gold, 0, 0, fmaxf(1.0f, 0.05f * s * P.k));   /* (its buckle) */
}
