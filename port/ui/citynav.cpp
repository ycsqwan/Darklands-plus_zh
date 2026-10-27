#include <stdarg.h>
// citynav.cpp - "Go to" in cities. A city in Darklands is a maze of card menus (main street,
// market, churches, the crafts district, side streets ...); the list here names every place at
// once, and picking one walks the party there through the game's own menus - the very options a
// player would click, one card after the other, at the game's pace. It stops at anything it
// does not know (an encounter on the way, a closed gate), at a key press or click of the
// player, and when it has chosen the place.
//
// Cards are told apart by their names (s043 card_name, DS:e3d4: "$MainS01" ...); the option
// lines by their text in the card file (the text block on show, shown_block D_e959 - what
// card_options last showed - option k = the k-th option mark), so nothing depends on the crisp
// text or on the screen. (Not options_block, D_ee53: only some scripts keep their list's block
// there, so another card's stayed in it - Freiberg's main street read as block 4 of four.)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <string>
#include <vector>
#include <map>
#include "richtext.h"
#include "imgui.h"
#include "ornament.h"
#include "ui.h"
#include "gamedata.h"
#include "teachersui.h"
extern "C" {
#include "../mods/offices.h"
}
#include "townicons.h"
#include "quests.h"
extern "C" {
int datamods_generation(void);
#include "../runtime/cpu.h"
#include "../runtime/game.h"
#include "../runtime/gamevars.h"
#include "../runtime/config.h"
#include "../runtime/hle.h"
#include "../runtime/hooks.h"
#include "../mods/assist.h"
#include "../mods/cardview.h"
#include "../mods/fixes.h"
#include "../mods/gamemodel.h"
u32 plat_ticks_ms(void);
void plat_click_game(int gx, int gy);
void ui_notice(const char *msg);
}

/* ---------------------------------------------------------------- the card files */
static std::map<std::string, std::vector<std::string>> cards;   /* "$MAINS01" -> text blocks */
static bool cards_loaded;

static void load_cards(void)
{
    cards_loaded = true;
    char path[1300]; dos_host_path_read("MSGFILES", path, sizeof path);
    FILE *f = fopen(path, "rb");
    if (!f) return;
    std::vector<unsigned char> d;
    unsigned char buf[65536]; size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) d.insert(d.end(), buf, buf + n);
    fclose(f);
    if (d.size() < 2) return;
    unsigned cnt = d[0] | d[1] << 8;
    for (unsigned i = 0; i < cnt && 2 + (i + 1) * 24 <= d.size(); i++) {
        const unsigned char *e = &d[2 + i * 24];
        char name[13]; memcpy(name, e, 12); name[12] = 0;
        for (char *p = name; *p; p++) { *p = (char)toupper((unsigned char)*p); if (*p == '.') *p = 0; }
        unsigned len = e[16] | e[17] << 8 | e[18] << 16 | (unsigned)e[19] << 24, off = e[20] | e[21] << 8 | e[22] << 16 | (unsigned)e[23] << 24;
        if ((size_t)off + len > d.size() || len < 1) continue;
        const unsigned char *m = &d[off], *end = m + len;
        unsigned nb = *m++;
        std::vector<std::string> blocks;
        for (unsigned b = 0; b < nb && m + 5 <= end; b++) {
            m += 5;
            const unsigned char *z = m; while (z < end && *z) z++;
            blocks.emplace_back((const char *)m, (size_t)(z - m));
            m = z + 1;
        }
        cards[name] = blocks;
    }
}

/* the option texts of a text block, by option number (lower case, "$vars" kept) */
static std::vector<std::string> block_options(const std::string &t)
{
    std::vector<std::string> o;
    for (size_t i = 0; i < t.size(); i++) {
        unsigned char c = (unsigned char)t[i];
        if (c != 0x15 && c != 0x16 && c != 0x10 && c != 0x06) continue;
        size_t e = t.find('\n', i + 1); if (e == std::string::npos) e = t.size();
        std::string s = t.substr(i + 1, e - i - 1);
        size_t sep = s.find('\x1d'); if (sep != std::string::npos) s = s.substr(sep + 1);
        for (char &ch : s) ch = (char)tolower((unsigned char)ch);
        o.push_back(s);
        i = e - 1;
    }
    return o;
}

/* whether block `block` of card `card` (as the card files have it, $variables unexpanded)
   contains `word` (ui/quests.cpp: what a card tells of a Sabbat) */
extern "C" int citynav_card_block_has(const char *card, int block, const char *word)
{
    { static int gen = -1; int datamods_generation(void); if (gen != datamods_generation()) { gen = datamods_generation(); cards.clear(); cards_loaded = false; } }
    if (!cards_loaded) load_cards();
    std::string k(card); for (char &c : k) c = (char)toupper((unsigned char)c);
    auto it = cards.find(k);
    if (it == cards.end() || block < 0 || block >= (int)it->second.size()) return 0;
    return it->second[block].find(word) != std::string::npos;
}

/* the text blocks of the card on show, as the game loaded them (msg_file, DGROUP a895, by msg_load:
   the .msg as in MSGFILES - the mods' and the restored cards' too, which MSGFILES on disk may not
   hold); empty when none is loaded */
static std::vector<std::string> live_blocks(void)
{
    std::vector<std::string> blocks;
    const u16 MSG = 0xa895, MAXLEN = 6000;
    unsigned nb = g_rb(MSG), pos = 1;
    if (nb == 0 || nb > 64) return blocks;
    for (unsigned b = 0; b < nb && pos + 5 < MAXLEN; b++) {
        pos += 5;
        std::string t;
        while (pos < MAXLEN) { char c = (char)g_rb((u16)(MSG + pos)); pos++; if (!c) break; t += c; }
        blocks.push_back(t);
    }
    return blocks;
}
/* the text of the card last loaded (msg_load, s043 150b:020e: its name and blocks), copied as it is
   loaded: the game's buffer (msg_file, DS:a895) is shared - the loot, a level's enemy table, the
   screens opened from the menus write over it - so it holds the card's text only just after */
static std::string loaded_name;
static std::vector<std::string> loaded_blocks;
static void msg_load_hook(void)
{
    u16 off = rw(SS, (u16)(SP + 4));                         /* (char *name, DGROUP) */
    hook_call_original();
    char n[16]; g_str(G_DGROUP, off, n, sizeof n);
    for (char *q = n; *q; q++) *q = (char)toupper((unsigned char)*q);
    loaded_name = n; loaded_blocks = live_blocks();
}
extern "C" void citynav_init(void) { hook_install(GF_msg_load, msg_load_hook); }
/* card's text: as it was loaded (a mod's card, a restored one, the game's own: what is on show), else as
   MSGFILES has it */
static std::vector<std::string> card_blocks(const std::string &card)
{
    if (loaded_name == card && !loaded_blocks.empty()) return loaded_blocks;
    if (!cards_loaded) load_cards();
    auto it = cards.find(card);
    return it != cards.end() ? it->second : std::vector<std::string>();
}
extern "C" void citynav_test_live(void)                    /* (test ulivecard: the loaded card against MSGFILES) */
{
    if (!cards_loaded) load_cards();
    char n[13]; g_str(G_DGROUP, 0xe3d4, n, sizeof n); std::string k(n); for (char &c : k) c = (char)toupper((unsigned char)c);
    std::vector<std::string> lb = live_blocks(); auto it = cards.find(k);
    fprintf(stderr, "livecard %s: %d blocks in memory, %d in MSGFILES, %s\n", k.c_str(), (int)lb.size(), it == cards.end() ? -1 : (int)it->second.size(),
            it != cards.end() && it->second == lb ? "the same" : "different");
}

static std::string card_now(void)
{
    char n[13]; g_str(G_DGROUP, 0xe3d4, n, sizeof n);
    for (char *p = n; *p; p++) *p = (char)toupper((unsigned char)*p);
    return n;
}

/* ---------------------------------------------------------------- the city */
enum Hub { H_MAIN, H_MARKET, H_CHURCHES, H_CRAFTS, H_ARMS, H_CIVIL, H_SIDE, H_SQUARE, H_N, H_NONE = -1 };
static const char *HUB_CARD[H_N] = { "$MAINS", "$MARKE", "$CHURC", "$BUSIN", "$MILCR", "$CIVCR", "$SIDES", "$CITYS" };

enum Dest {
    D_MERCHANT, D_FOREIGN, D_SWORD, D_ARMOR, D_BOW, D_BLACKSMITH, D_SOLDIERS, D_JEWEL, D_TINKER, D_CLOTH,
    D_ALCHEMIST, D_HERBAL, D_PHARMACIST, D_PHYSICIAN,
    D_FUGGER, D_MEDICI, D_HANSE, D_PAWN,
    D_CATHEDRAL, D_CHURCH, D_MONASTERY, D_UNIVERSITY, D_HOSPITAL,
    D_INN, D_SQUARE, D_TOWNHALL, D_PRISON, D_BARRACKS, D_FORTRESS, D_DOCKS, D_SLUM, D_GROVE, D_WALL, D_LEAVE, D_N
};
/* a place: its name in the list, what it is (the card texts, said plainly), its section, the
   building bit / shop quality index of DARKLAND.CTY that it needs (-1 any; -2 the bowyer or the
   gunsmith), and its picture (ui/townicons.h) */
struct DestInfo { const char *label; const char *desc; int group; int building; int shop; int icon; };
static const DestInfo DEST[D_N] = {
    { "商人", "市集摊位：日常商品的买卖。", 0, -1, 1, TI_MARKET },
    { "外国商人", "来自远方商人的凉亭：可买卖的商品。", 0, -1, -1, TI_FOREIGN },
    { "锻剑师", "锻剑师巷：武器与锻剑师公会。", 0, -1, 2, TI_SWORD },
    { "锻甲师", "锻甲师路：盔甲与盾牌，以及锻甲师公会。", 0, -1, 3, TI_ARMOUR },
    { "弓箭匠", "弓箭匠与枪匠广场：弓、弩与枪及其公会。", 0, -1, -2, TI_BOW },
    { "锻铁匠", "锻铁匠街：武器与锻铁匠公会。", 0, -1, 0, TI_SMITH },
    { "士兵之路", "商店与推车挤满了士兵与随营人员，还有这条街的领头人。", 0, -1, -1, TI_SOLDIERS },
    { "珠宝匠", "", 0, -1, 7, TI_FOREIGN },
    { "工艺师", "铁匠广场：锤子、狼牙棒、连枷与长柄武器，以及工匠公会。", 0, -1, 6, TI_TOOLS },
    { "布料商", "布料商街：服装与软质护甲（棉垫、皮革、钉皮、煮皮），以及布料商公会。", 0, -1, 8, TI_CLOTH },   /* (shown with the cardopts fix: by day its shopfronts are switched off in 483.07) */
    { "炼金术士", "占星师巷：可购买或交易的配方、更好的哲人石、炼金术课程。", 3, -1, -1, TI_ALCHEMIST },
    { "草药师", "", 3, -1, -1, TI_PHARMACY },
    { "药剂师", "摆满草药与稀有材料的摊位：炼金术原料。", 0, -1, -1, TI_PHARMACY },
    { "医生", "付费治疗伤口、学习治愈术、买卖炼金术原料。", 3, -1, -1, TI_PHYSICIAN },
    { "富格尔银行", "重币兑换成毫无重量的银行票据，反之亦然，需支付手续费。", 1, -1, -1, TI_FUGGER },
    { "美第奇银行", "美第奇代表：银行票据，与富格尔银行相同。", 1, -1, -1, TI_MEDICI },
    { "汉萨同盟", "北方商镇联盟的办公机构：接受报酬的任务。礼貌询问时，他们会根据队伍在汉萨中的地位来评判；大胆询问时，则依据其在此地的声誉与名气。礼貌领取奖励也能提升这一地位。", 1, -1, -1, TI_HANSE },
    { "当铺", "富格尔家族的借贷所：种类繁多的日常用品，以及几乎任何出售物品都能换取的现金。", 0, 9, -1, TI_PAWN },
    { "大教堂", "弥撒（为所有人带来些许神恩）；捐献钱包的三分之一（领袖的神恩）；守护圣人的故事；为教士跑腿（一件圣物）；庇护所。", 2, 4, -1, TI_CATHEDRAL },
    { "教堂", "定时弥撒（为所有人带来神恩，宗教与拉丁文越多越佳）；告解（领袖的神恩，数小时）；缴纳十分之一的钱包税（所有人的神恩已满）；当城镇与你为敌时的庇护所。", 2, -1, -1, TI_CHURCH },
    { "修道院", "僧侣图书馆（宗教、拉丁文）及祈求神恩的祷告。", 2, 7, -1, TI_MONASTERY },
    { "大学", "新的圣人与待学的炼金术配方、稀有炼金材料、更好的哲人石。工作人员期待掌握拉丁文并具备敏锐头脑。", 3, 8, -1, TI_UNIVERSITY },
    { "临终关怀所", "修女照料病患：施舍、陪伴工作一天（治愈术）、付费或免费治疗伤口（若她们接纳你）。仅白天开放。", 3, -1, -1, TI_PHYSICIAN },
    { "旅店", "提供餐饮与床位、新闻与传闻、马厩、储藏室；也可留宿学习或训练。", 4, -1, -1, TI_INN },
    { "城市广场", "通知、闲话与新闻。", 4, 1, -1, TI_SQUARE },
    { "市政厅", "市议会：觐见领主，地牢中的囚犯。", 1, 1, -1, TI_TOWNHALL },
    { "监狱", "城市的监狱。", 4, 1, -1, TI_PRISON },
    { "兵营", "城市守卫的总部：招募队伍成员，武器训练。", 4, 1, -1, TI_BARRACKS },
    { "要塞", "领主的要塞：觐见领主，地牢中的囚犯。", 1, 2, -1, TI_FORTRESS },
    { "码头", "前往其他城镇的船只，需付费（每艘船在指定日期启航）。", 4, 11, -1, TI_DOCKS },
    { "贫民窟", "危险的贫民窟：流言蜚语，以及一间价格低廉的房间。", 4, 6, -1, TI_SLUMS },
    { "树林", "队伍可以等待的安静角落：一小时、三小时，或直到夜幕降临。", 4, -1, -1, TI_GROVE },
    { "城墙", "通往城墙某段的小巷：无法使用城门时的出路。", 4, -1, -1, TI_WALL },
    { "城门", "从其中一座城门离开城市。", 4, -1, -1, TI_GATE },
};
static const char *GROUPS[5] = { "商人", "任务发布者", "宗教", "学习与治疗", "城镇周边" };
/* the order within the list (the sections' own); every place has its tile in every town, the
   ones the town lacks greyed, so the tiles stay where they are from town to town */
static const int ORDER[] = {
    D_MERCHANT, D_FOREIGN, D_PHARMACIST, D_PAWN, D_SWORD, D_ARMOR, D_BOW, D_BLACKSMITH, D_TINKER, D_CLOTH, D_SOLDIERS, D_JEWEL,
    D_FUGGER, D_MEDICI, D_HANSE, D_TOWNHALL, D_FORTRESS,
    D_CATHEDRAL, D_CHURCH, D_MONASTERY,
    D_UNIVERSITY, D_PHYSICIAN, D_ALCHEMIST, D_HERBAL, D_HOSPITAL,
    D_INN, D_SQUARE, D_BARRACKS, D_PRISON, D_DOCKS, D_SLUM, D_GROVE, D_WALL, D_LEAVE,
};

/* what an option of a hub card leads to: a place (D_*) or another hub (100 + H_*) */
struct Rule { int hub; const char *key; int to; };
static const Rule RULES[] = {
    { H_MAIN, "$citysquare", 100 + H_SQUARE }, { H_MAIN, "$fortress", D_FORTRESS }, { H_MAIN, "$marketplace", 100 + H_MARKET },
    { H_MAIN, "spires", 100 + H_CHURCHES }, { H_MAIN, "craft", 100 + H_CRAFTS }, { H_MAIN, "$inn", D_INN }, { H_MAIN, "docks", D_DOCKS },
    { H_MAIN, "弄堂", 100 + H_SIDE }, { H_MAIN, "grove", D_GROVE }, { H_MAIN, "gate", D_LEAVE },
    { H_MARKET, "everyday", D_MERCHANT }, { H_MARKET, "foreign", D_FOREIGN }, { H_MARKET, "pharmacist", D_PHARMACIST },
    { H_MARKET, "富格尔银行", D_FUGGER }, { H_MARKET, "medici", D_MEDICI }, { H_MARKET, "hanseatic", D_HANSE },
    { H_MARKET, "leihhaus", D_PAWN }, { H_MARKET, "主街", 100 + H_MAIN }, { H_MARKET, "弄堂", 100 + H_SIDE },
    { H_CHURCHES, "$cathedral", D_CATHEDRAL }, { H_CHURCHES, "$citychurch", D_CHURCH }, { H_CHURCHES, "$hospital", D_HOSPITAL },
    { H_CHURCHES, "$monastery", D_MONASTERY }, { H_CHURCHES, "$university", D_UNIVERSITY }, { H_CHURCHES, "$citysquare", 100 + H_SQUARE },
    { H_CHURCHES, "$fortress", D_FORTRESS }, { H_CHURCHES, "主街", 100 + H_MAIN }, { H_CHURCHES, "弄堂", 100 + H_SIDE },
    { H_CRAFTS, "武器与盔甲", 100 + H_ARMS }, { H_CRAFTS, "较为温和", 100 + H_CIVIL }, { H_CRAFTS, "$inn", D_INN },
    { H_CRAFTS, "physician", D_PHYSICIAN }, { H_CRAFTS, "grove", D_GROVE }, { H_CRAFTS, "$slum", D_SLUM },
    { H_CRAFTS, "主街", 100 + H_MAIN }, { H_CRAFTS, "弄堂", 100 + H_SIDE }, { H_CRAFTS, "城墙", D_WALL },
    { H_CRAFTS, "gate", D_LEAVE },
    { H_ARMS, "soldier", D_SOLDIERS }, { H_ARMS, "blacksmith", D_BLACKSMITH }, { H_ARMS, "swordsmith", D_SWORD },
    { H_ARMS, "armorer", D_ARMOR }, { H_ARMS, "bowyer", D_BOW }, { H_ARMS, "其他手工艺", 100 + H_CIVIL },
    { H_ARMS, "弄堂", 100 + H_SIDE }, { H_ARMS, "主街", 100 + H_MAIN },
    { H_CIVIL, "physician", D_PHYSICIAN }, { H_CIVIL, "astrologist", D_ALCHEMIST }, { H_CIVIL, "herbalist", D_HERBAL },
    { H_CIVIL, "jeweler", D_JEWEL }, { H_CIVIL, "tinker", D_TINKER }, { H_CIVIL, "clothmaker", D_CLOTH },
    { H_CIVIL, "-making", 100 + H_ARMS }, { H_CIVIL, "弄堂", 100 + H_SIDE }, { H_CIVIL, "主街", 100 + H_MAIN },
    { H_SIDE, "主街", 100 + H_MAIN }, { H_SIDE, "$inn", D_INN }, { H_SIDE, "$cityslums", D_SLUM },
    { H_SIDE, "grove", D_GROVE }, { H_SIDE, "城墙", D_WALL },
    /* (by night, $SIDES01 and $MAINS02 put it otherwise) */
    { H_SIDE, "城市集市", 100 + H_MARKET }, { H_SIDE, "城市中心广场", 100 + H_SQUARE }, { H_SIDE, "spires", 100 + H_CHURCHES },
    { H_SIDE, "手工艺区", 100 + H_CRAFTS }, { H_SIDE, "docks", D_DOCKS }, { H_SIDE, "中心要塞", D_FORTRESS },
    { H_MAIN, "手工艺区", 100 + H_CRAFTS }, { H_MAIN, "弄堂与小巷", 100 + H_SIDE },
    /* (other wordings of the same ways, in other text blocks of the same cards: by night, or a hub's
       other card; every hub card's every block is checked against this table - a line none of these
       phrases matches, the walk cannot take: "not found in this city") */
    { H_CIVIL, "炼金术符号", D_ALCHEMIST }, { H_MAIN, "伟大教堂", 100 + H_CHURCHES }, { H_MARKET, "$pawnshop", D_PAWN },
    { H_SIDE, "$citysquare", 100 + H_SQUARE }, { H_SIDE, "$fortress", D_FORTRESS }, { H_SIDE, "$marketplace", 100 + H_MARKET },
    { H_SIDE, "伟大教堂", 100 + H_CHURCHES }, { H_SIDE, "普通街区", 100 + H_CRAFTS }, { H_SIDE, "商业与手工艺街区", 100 + H_CRAFTS },
    { H_SIDE, "通往城外的城门", D_LEAVE },
    { H_SQUARE, "张贴的告示", D_SQUARE },
    { H_SQUARE, "$councilhall", D_TOWNHALL }, { H_SQUARE, "prison", D_PRISON }, { H_SQUARE, "$citybarracks", D_BARRACKS },
    { H_SQUARE, "$university", D_UNIVERSITY }, { H_SQUARE, "spires", 100 + H_CHURCHES }, { H_SQUARE, "$marketplace", 100 + H_MARKET },
    { H_SQUARE, "主街", 100 + H_MAIN }, { H_SQUARE, "弄堂", 100 + H_SIDE },
    /* lines that lead nowhere Go to goes (-2: known, not a way): a quest's own house, the night
       market's ways upstairs (stopped there with what to do), placeholders the game hides */
    { H_MARKET, "upstairs", -2 }, { H_MARKET, "避开它们", -2 }, { H_MARKET, "攻击守卫", -2 },
    { H_ARMS, "specific", -2 }, { H_SIDE, "specific", -2 }, { H_SIDE, "你记得的其他地点", -2 },
    { H_SIDE, "连接到重要区域", -2 }, { H_SIDE, "$poorhouse", -2 }, { H_SIDE, "待定", -2 },
    { H_ARMS, "未使用", -2 }, { H_CIVIL, "未使用", -2 },
};
/* a hub that is itself a place to go to (the square: its notices and gossip) */
static const int HUB_PLACE[H_N] = { -1, -1, -1, -1, -1, -1, -1, D_SQUARE };

static int hub_of(const std::string &card)
{
    for (int h = 0; h < H_N; h++) if (!strncmp(card.c_str(), HUB_CARD[h], 6)) return h;
    return H_NONE;
}
extern "C" int cutcards_hospice_on(void);
static int rule_for(int hub, const std::string &text)
{
    if (hub == H_CHURCHES && text.empty() && cutcards_hospice_on()) return D_HOSPITAL;   /* ($Churc00's empty line: the hospice, mods/cutcards.c) */
    for (const Rule &r : RULES) if (r.hub == hub && text.find(r.key) != std::string::npos) return r.to;
    return -1;
}
/* a line that is no way at all: empty, a number or an "x" the game puts for an option it never
   shows (the card files' placeholders) */
static bool placeholder_line(const std::string &t)
{
    size_t k = 0; while (k < t.size() && (t[k] == ' ' || t[k] == '.')) k++;
    std::string u = t.substr(k); while (!u.empty() && (u.back() == ' ' || u.back() == '.')) u.pop_back();
    if (u.empty() || u == "x") return true;
    for (char c : u) if (c < '0' || c > '9') return false;
    return true;
}

/* test (ucitylines): every option line of every hub card, every text block, through rule_for;
   the lines no rule takes are logged, with a count at the end */
extern "C" void citynav_check_lines(void)
{
    if (!cards_loaded) load_cards();
    int n = 0, bad = 0;
    for (auto &c : cards) {
        int hub = hub_of(c.first); if (hub == H_NONE) continue;
        for (size_t b = 0; b < c.second.size(); b++) {
            std::vector<std::string> o = block_options(c.second[b]);
            for (size_t i = 0; i < o.size(); i++) {
                n++;
                if (rule_for(hub, o[i]) == -1 && !placeholder_line(o[i])) { bad++; fprintf(stderr, "citylines: %s/%d/%d unmatched: %s\n", c.first.c_str(), (int)b, (int)i, o[i].c_str()); }
            }
        }
    }
    fprintf(stderr, "citylines: %d lines, %d unmatched\n", n, bad);
}
static int choose_leave(const std::vector<std::string> &line);
/* test (ucityplaces): every option list of every card that is not a hub, through choose_leave - the
   way out that Go to takes from a place; a card some of whose lists have a way out and others none
   (a place where the walk could be stuck) is logged with those lists */
extern "C" void citynav_check_places(void)
{
    if (!cards_loaded) load_cards();
    int ncards = 0, partial = 0;
    for (auto &c : cards) {
        if (hub_of(c.first) != H_NONE) continue;
        std::vector<int> out, none;
        for (size_t b = 0; b < c.second.size(); b++) {
            std::vector<std::string> o = block_options(c.second[b]);
            if (o.empty()) continue;
            bool ph = true; for (auto &x : o) if (!placeholder_line(x)) ph = false;
            if (ph) continue;
            (choose_leave(o) >= 0 ? out : none).push_back((int)b);
        }
        if (out.empty()) continue;                        /* (no way out anywhere: not a place of the streets) */
        ncards++;
        if (none.empty()) continue;
        partial++;
        for (int b : none) {
            std::vector<std::string> o = block_options(c.second[b]);
            std::string all; for (auto &x : o) all += " | " + x.substr(0, 60);
            fprintf(stderr, "cityplaces: %s/%d no way out:%s\n", c.first.c_str(), b, all.c_str());
        }
    }
    fprintf(stderr, "cityplaces: %d cards with a way out, %d with lists that have none\n", ncards, partial);
}

/* steps from each hub to a place, through the hubs (side streets cost more: more encounters) */
static int dist_to(int dest, int from, unsigned avoid)
{
    int d[H_N]; for (int h = 0; h < H_N; h++) d[h] = HUB_PLACE[h] == dest ? 0 : 1000;
    for (const Rule &r : RULES) if (r.to == dest && !(avoid >> r.hub & 1)) d[r.hub] = 1;
    for (int it = 0; it < H_N; it++)
        for (const Rule &r : RULES)
            if (r.to >= 100 && !(avoid >> r.hub & 1)) {
                int c = d[r.to - 100] + (r.to - 100 == H_SIDE ? 3 : 1);
                if (c < d[r.hub]) d[r.hub] = c;
            }
    return d[from];
}


/* a timed refusal the cards keep as an event at the town (event_here: kind, the hours; +0x1c the
   place, +0x22 the kind, +0 the type, +0x12.. the end: hour, day, month, year): the alchemists
   turning the party away ($Alche00: 0x44, 60 hours), the monastery's library after a visit
   ($CityM00 option 2: 0x34, 60 days after studying, 30 after a refusal), its prayers (0x30, a
   week), the university's library after a refusal ($Unive00: 0x4e type 0, a week) */
static bool town_event_end(int loc, int kind, int type0, int end[4])
{
    bool any = false; long best = 0;
    for (int i = 0; i < G_NEVENTS; i++) {
        u32 p = g_event(i);
        if (!p) continue;
        u16 sg = (u16)(p >> 16), o = (u16)p;
        if ((s16)rw(sg, (u16)(o + 0x22)) != kind || (s16)rw(sg, (u16)(o + 0x1c)) != loc) continue;
        if (type0 >= 0 && (s16)rw(sg, o) != type0) continue;
        int e[4]; for (int k = 0; k < 4; k++) e[k] = (s16)rw(sg, (u16)(o + 0x12 + 2 * k));
        long key = (((long)e[3] * 12 + e[2]) * 31 + e[1]) * 24 + e[0];
        if (!any || key > best) { any = true; best = key; memcpy(end, e, sizeof e); }
    }
    return any;
}
/* ---------------------------------------------------------------- walking */
static bool night_closed(int d);
static int goal = -1, steps_taken;
static unsigned visited;                                   /* hubs passed on this walk: not again */
static int conts, leaves, replans;
static int arrived_last;                                   /* the last walk ended by arriving (not stopped) */                         /* texts read on, places left before the first hub, ways planned anew */
static std::string last_key; static int same_count;
static u32 phase_t, last_act;
static std::string phase_key;

/* the walk's steps: printed as they go with DL_CITYNAV_LOG, and kept, so that a walk which fails
   writes how it went into the log (darklands_port.log) for a bug report */
static int logon = -1;
static std::string trail;
static void trail_add(const char *fmt, ...)
{
    char b[512]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    if (logon < 0) logon = getenv("DL_CITYNAV_LOG") != 0;
    if (logon) fprintf(stderr, "citynav: %s", b);
    if (trail.size() > 6000) trail.erase(0, trail.size() - 4000);
    trail += b;
}
#define LOG(...) trail_add(__VA_ARGS__)

static void stop(const char *why)
{
    LOG("stop: %s\n", why ? why : "");
    bool failed = why && strcmp(why, "stopped.") && goal >= 0;
    if (failed && !logon) { fprintf(stderr, "Go to %s failed (%s). The walk:\n%s", DEST[goal].label, why, trail.c_str()); fflush(stderr); }
    if (goal >= 0 && why) { char m[160]; snprintf(m, sizeof m, "前往 %s：%s", DEST[goal].label, why); ui_notice(m); }
    goal = -1;
}

static int leaving_flag(int v);
extern "C" void citynav_user_input(void) { if (goal >= 0) stop("stopped."); leaving_flag(0); }
extern "C" int citynav_busy(void) { return goal >= 0; }
extern "C" { extern int hdtext_fast_fades; }

extern "C" int encounters_card_active(void);
extern "C" int encounters_builtin(int i);
static bool in_city(void)
{
    int cur = (s16)g_rw(G_CUR_LOC);
    /* (a new encounter met on the way out, still on the town's square of the map, keeps the town
       as the game's place: the party is on the road, not in the streets; the restored hospice is
       a place in town) */
    int enc = encounters_card_active();
    return cur >= 0 && cur < 0x5c && game_mode() == GM_CARD && (!enc || encounters_builtin(enc - 1));
}
/* before the walls: the city's approach, its gates and its walls ($CityE, $CityG00..04, $CityW),
   where no street leads anywhere yet (the groves, $CityG05/06, are inside) */
static bool outside_walls(void)
{
    std::string c = card_now();
    if (c.compare(0, 6, "$CITYE") == 0 || c.compare(0, 6, "$CITYW") == 0) return true;
    return c.compare(0, 6, "$CITYG") == 0 && c.size() >= 8 && c[6] == '0' && c[7] >= '0' && c[7] <= '4';
}
extern "C" int citynav_available(void) { return in_city() && !outside_walls(); }

static bool leave_option(const std::string &t)
{
    if (t.find("主街") == std::string::npos) return false;
    static const char *verbs[] = { "leave", "return", "出门", "远离", "悠闲地走开", "返回楼下", "去往", "exit" };
    for (const char *v : verbs) if (t.find(v) != std::string::npos) return true;
    return false;
}

static int choose_leave(const std::vector<std::string> &line)
{
    for (size_t i = 0; i < line.size(); i++) if (leave_option(line[i])) return (int)i;
    for (size_t i = 0; i < line.size(); i++) if (line[i].find("返回到") != std::string::npos && line[i].find("side") == std::string::npos) return (int)i;
    for (size_t i = 0; i < line.size(); i++)
        if ((line[i].find("leave") != std::string::npos || line[i].find("depart") != std::string::npos) && line[i].find("side") == std::string::npos
            && line[i].find("alley") == std::string::npos) return (int)i;
    for (size_t i = 0; i < line.size(); i++)                 /* the only way out may be a side street (the slums) or a step back */
        if (line[i].find("leave") != std::string::npos || line[i].find("后退") != std::string::npos || line[i].find("reconsider") != std::string::npos) return (int)i;
    /* a polite way out of a place's second list (every place card's every list is checked: ucityplaces) */
    static const char *const BACK[] = { "改变主意", "为打扰致歉", "不要卷入其中", "明早再来", "继续。" };
    for (const char *b : BACK) for (size_t i = 0; i < line.size(); i++) if (line[i].find(b) != std::string::npos) return (int)i;
    return -1;
}

static void click_line(int i)
{
    int o = (s8)g_rb((u16)(GV_card_layout_opt + i));
    LOG("choose line %d (option %d)\n", i, o);
    if (cardview_active() && o >= 0) cardview_choose(o);   /* the cards drawn by Darklands+: by the option's number (a click by
                                                               position could land on the next row: Alchemist led to the clothmakers) */
    else {
        int x0 = 60 + (s16)g_rw(GV_card_layout_x), y0 = g_rb((u16)(GV_card_layout_y + i)), h = g_rb((u16)(GV_card_layout_h + i));
        plat_click_game(x0 + 10, y0 + (h > 8 ? 4 : h / 2));
    }
    last_act = plat_ticks_ms(); steps_taken++;
}

/* leaving for a journey (the world map's "Leave and travel there"): the walk to a gate, then the
   card at the gate ($Selec00 ...) answered with the plain way out ("simply walk out", "leave"),
   never a sneak or a bribe: when there is none, the player chooses */
static int leaving; static u32 leaving_t;
extern "C" int citynav_walking(void) { return goal >= 0 || leaving; }   /* (the pointer's hourglass: ui/overlay_text.cpp) */
extern "C" void citynav_go(int dest);
static bool walk_out_option(const std::string &t)
{
    if (t.find("sneak") != std::string::npos || t.find("hide") != std::string::npos || t.find("bribe") != std::string::npos || t.find("saint") != std::string::npos
        || t.find("alchem") != std::string::npos || t.find("fight") != std::string::npos || t.find("attack") != std::string::npos) return false;
    return t.find("走出去") != std::string::npos || t.find("离开城市") != std::string::npos || t.find("离开小镇") != std::string::npos
        || t.find("离开城外") != std::string::npos || t.find("骑马离开") != std::string::npos;
}
static void leaving_tick(void)
{
    u32 now = plat_ticks_ms();
    if (game_mode() == GM_MAP) { leaving = 0; return; }                        /* out: the journey begins (mods/travel.c) */
    if (now - leaving_t > 20000 || !in_city()) { leaving = 0; return; }
    if (goal >= 0 || now - last_act < 300) return;                             /* (still walking to the gate) */
    if (!assist_in_card_menu()) { std::string c = card_now(); if (hub_of(c) == H_NONE && now - phase_t > 300) { plat_click_game(304, 12); last_act = now; } return; }
    if (g_rb(GV_card_box_shown)) return;
    std::string card = card_now();
    int blk = (s16)g_rw(GV_shown_block);
    std::vector<std::string> blocks = card_blocks(card);
    if (blk < 0 || blk >= (int)blocks.size()) return;
    std::vector<std::string> opts = block_options(blocks[blk]);
    int n = (s8)g_rb(GV_card_layout);
    if (n <= 0 || n > 20) return;
    for (int i = 0; i < n; i++) {
        int o = (s8)g_rb((u16)(GV_card_layout_opt + i));
        if (o >= 0 && o < (int)opts.size() && walk_out_option(opts[o])) { LOG("leaving: %s\n", opts[o].c_str()); click_line(i); leaving = 0; return; }
    }
    leaving = 0;
    ui_notice("大门：选择如何离开，旅程将在陆地视图中开始。");
}
static int leaving_flag(int v) { int o = leaving; if (v >= 0) leaving = v; return o; }
extern "C" void citynav_leave_town(void)
{
    citynav_go(D_LEAVE);
    leaving = 1; leaving_t = plat_ticks_ms();
}

extern "C" void citynav_tick(void)
{
    hdtext_fast_fades = goal >= 0 || leaving;              /* the cards' fades at once while walking */
    if (leaving && goal < 0) { if (!cards_loaded) load_cards(); leaving_tick(); return; }
    if (goal < 0) return;
    { static int gen = -1; int datamods_generation(void); if (gen != datamods_generation()) { gen = datamods_generation(); cards.clear(); cards_loaded = false; } }
    if (!cards_loaded) load_cards();
    u32 now = plat_ticks_ms();
    if (now - last_act < 250) return;                      /* let the game take the last click */
    if (!in_city()) { stop(game_mode() == GM_CARD ? "已离开城市。" : "interrupted."); return; }
    if (steps_taken > 20) { stop("步骤太多；已停止。"); return; }
    std::string card = card_now();
    int blk = (s16)g_rw(GV_shown_block);
    char kb[48]; snprintf(kb, sizeof kb, "%s/%d/%d", card.c_str(), blk, assist_in_card_menu());
    if (phase_key != kb) { phase_key = kb; phase_t = now; }
    int hub = hub_of(card);
    if (!assist_in_card_menu()) {                        /* the card's text: read on (hubs, and the place the walk starts from) */
        if (now - phase_t < 300) return;
        if (hub == H_NONE) {
            if (visited || conts >= 16) { stop(steps_taken ? "路上发生了些事。" : "先离开这个地方。"); return; }   /* (the Quickstart's inn card has six pages; a slow game lets clicks pass unread) */
            conts++;
        }
        LOG("%s: read on\n", kb);
        plat_click_game(304, 12); last_act = now;
        return;
    }
    if (now - phase_t < 120) return;                       /* the option list fading in (at once while walking) */
    if (kb == last_key) { if (++same_count > 3) { stop("路被堵住了。"); return; } } else { last_key = kb; same_count = 0; }
    /* the card's text as the game loaded it (a mod's card, a restored one, the game's own: what is on
       show), else as MSGFILES has it */
    std::vector<std::string> blocks = card_blocks(card);
    if (blk < 0 || blk >= (int)blocks.size()) {
        LOG("%s: its option block %d not among the card's %d\n", kb, blk, (int)blocks.size());
        stop("在这张卡片上找不到出路。"); return;
    }
    std::vector<std::string> opts = block_options(blocks[blk]);
    int n = (s8)g_rb(GV_card_layout);
    if (n <= 0 || n > 20) return;
    if (g_rb(GV_card_box_shown)) return;                              /* a skill-check box is up */
    std::vector<std::string> line(n);
    for (int i = 0; i < n; i++) { int o = (s8)g_rb((u16)(GV_card_layout_opt + i)); line[i] = o >= 0 && o < (int)opts.size() ? opts[o] : ""; }
    { LOG("%s hub %d visited %x, line %d of %d:\n", kb, hub, visited, (int)g_rb(GV_card_layout_opt), (int)blocks.size()); for (int i = 0; i < n; i++) LOG("  %d: %s -> %d\n", i, line[i].c_str(), hub >= 0 ? rule_for(hub, line[i]) : -1); }
    if (card == "$INSID00" && blk == 0) {                   /* the offices above the marketplace by night: the one wanted, else out by the window */
        static const struct { int d; const char *key; } OFF[] = { { D_MERCHANT, "领头商人" }, { D_FUGGER, "fugger" }, { D_MEDICI, "medici" }, { D_HANSE, "hanseatic" } };
        for (auto &o : OFF) if (o.d == goal) for (int i = 0; i < n; i++) if (line[i].find(o.key) != std::string::npos) { click_line(i); goal = -1; return; }
        for (int i = 0; i < n; i++) if (line[i].find("window") != std::string::npos) { leaves++; click_line(i); return; }
    }
    if (hub == H_NONE) {                                   /* a shop or other place of the city: out the way the player would leave */
        if (visited || leaves >= 3) { stop("路上发生了些事。"); return; }
        int pick = choose_leave(line);
        if (pick < 0) { stop(leaves ? "路上发生了些事。" : "先离开这个地方。"); return; }
        leaves++; click_line(pick);
        return;
    }
    if (HUB_PLACE[hub] == goal) { LOG("arrived\n"); goal = -1; return; }   /* the square itself */
    for (int i = 0; i < n; i++) if (rule_for(hub, line[i]) == goal) { click_line(i); arrived_last = 1; goal = -1; return; }   /* there */
    bool market_goal = false;                                /* (a place of the marketplace itself: elsewhere, the way goes on through it) */
    for (const Rule &r : RULES) if (r.hub == H_MARKET && r.to == goal) market_goal = true;
    if (hub == H_MARKET && (night_closed(goal) || (card == "$MARKE01" && market_goal))) {    /* the marketplace by night ($MARKE01): its stalls shut, the offices upstairs to be reached past the guards */
        bool office = goal == D_MERCHANT || goal == D_FUGGER || goal == D_MEDICI || goal == D_HANSE;
        stop(office ? "夜晚的集市：选择如何越过守卫，前往楼上的办公室（在那里再次按G即可进入办公室）。"
                    : "夜晚的集市：摊位要到5点才开门。");
        return;
    }
    visited |= 1u << hub;
    int best = -1, bd = 1000;
    for (int i = 0; i < n; i++) {
        int to = rule_for(hub, line[i]);
        if (to < 100 || (visited >> (to - 100) & 1)) continue;
        int d = dist_to(goal, to - 100, visited) + (to - 100 == H_SIDE ? 3 : 1);
        if (d < bd) { bd = d; best = i; }
    }
    if ((best < 0 || bd >= 1000) && visited != (1u << hub) && ++replans <= 2) {   /* (set back on the way, as when guards turn the party away: the hubs passed are open again) */
        LOG("no way on through new hubs: the way planned anew\n");
        visited = 1u << hub;
        for (int i = 0; i < n; i++) {
            int to = rule_for(hub, line[i]);
            if (to < 100 || to - 100 == hub) continue;
            int d = dist_to(goal, to - 100, visited) + (to - 100 == H_SIDE ? 3 : 1);
            if (d < bd) { bd = d; best = i; }
        }
    }
    if (best < 0 || bd >= 1000) { stop("在这座城市里找不到。"); return; }
    click_line(best);
}

extern "C" void citynav_go(int dest)
{
    if (dest < 0 || dest >= D_N) return;
    assist_watch_cards();
    trail.clear(); LOG("go to %s from %s (place %d)\n", DEST[dest].label, card_now().c_str(), (s16)g_rw(G_CUR_LOC));
    goal = dest; steps_taken = 0; visited = 0; conts = leaves = replans = 0; arrived_last = 0; same_count = 0; last_key.clear(); phase_key.clear(); last_act = 0;
    char m[120]; snprintf(m, sizeof m, "前往：%s。按任意键或点击以停止。", DEST[dest].label); ui_notice(m);
}

/* while Go to walks the party through the streets, the cards it passes are not shown (each was up
   for a moment: they read as flashes): the field is covered by a page saying where the party
   goes, the place's picture and footsteps along a dotted way, until it arrives (or stops) */
extern "C" void plat_game_to_win(float gx, float gy, float *wx, float *wy);
extern "C" void citynav_draw_cover(void)
{
    static u32 shown_from, ended_t; static int last_goal = -1, tail_goal = -1; static std::string tail_card;
    u32 now = plat_ticks_ms();
    int g = goal >= 0 ? goal : leaving ? D_LEAVE : -1;
    if (g >= 0 && (steps_taken || leaving)) tail_goal = -1;
    if (g < 0 && last_goal >= 0 && arrived_last && tail_goal < 0 && now - ended_t > 3000) {          /* (arrived: the last street's card stays until the place's own comes) */
        tail_goal = last_goal; tail_card = card_now(); ended_t = now;
    }
    if (g < 0 && tail_goal >= 0) {
        if (card_now() != tail_card || now - ended_t > 2500 || game_mode() != GM_CARD) tail_goal = -1;
        else g = tail_goal;
    }
    if (g < 0 || !config_get_int("assist.city_goto_cover", 1)) { last_goal = -1; return; }
    if (g != last_goal) { if (tail_goal < 0) shown_from = now; last_goal = g; }
    if (!steps_taken && !leaving && tail_goal < 0) return;   /* (not yet under way: the card it starts from) */
    ImDrawList *dl = ImGui::GetForegroundDrawList();
    float x0, y0, x1, y1; plat_game_to_win(60, 0, &x0, &y0); plat_game_to_win(320, 200, &x1, &y1);
    float w = x1 - x0, h = y1 - y0, k = h / 200.0f;
    dl->AddRectFilledMultiColor(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(236, 222, 188, 255), IM_COL32(230, 214, 176, 255), IM_COL32(214, 194, 152, 255), IM_COL32(222, 204, 164, 255));
    if (orn_on()) { orn_parchment_edges(dl, ImVec2(x0 + 2 * k, y0 + 2 * k), ImVec2(x1 - 2 * k, y1 - 2 * k), 6 * k); orn_frame(dl, ImVec2(x0, y0), ImVec2(x1, y1), 1.9f * k); }
    ImVec2 c(x0 + w * 0.5f, y0 + h * 0.42f);
    ui_draw_town_icon(dl, c, 46 * k, DEST[g].icon);
    char t[96]; snprintf(t, sizeof t, g == D_LEAVE ? "步行前往城门" : "步行前往：%s", DEST[g].label);
    ImFont *f = ImGui::GetFont(); float fs = 9.0f * k;
    ImVec2 ts = f->CalcTextSizeA(fs, FLT_MAX, 0, t);
    dl->AddText(f, fs, ImVec2(c.x - ts.x * 0.5f, c.y + 30 * k), IM_COL32(51, 31, 15, 255), t);
    /* footsteps along a dotted way under it, walking on */
    float py = c.y + 48 * k, px0 = c.x - 60 * k, px1 = c.x + 60 * k;
    for (float x = px0; x <= px1; x += 4 * k) dl->AddCircleFilled(ImVec2(x, py), 0.7f * k, IM_COL32(120, 90, 60, 160), 6);
    float tt = (now - shown_from) / 1000.0f;
    for (int i = 0; i < 4; i++) {
        float ph = fmodf(tt * 0.6f + i * 0.25f, 1.0f), x = px0 + (px1 - px0) * ph, a = sinf(ph * 3.14159f);
        float oy = (i & 1 ? -2.2f : 2.2f) * k;
        dl->AddEllipseFilled(ImVec2(x, py + oy), ImVec2(2.4f * k, 1.3f * k), IM_COL32(80, 50, 26, (int)(220 * a)), 0, 12);
    }
    const char *sub = "按任意键或点击即可停止步行。";
    float ss = 5.5f * k; ImVec2 s2 = f->CalcTextSizeA(ss, FLT_MAX, 0, sub);
    dl->AddText(f, ss, ImVec2(c.x - s2.x * 0.5f, py + 10 * k), IM_COL32(110, 84, 56, 255), sub);
}

/* ---------------------------------------------------------------- the list */
/* Whether the city's cards offer a place, as the card scripts decide (s045..s050):
 * - a building bit of DARKLAND.CTY or a shop quality;
 * - the Fugger, Medici and Hanse offices: bytes +0x15..+0x17 of the location record;
 * - Soldier's Road: byte +0x14 of the location record is 1 or 2 (rarely so);
 * - the physician and Astrologists' Lane: in small cities only some games have them
 *   (from the city's seed, the game's seed and, for the physician, the year);
 * - the hospital and Jewelers' Avenue are never offered (their options stay switched off),
 *   nor the herbalists and barber-surgeons ($CivCr01 and $Barbe00 are unused cards);
 * - Clothmakers' Street by day ($Cloth00) has nothing on it but "leave" in 483.07 (its shopfronts
 *   switched off by mistake: the "cardopts" fix opens them); without the fix it is not listed, a
 *   walk there would be for nothing (at night a sleeper woken there sells, for reputation). */
static bool never(int d)
{
    if (d == D_CLOTH) return !fix_is_on("cardopts");
    if (d == D_HOSPITAL) return !cutcards_hospice_on();          /* (by day: the restored hospice, mods/cutcards.c) */
    return d == D_JEWEL || d == D_HERBAL;   /* (the herbalists' road is only on $CivCr01, a card the game never loads) */
}
static bool present_at(int d, int cur)
{
    if (never(d)) return false;
    if (cur < 0 || cur >= (int)gamedata.cities.size()) return true;
    const City &c = gamedata.cities[cur];
    if (DEST[d].building >= 0 && !(c.buildings >> DEST[d].building & 1)) return false;
    if (DEST[d].shop >= 0 && !c.shopq[DEST[d].shop]) return false;
    if (DEST[d].shop == -2 && !c.shopq[4] && !c.shopq[5]) return false;
    u32 lr = g_location(cur);
    if (lr) {
        u16 ls = (u16)(lr >> 16), lo = (u16)lr;
        if (d == D_SOLDIERS) { int v = (s8)rb(ls, (u16)(lo + 0x14)); if (v != 1 && v != 2) return false; }
        if (d == D_FUGGER && !rb(ls, (u16)(lo + 0x15))) return false;
        if (d == D_MEDICI && !rb(ls, (u16)(lo + 0x16))) return false;
        if (d == D_HANSE && !rb(ls, (u16)(lo + 0x17))) return false;
    }
    s16 q = (s16)town_seed(cur);                                 /* loc_query 0x21 (16-bit arithmetic, as the game) */
    if (d == D_PHYSICIAN && c.size <= 3 && !((s16)(q + (s16)g_rw(G_CUR_YEAR)) % 3)) return false;
    if (d == D_ALCHEMIST && ((c.size <= 3 && !(q % 2)) || (c.size == 4 && !(q % 3)))) return false;
    return true;
}
static bool present(int d) { return present_at(d, (s16)g_rw(G_CUR_LOC)); }
extern "C" void citynav_test_places(void)                  /* (test ugotoplaces: the places of this town, as Go to's list greys them) */
{
    fprintf(stderr, "gotoplaces:");
    for (int d = 0; d < D_N; d++) if (present(d)) fprintf(stderr, " %d", d);
    fprintf(stderr, "\n");
}
/* the marketplace by night ($Marke01, is_daylight: 5:00 to 18:59) offers no stall or office, only a
   sneak upstairs: the places reached through it are shut until 5:00 */
static bool is_night(void) { int h = (s16)g_rw(GV_cur_hour); return h >= 19 || h < 5; }
static bool night_closed(int d)
{
    switch (d) { case D_MERCHANT: case D_FOREIGN: case D_PHARMACIST: case D_PAWN: case D_FUGGER: case D_MEDICI: case D_HANSE: return is_night(); }
    return false;
}

/* what town loc offers that is worth knowing (ui/townicons.h TI_UNIVERSITY .. TI_BANK), for the
   margin: as the city's cards decide (the location record is read: while a card or the map is up) */
bool citynav_town_offers(int loc, int icon)
{
    if (loc < 0 || loc >= (int)gamedata.cities.size()) return false;
    switch (icon) {
    case TI_UNIVERSITY: return present_at(D_UNIVERSITY, loc);
    case TI_CATHEDRAL: return present_at(D_CATHEDRAL, loc);
    case TI_ALCHEMIST: return present_at(D_ALCHEMIST, loc);
    case TI_PHYSICIAN: return present_at(D_PHYSICIAN, loc);
    case TI_DOCKS: return present_at(D_DOCKS, loc);
    case TI_BANK: return present_at(D_FUGGER, loc) || present_at(D_MEDICI, loc);
    }
    return false;
}

/* test (ucmd utownoffers): how many towns have each of the places the town icons mark */
extern "C" void citynav_offer_counts(void)
{
    static const char *NM[6] = { "university", "cathedral", "alchemist", "physician", "docks", "bank" };
    int n = (int)gamedata.cities.size();
    for (int k = 0; k < 6; k++) { int c = 0; for (int l = 0; l < n; l++) c += citynav_town_offers(l, k); fprintf(stderr, "town offers: %s in %d of %d towns\n", NM[k], c, n); }
}

/* the party's tasks in this town, by the place they lead to: a reward waiting with its giver (the
   giver's house: a bank's, the Hanse's, the cathedral, the council), a thing kept in an office above
   the marketplace (reached at night) */
struct DestTask { int cat; std::string title, text; };
static std::map<int, std::vector<DestTask>> dest_tasks(int town)
{
    std::map<int, std::vector<DestTask>> m;
    std::vector<QuestRow> rows; quest_rows(rows, QSORT_DEADLINE);
    for (const QuestRow &r : rows) {
        if (r.loc != town) continue;
        int d = -1;
        switch (r.giver) {     /* (where the giver's card looks for the party's return, ev24_at_owner: $Medic00 6, $Hanse00 7, $Fugge00 8,
                                  $Cathe01 3, $Counc00 10; a merchant's, a foreign trader's, a pharmacist's at the marketplace itself) */
        case 6: d = D_MEDICI; break; case 7: d = D_HANSE; break; case 8: d = D_FUGGER; break; case 3: d = D_CATHEDRAL; break; case 10: d = D_TOWNHALL; break;
        }
        if (r.office >= 0) { static const int OD[4] = { D_MERCHANT, D_MEDICI, D_FUGGER, D_HANSE }; d = OD[r.office & 3]; }
        if (d >= 0) m[d].push_back({ r.cat, r.title, r.text });
    }
    return m;
}

/* the options of the card on show that lead to one of the party's tasks in this town, for the card
   view (ui/cardview.cpp: the task's picture at the option's end, the task under the pointer).
   A reward waits with its giver: at a house (the banks, the Hanse, the cathedral, the council, the
   fortress without one) its own option for the reward; at the marketplace (a merchant, a foreign
   trader, a pharmacist) the line to the stall. A thing to get from an office above the marketplace:
   by night, the market's ways upstairs, then the office. On the streets' hubs, the line to the
   place, or to a hub whose own lines lead there. */
static const char *HOUSE_CARD[D_N];
static std::vector<CardTask> option_tasks_now(void)
{
    std::vector<CardTask> out;
    int town = (s16)g_rw(G_CUR_LOC);
    if (!in_city() || town < 0 || town >= (int)gamedata.cities.size()) return out;
    struct Want { int dest, office; CardTask t; };
    std::vector<Want> w;
    bool council = gamedata.cities[town].buildings & 2;
    std::vector<QuestRow> rows; quest_rows(rows, QSORT_DEADLINE);
    for (const QuestRow &r : rows) {
        if (r.loc != town) continue;
        CardTask t; t.opt = -1; t.cat = r.cat; t.title = r.title; t.text = r.text;
        if (r.office >= 0) { if (is_night()) w.push_back({ -1, r.office & 3, t }); continue; }
        int d = -1;
        switch (r.giver) {
        case 0: d = D_MERCHANT; break; case 4: d = D_FOREIGN; break; case 5: d = D_PHARMACIST; break;
        case 6: d = D_MEDICI; break; case 7: d = D_HANSE; break; case 8: d = D_FUGGER; break; case 3: d = D_CATHEDRAL; break;
        case 10: d = council ? D_TOWNHALL : fix_is_on("lordreward") ? D_FORTRESS : -1; break;
        }
        if (d >= 0 && !night_closed(d)) w.push_back({ d, -1, t });
    }
    if (w.empty()) return out;
    std::string card = card_now();
    int blk = (s16)g_rw(GV_shown_block);
    std::vector<std::string> blocks = live_blocks();
    if (blocks.empty()) { if (!cards_loaded) load_cards(); auto it = cards.find(card); if (it != cards.end()) blocks = it->second; }
    if (blk < 0 || blk >= (int)blocks.size()) return out;
    std::vector<std::string> opts = block_options(blocks[blk]);
    int hub = hub_of(card);
    static const char *OFFICE[4] = { "领头商人", "medici", "fugger", "hanseatic" };   /* (papers_office: 0 the leading merchant, 1 the Medici, 2 the Fugger, 3 the Hanse) */
    auto leads = [&](int to, const Want &x) {                 /* a hub's line `to` leads to the task's place (directly, or by its next hub) */
        if (x.dest >= 0 && to == x.dest) return true;
        if (to < 100) return false;
        for (const Rule &r : RULES) if (r.hub == to - 100 && x.dest >= 0 && r.to == x.dest) return true;
        return x.office >= 0 && to == 100 + H_MARKET;
    };
    for (int o = 0; o < (int)opts.size(); o++) {
        const std::string &t = opts[o];
        for (const Want &x : w) {
            bool yes = false;
            if (x.office >= 0 && hub == H_MARKET) yes = t.find("upstairs") != std::string::npos || t.find("避开它们") != std::string::npos;   /* ($Marke01: the saint takes them up too) */
            else if (x.office >= 0 && !strncmp(card.c_str(), "$INSID", 6)) yes = t.find(OFFICE[x.office]) != std::string::npos;
            else if (hub != H_NONE) { int to = rule_for(hub, t); yes = to >= 0 && leads(to, x); }
            else if (x.dest >= 0 && HOUSE_CARD[x.dest] && !strncmp(card.c_str(), HOUSE_CARD[x.dest], 6))
                yes = t.find(x.dest == D_TOWNHALL || x.dest == D_FORTRESS ? "请求觐见" : "reward") != std::string::npos;
            if (yes) { CardTask c = x.t; c.opt = o; out.push_back(c); }
        }
    }
    return out;
}
std::vector<CardTask> citynav_option_tasks(void)
{
    static std::vector<CardTask> v; static int frame = -100; static std::string key;
    std::string k = card_now() + "/" + std::to_string((s16)g_rw(GV_shown_block));
    if (k != key || ImGui::GetFrameCount() - frame > 30) {
        if (!HOUSE_CARD[D_FUGGER]) { HOUSE_CARD[D_FUGGER] = "$FUGGE"; HOUSE_CARD[D_MEDICI] = "$MEDIC"; HOUSE_CARD[D_HANSE] = "$HANSE";
                                     HOUSE_CARD[D_CATHEDRAL] = "$CATHE"; HOUSE_CARD[D_TOWNHALL] = "$COUNC"; HOUSE_CARD[D_FORTRESS] = "$CITYF"; }
        v = option_tasks_now(); key = k; frame = ImGui::GetFrameCount();
    }
    return v;
}

extern "C" const char *journal_town_fact(int loc, const char *key);   /* ui/journal.cpp */
extern ImFont *ui_font_bold;

/* text cut to fit `w` pixels at font size `fs`, with "..." when cut */
static std::string fit(const std::string &t, float fs, float w)
{
    ImFont *f = ImGui::GetFont();
    if (f->CalcTextSizeA(fs, FLT_MAX, 0, t.c_str()).x <= w) return t;
    std::string r = t;
    while (!r.empty() && f->CalcTextSizeA(fs, FLT_MAX, 0, (r + "...").c_str()).x > w) r.pop_back();
    while (!r.empty() && r.back() == ' ') r.pop_back();
    return r + "...";
}

/* a place's tooltip: its name, what it is, then what is known of it now, a heading to each part */
struct TipPart { std::string head, text; };
/* the pane at the right of Go to: the place under the pointer (or the last one pointed at) - its
   name, what it is, then what is known of it now, a heading to each part. It stays put, where a
   tooltip followed the pointer over the tiles */
static int pane_d = -1; static std::vector<TipPart> pane_parts; static bool pane_absent;
static void place_pane(float w, float h, float scale)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(236, 222, 192, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6 * scale);
    ImGui::BeginChild("detail", ImVec2(w, h), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PushTextWrapPos(0);
    if (pane_d < 0) ImGui::TextDisabled("指向某个地点，即可在此处查看其介绍：提供的服务、任务、导师等。");
    else {
        if (ui_font_bold) ImGui::PushFont(ui_font_bold);
        ImGui::TextUnformatted(DEST[pane_d].label);
        if (ui_font_bold) ImGui::PopFont();
        if (DEST[pane_d].desc[0]) ui_rich_text(rich_auto(DEST[pane_d].desc).c_str());   /* (the skills and attributes it names as their shields) */
        for (const TipPart &x : pane_parts) {
            ImGui::Spacing();
            if (!x.head.empty()) ImGui::TextColored(ImVec4(0.55f, 0.24f, 0.13f, 1), "%s", x.head.c_str());
            ui_rich_text(x.text.c_str());
        }
        if (!pane_absent) { ImGui::Spacing(); ImGui::TextDisabled("点击该格子，队伍便会前往那里。"); }
    }
    ImGui::PopTextWrapPos();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

/* the "Go to" window; returns false when it closes.
 * Each place is a tile: its picture, its name, and under the name a line for each thing worth
 * knowing now - a task giver's chance of a task (or until when none, a reward waiting), the
 * party's own task there, a teacher to hire, a shop's wares, what the party learned of the
 * physician or the alchemists. A tile is as tall as its row needs. A lectern beside the name marks
 * a teacher, a task's picture the place a task of the party's leads to; the whole of each is under
 * the pointer. A place the town lacks keeps its tile, greyed, so every place is always where it was. */
struct TileLine { std::string text; ImU32 col; };
struct Tile { int d; std::vector<TileLine> lines; std::vector<TipPart> tip; int teach; int task_cat; bool teach_off; bool here; };

extern "C" int citynav_window(void)
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, 90));
    float w = fminf(io.DisplaySize.x * 0.94f, UIS(1250)), maxh = io.DisplaySize.y * 0.94f;
    ImGui::SetNextWindowSizeConstraints(ImVec2(w, 0), ImVec2(w, maxh));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    bool keep = true;
    int cur = (s16)g_rw(G_CUR_LOC);
    bool city = cur >= 0 && cur < (int)gamedata.cities.size();
    std::string title = city ? "前往：" + gamedata.cities[cur].name : std::string("前往");
    title += "###citynav";
    int picked = -1;
    float scale = fminf(1.0f, fmaxf(0.7f, io.DisplaySize.y / 900.0f));
    static bool tall;                                        /* (the list did not fit last frame: a scroll bar) */
    if (ImGui::Begin(title.c_str(), &keep, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
                     | (tall ? 0 : ImGuiWindowFlags_NoScrollbar))) {
        tall = ImGui::GetWindowHeight() >= maxh - 2;
        ImGui::SetWindowFontScale(scale);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8 * scale, 6 * scale));
        ImGui::PushTextWrapPos(0);
        ImGui::TextDisabled("选择一个地点，队伍便会穿过街道前往。按任意键或点击即可停止。");
        { int hour = (s16)g_rw(GV_cur_hour);    /* (the game's night: is_daylight, 5:00 to 18:59) */
          if (hour >= 19 || hour < 5) ImGui::TextColored(ImVec4(0.45f, 0.18f, 0.10f, 1), "夜间（%d:00）：集市的摊位和商铺均已关闭，守卫已上岗。", hour); }
        ImGui::PopTextWrapPos();
        static std::map<int, std::vector<DestTask>> tasks; static int tasks_frame = -100;
        static TchSource teach[12]; static int nteach;            /* the teachers the town offers (ui/teachersui.h) */
        if (ImGui::GetFrameCount() - tasks_frame > 30) {
            tasks = dest_tasks(cur); tasks_frame = ImGui::GetFrameCount();   /* (read twice a second) */
            nteach = ui_teachers_on() ? ui_town_teachers(cur, teach, 12) : 0;
        }
        bool work_on = config_get_int("assist.goto_work", 1) && !config_get_int("original", 0);
        float lh = ImGui::GetFontSize(), sfs = lh * 0.8f, slh = sfs * 1.18f, pad = 6 * scale, rnd = 6 * scale;
        float is = floorf(lh * 2.1f);                              /* the place's picture */
        float gap = ImGui::GetStyle().ItemSpacing.x;
        float pane_w = floorf(fminf(UIS(330) * scale, ImGui::GetContentRegionAvail().x * 0.3f));
        float tiles_maxh = fmaxf(UIS(200), maxh - ImGui::GetCursorPosY() - ImGui::GetStyle().WindowPadding.y - 4);
        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, tiles_maxh));
        ImGui::BeginChild("tiles", ImVec2(ImGui::GetContentRegionAvail().x - pane_w - gap, 0), ImGuiChildFlags_AutoResizeY);
        int hov_now = -1; bool hov_now_absent = false; std::vector<TipPart> hov_now_parts;
        float avail = ImGui::GetContentRegionAvail().x;
        int cols = (int)((avail + gap) / (lh * 12.0f + gap));      /* (a tile about 12 lines of type wide: its lines fit) */
        cols = cols < 3 ? 3 : cols > 5 ? 5 : cols;
        float bw = floorf((avail - (cols - 1) * gap) / cols);
        static const ImU32 GREEN = IM_COL32(38, 100, 34, 255), BROWN = IM_COL32(104, 82, 58, 255), BLUE = IM_COL32(40, 70, 140, 255),
                           GOLDD = IM_COL32(150, 96, 0, 255), REDD = IM_COL32(150, 40, 25, 255), GREY = IM_COL32(140, 124, 104, 255);

        /* what each place's tile says */
        auto make_tile = [&](int d) {
            Tile T{ d, {}, {}, -1, -1, false, true };
            if (!present(d)) { T.here = false; return T; }          /* (a place the town lacks: its name greyed, no more) */
            if (night_closed(d)) { T.lines.push_back({ "直至5:00才开放", REDD }); T.tip.push_back({ "", "夜间关闭：集市的摊位和商铺将于5:00重新开放。前往此处时，队伍仍会直接抵达集市。" }); }
            {   /* a refusal that lasts a while */
                struct Ban { int d, kind, type0; const char *line, *tip; };
                static const Ban BANS[] = {
                    { D_ALCHEMIST, 0x44, -1, "直至%d %s才可进入", "炼金术士拒绝了队伍；他们将于%d %s，%d:00再次接待您。" },
                    { D_MONASTERY, 0x34, -1, "图书馆直至%d %s关闭", "僧侣的图书馆（学习圣人知识的地方）对队伍关闭，直至%d %s，%d:00。" },
                    { D_MONASTERY, 0x30, -1, "从%d %s起再次祈祷", "队伍曾与僧侣一同祈祷；他们将于%d %s，%d:00再次开启祈祷。" },
                    { D_UNIVERSITY, 0x4e, 0, "图书馆直至%d %s关闭", "大学的图书馆（学习圣人知识的地方）对队伍关闭，直至%d %s，%d:00。" },
                };
                for (const Ban &B : BANS) {
                    int e[4];
                    if (B.d != d || !town_event_end(cur, B.kind, B.type0, e)) continue;
                    const char *mo = gm_month_short((e[2] % 12 + 12) % 12);
                    char b[96], t[200]; snprintf(b, sizeof b, B.line, e[1], mo); snprintf(t, sizeof t, B.tip, e[1], mo, e[0]);
                    T.lines.push_back({ b, REDD }); T.tip.push_back({ "现在不行", t });
                }
            }
            /* a task giver: whether it has a task now, or until when none (mods/offices.h) */
            int giver = d == D_MERCHANT ? 0 : d == D_FOREIGN ? 4 : d == D_PHARMACIST ? 5 : d == D_MEDICI ? 6 : d == D_HANSE ? 7 : d == D_FUGGER ? 8
                      : d == D_CATHEDRAL ? 3 : d == D_TOWNHALL || d == D_FORTRESS ? 10 : -1;
            if (d == D_FORTRESS && city && (gamedata.cities[cur].buildings & 2)) giver = -1;   /* (a town with a council: the lord gives none at the fortress) */
            OfficeStatus os;
            if (giver >= 0 && work_on && office_status(cur, giver, d == D_TOWNHALL, &os)) {
                char b[64]; ImU32 c = BROWN;
                switch (os.state) {
                case OFS_ASK: snprintf(b, sizeof b, "任务几率%d%%", os.chance); c = GREEN;
                    if (giver == 0 || giver == 4 || giver == 5) T.tip.push_back({ "任务", "商人可能在您买卖物品后提供一项任务。" }); break;
                case OFS_SHUT: snprintf(b, sizeof b, "直至%d %s无任务", os.day, gm_month_short((os.month % 12 + 12) % 12)); break;
                case OFS_ON_TASK: snprintf(b, sizeof b, "正在为他们执行任务"); c = BLUE; break;
                case OFS_REWARD: snprintf(b, sizeof b, "您的奖励正在等待"); c = GOLDD; break;
                case OFS_BARRED: snprintf(b, sizeof b, "无任务：声望过低"); c = REDD; break;
                default: b[0] = 0; break;
                }
                if (b[0]) T.lines.push_back({ b, c });
                std::string n = os.note;
                if (os.state == OFS_SHUT) { char u[64]; snprintf(u, sizeof u, "直至%d %s，%d:00。", os.day, gm_month_short((os.month % 12 + 12) % 12), os.hour); n += u; }
                if (!n.empty()) T.tip.push_back({ "任务", n });
            }
            /* the party's own tasks that lead here */
            auto tk = tasks.find(d);
            if (tk != tasks.end() && !tk->second.empty()) {
                T.task_cat = tk->second[0].cat;
                for (const DestTask &x : tk->second) {
                    T.lines.push_back({ "您的任务：" + (x.title.empty() ? std::string("查看世界地图（M）") : x.title), BLUE });
                    T.tip.push_back({ "您的任务", x.text });
                }
            }
            /* a teacher to be had here (ui/teachersui.h) */
            int src = d == D_SWORD ? TCH_SWORDSMITHS : d == D_ARMOR ? TCH_ARMORERS : d == D_BOW ? TCH_BOWYERS : d == D_BLACKSMITH ? TCH_BLACKSMITHS
                    : d == D_TINKER ? TCH_ARTIFICERS : d == D_ALCHEMIST ? TCH_ALCHEMIST : d == D_PHYSICIAN ? TCH_PHYSICIAN
                    : d == D_MONASTERY ? TCH_MONASTERY : d == D_UNIVERSITY ? TCH_UNIVERSITY : -1;
            for (int t = 0; src >= 0 && t < nteach; t++) if (teach[t].src == src) T.teach = t;
            if (T.teach >= 0) {
                const TchSource &S = teach[T.teach];
                T.teach_off = S.off != 0;
                char b[48]; snprintf(b, sizeof b, fix_is_on("teachcap") ? "至%d" : "（技能%d）", S.strength);
                T.lines.push_back({ "教授" + ui_teacher_skills(S) + b, T.teach_off ? GREY : IM_COL32(32, 62, 120, 255) });   /* (greyed, with a faint lectern: not now) */
                T.tip.push_back({ "老师", ui_teacher_text(S) });
            }
            /* a shop: the quality of its wares */
            if (city && DEST[d].shop != -1) {
                const City &c = gamedata.cities[cur];
                int r = DEST[d].shop >= 0 ? c.shopq[DEST[d].shop] : c.shopq[5] ? c.shopq[5] : c.shopq[4];
                if (r > 0) {
                    int q = shop_wares_q(r);
                    char b[96]; snprintf(b, sizeof b, q >= 30 ? "商品%dq，优于大多数" : "商品%dq", q);
                    T.lines.push_back({ b, q >= 30 ? GREEN : BROWN });
                    snprintf(b, sizeof b, "商品%dq（大多数商店出售约25q）。", q);
                    T.tip.insert(T.tip.begin(), { "", b });
                }
            }
            /* what the party learned of the physician here */
            if (d == D_PHYSICIAN) {
                const char *fact = journal_town_fact(cur, "physician");
                if (fact) {
                    std::string f = fact, l = f;
                    size_t cl = l.find(':'); if (cl != std::string::npos) l.erase(cl);
                    if (!l.empty()) l[0] = (char)toupper((unsigned char)l[0]);
                    T.lines.push_back({ l, BROWN });
                    T.tip.push_back({ "您所学到的内容", "医生：" + f + "." });
                }
            }
            /* the Philosopher's Stone the alchemist or the university sells (always the same: town_stone_offer) */
            if (d == D_ALCHEMIST || d == D_UNIVERSITY) {
                int q = town_stone_offer(cur, d == D_UNIVERSITY), have = (s16)g_rw(GV_philosophers_stone);
                char b[80]; snprintf(b, sizeof b, q > have ? "哲学家之石%dq" : "石头%dq，品质并不更好", q);
                T.lines.push_back({ b, q > have ? GREEN : GREY });
                snprintf(b, sizeof b, have ? "一块质量%d的哲学家之石；队伍拥有的是%d。" : "一块品质为%d的贤者之石；队伍中没有。", q, have);
                T.tip.push_back({ "", b });
            }
            return T;
        };
        auto tile_h = [&](const Tile &T) { return fmaxf(is + 2 * pad, 2 * pad + lh + T.lines.size() * slh); };

        bool any_teach = false, any_task = false;
        for (int g = 0; g < 5; g++) {
            std::vector<Tile> tiles;
            for (int d : ORDER) {
                if (DEST[d].group != g || never(d)) continue;
                tiles.push_back(make_tile(d));
            }
            if (tiles.empty()) continue;
            orn_septext(GROUPS[g]);
            for (size_t r0 = 0; r0 < tiles.size(); r0 += cols) {            /* a row: as tall as its tallest tile */
                float th = 0;
                for (size_t k = r0; k < tiles.size() && k < r0 + cols; k++) th = fmaxf(th, tile_h(tiles[k]));
                th = floorf(th);
                for (size_t k = r0; k < tiles.size() && k < r0 + cols; k++) {
                    const Tile &T = tiles[k]; int d = T.d;
                    if (k > r0) ImGui::SameLine();
                    ImGui::PushID(d);
                    ImVec2 p = ImGui::GetCursorScreenPos();
                    if (ImGui::InvisibleButton("b", ImVec2(bw, th)) && T.here) picked = d;
                    bool hov = ImGui::IsItemHovered() && T.here, hov_absent = ImGui::IsItemHovered() && !T.here;
                    ImDrawList *dl = ImGui::GetWindowDrawList();
                    dl->AddRectFilled(p, ImVec2(p.x + bw, p.y + th), hov ? IM_COL32(140, 61, 33, 255) : T.here ? IM_COL32(224, 204, 166, 255) : IM_COL32(232, 220, 196, 255), rnd);
                    if (!hov) dl->AddRect(p, ImVec2(p.x + bw, p.y + th), T.here ? IM_COL32(178, 148, 106, 255) : IM_COL32(205, 190, 164, 255), rnd);
                    ui_draw_town_icon(dl, ImVec2(p.x + pad + is * 0.5f, p.y + th * 0.5f), is, DEST[d].icon);
                    if (!T.here) dl->AddCircleFilled(ImVec2(p.x + pad + is * 0.5f, p.y + th * 0.5f), is * 0.5f + 1, IM_COL32(232, 220, 196, 165));   /* (the picture faded) */
                    float tx = p.x + pad * 2 + is, rx = p.x + bw - pad, ny = T.lines.empty() ? p.y + (th - lh) * 0.5f : p.y + pad;
                    float ms = lh * 1.15f;                              /* the marks beside the name */
                    if (T.task_cat >= 0) { any_task = true; ui_draw_quest_icon(dl, ImVec2(rx - ms * 0.5f, ny + lh * 0.5f), ms, T.task_cat); rx -= ms + 3 * scale; }
                    if (T.teach >= 0) {
                        any_teach = true;
                        int a = T.teach_off ? 110 : 255;
                        ui_draw_teach_mark(dl, ImVec2(rx - ms * 0.5f, ny + lh * 0.5f), ms * 0.95f,
                                           hov ? IM_COL32(252, 242, 220, a) : IM_COL32(80, 50, 24, a), hov ? IM_COL32(140, 61, 33, a) : IM_COL32(252, 246, 228, a));
                        rx -= ms + 3 * scale;
                    }
                    ImU32 fg = hov ? IM_COL32(252, 242, 220, 255) : T.here ? IM_COL32(51, 31, 15, 255) : IM_COL32(150, 134, 112, 255);
                    std::string lab = fit(DEST[d].label, lh, rx - tx - 2 * scale);
                    dl->AddText(ImVec2(tx, ny), fg, lab.c_str());
                    float ly = ny + lh + 1 * scale, room = p.x + bw - pad - tx;
                    for (const TileLine &L : T.lines) {
                        ImU32 lc = hov ? IM_COL32(240, 214, 180, 255) : L.col;
                        if (rich_has(L.text.c_str())) {                   /* (shields in it: cut at the tile's edge) */
                            dl->PushClipRect(ImVec2(tx, ly), ImVec2(tx + room, ly + slh), true);
                            rich_draw(dl, ImGui::GetFont(), sfs, ImVec2(tx, ly), 0, lc, L.text.c_str());
                            dl->PopClipRect();
                        } else dl->AddText(ImGui::GetFont(), sfs, ImVec2(tx, ly), lc, fit(L.text, sfs, room).c_str());
                        ly += slh;
                    }
                    if (hov) { hov_now = d; hov_now_parts = T.tip; }
                    else if (hov_absent) { hov_now = d; hov_now_absent = true; hov_now_parts = { { "", std::string("不在") + (city ? gamedata.cities[cur].name : std::string("这座城镇")) + "." } }; }
                    ImGui::PopID();
                }
            }
        }
        /* the foot: what the marks mean (those shown), and what this city lacks */
        if (any_teach || any_task) { ImGui::Spacing(); ImGui::Separator(); }
        if (any_teach || any_task) {
            ImDrawList *dl = ImGui::GetWindowDrawList();
            float ms = lh * 1.05f;
            auto mark = [&](int kind, const char *text) {
                ImVec2 a = ImGui::GetCursorScreenPos();
                ImGui::Dummy(ImVec2(ms, ms));
                if (kind) ui_draw_quest_icon(dl, ImVec2(a.x + ms * 0.5f, a.y + ms * 0.5f), ms, 0);
                else ui_draw_teach_mark(dl, ImVec2(a.x + ms * 0.5f, a.y + ms * 0.5f), ms * 0.9f, IM_COL32(80, 50, 24, 255), IM_COL32(252, 246, 228, 255));
                ImGui::SameLine(0, 4 * scale);
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("%s", text);
            };
            bool first = true;
            if (any_teach) { mark(0, "可雇佣的教师（微弱：目前不可用）"); first = false; }
            if (any_task) { if (!first) ImGui::SameLine(0, 24 * scale); mark(1, "你的某个任务通向此处"); }
        }
        ImGui::EndChild();
        float tiles_h = ImGui::GetItemRectSize().y;
        if (hov_now >= 0) { pane_d = hov_now; pane_parts = hov_now_parts; pane_absent = hov_now_absent; }
        ImGui::SameLine(0, gap);
        place_pane(pane_w, tiles_h, scale);
        ImGui::PopStyleVar();
    }
    ImGui::End();
    if (!keep || picked >= 0) pane_d = -1;
    if (picked >= 0) { citynav_go(picked); return 0; }
    return keep;
}

/* a town's buildings bits (DARKLAND.CTY +0x5e) and size, for the day jobs (mods/lodgeview.c); 0 if no town */
extern "C" int citynav_town_facts(int loc, int *size)
{
    if (loc < 0 || loc >= (int)gamedata.cities.size()) return 0;
    if (size) *size = gamedata.cities[loc].size;
    return gamedata.cities[loc].buildings;
}
