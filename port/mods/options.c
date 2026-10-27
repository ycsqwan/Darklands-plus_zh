/* options.c - the table of Darklands+'s switches and the presets (options.h). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../runtime/config.h"
#include "options.h"
#include "mods.h"
#include "fixes.h"

/* the modules' setters, for the switches whose value a module keeps in a variable */
void menuview_set(int), titleview_set(int), cardview_set(int), barterview_set(int), sheetview_set(int), lootview_set(int);
void alchemyview_set(int), cacheview_set(int), lodgeview_set(int), prayview_set(int), createview_set(int), rosterview_set(int);
void infoview_set(int), marchview_set(int), enemyview_set(int), mapmsg_set(int), savesview_set(int);
void assist_set_chances(int), assist_set_best_leader(int), assist_set_card_help(int), assist_set_card_keys(int);
void autopause_set(int, int), autopause_set_foes(int), battle_speed_set(int), battle_fast_set(int);
void battlescroll_set(int, int), pathfix_set(int, int), travel_set(int, int), ammo_set(int), thrown_set(int);
void saves_enable(int), hdtext_set(int), hdart_set(int), sfx_set(int);
extern int autopause_down, autopause_hurt, battlescroll_on, battlescroll_rest, pathfix_party, pathfix_enemies;
extern int travel_enabled, travel_fast, orders_click_attack, loot_return_thrown;
extern int saves_auto_arrive, saves_auto_battle, saves_restart;

static void ap_down(int v) { autopause_set(v, autopause_hurt); }
static void ap_hurt(int v) { autopause_set(autopause_down, v); }
static void bs_paced(int v) { battlescroll_set(v, battlescroll_rest); }
static void bs_rest(int v) { battlescroll_set(battlescroll_on, v); }
static void pf_set(int v) { pathfix_set(v, v); }
static void tr_auto(int v) { travel_set(v, travel_fast); }
static void tr_fast(int v) { travel_set(travel_enabled, v); }
static void click_attack(int v) { orders_click_attack = v; }
static void return_thrown(int v) { loot_return_thrown = v; }
static void fast_key(int v) { if (!v) battle_fast_set(0); }
static void auto_arrive(int v) { saves_auto_arrive = v; }
static void auto_battle(int v) { saves_auto_battle = v; }
static void restart_end(int v) { saves_restart = v; }

static const int V_CHANCES[] = { 2, 1, 0 };
static const int V_PICTURE[] = { 0, 1, 2, 3 };
static const int V_TIPDELAY[] = { 0, 500, 1000 };
static const int V_SPEED[] = { 100, 150, 200, 300 };
static const int V_PACE[] = { 0, 1, 2 };
static const int V_RATE[] = { 25, 50, 100, 200 };


/* groups: one switch for a set of features nobody would want one by one (a screen at a time, each
   battle helper alone ...: more to choose than anyone wants, and more ways for them to meet untested).
   The members keep their own keys, which the modules read, but are not shown: the group's switch
   sets them all, each to its Darklands+ value (on) or the original's (off). */
typedef struct { const char *key; const char *members[26]; } Bundle;
static const Bundle BUNDLES[] = {
    { "display.native_screens", { "display.native_panel", "display.shop_compare", "display.native_menu", "display.native_title",
      "display.native_cards", "display.native_shops", "display.native_sheet", "display.native_loot", "assist.loot_worth",
      "display.native_alchemy", "assist.where_sold", "display.native_cache", "display.native_lodge", "display.native_pray",
      "display.native_create", "assist.creation_guide", "display.native_roster", "display.native_info", "display.native_march",
      "display.native_enemyinfo", "display.native_mapmsg", "display.native_saves", 0 } },
    { "battle.controls", { "battle.click_attack", "battle.volley_toggle", "battle.panel_select", "battle.quick_orders", "battle.next_member",
      "battle.order_bar", "battle.order_icons", "battle.drag_pan", "battle.select_stays", "battle.end_words", "battle.fast_key", 0 } },
    { "battle.helpers", { "battle.info", "battle.los_line", "battle.click_hint", "battle.foe_arrow", "battle.skill_gains",
      "battle.reload_bar", "battle.log", 0 } },
    { "battle.pauses", { "battle.pause_down", "battle.pause_hurt", "battle.pause_foes", 0 } },
    { "battle.scrolling", { "battle.smooth_scroll", "battle.scroll_paced", "battle.scroll_rest", 0 } },
    { "assist.card_helpers", { "assist.chances", "assist.card_help", "assist.shop_info", "assist.fast_fades", "assist.card_keys",
      "assist.teachers", "assist.docks_map", 0 } },
    { "assist.endless", { "assist.infinite_ammo", "assist.infinite_thrown", 0 } },
    { "map.overland_marks", { "map.names", "map.quest_marks", 0 } },
};
#define NBUNDLE ((int)(sizeof BUNDLES / sizeof BUNDLES[0]))
static int bundle_of(const char *key)
{
    for (int g = 0; g < NBUNDLE; g++) for (int i = 0; BUNDLES[g].members[i]; i++) if (!strcmp(key, BUNDLES[g].members[i])) return g;
    return -1;
}
int option_bundled(const Option *o) { return bundle_of(o->key) >= 0; }
static void bundle_set(int g, int v)
{
    for (int i = 0; BUNDLES[g].members[i]; i++) { const Option *o = options_find(BUNDLES[g].members[i]); if (o) option_set(o, v ? o->def : o->orig); }
}
/* at start: the members as their group says (a config from before the groups: the group on unless
   its first member was set to the original's way) */
void options_sync_bundles(void)
{
    for (int g = 0; g < NBUNDLE; g++) {
        const Option *f = options_find(BUNDLES[g].members[0]);
        int v = config_get_int(BUNDLES[g].key, f ? config_get_int(f->key, f->def) != f->orig : 1);
        config_set_int(BUNDLES[g].key, v);
        for (int i = 0; BUNDLES[g].members[i]; i++) { const Option *o = options_find(BUNDLES[g].members[i]); if (o) config_set_int(o->key, v ? o->def : o->orig); }
    }
}

#define S_SCREENS "新屏幕"
#define S_CARDS   "卡牌与城镇"
#define S_PARTY   "队伍与物品"
#define S_BATTLE  "战斗"
#define S_TRAVEL  "旅行"
#define S_CONTROL "操作控制"
#define S_EDIT    "编辑工具"
#define S_OTHER   "其他"
#define S_EASYPLAY "更易上手"   /* offered as built-in mods (F12 > Mods), no preset's */
#define S_RESTORED "重建内容"   /* their own page, F12 > Reconstructed Content: off by default and in every preset */

/* key, section, parent, title, description, class, original, Darklands+, Easy, choices, values, apply, note, flags */
static Option OPTIONS[] = {
    { "display.native_screens", S_SCREENS, 0, "新屏幕", "这些屏幕由Darklands+全新绘制，延续原作风格：队伍面板、菜单、开始画面、卡牌、商店、角色表、战利品、炼金术、储物间、旅店与营地、祈祷、角色创建、队伍选择、队伍信息、行军顺序、敌人信息、陆地视图中的消息，以及存档与载入。它们显示更多内容（概率、物品价值、食材售卖地点等）；规则保持不变。关闭时则显示原版屏幕。", OC_SHOW, 0, 1, 1, 0, 0, 0, 0, 0, 0, bundle_set },
    { "display.shop_compare", S_SCREENS, 0, "商品与装备对比", "在商店中，武器、盔甲或盾牌旁若出现绿色箭头，表示其性能优于当前队员所装备的；红色箭头则表示性能较差（空箭头表示未装备任何物品）；指向某件商品时，会高亮显示其所替换的装备。", OC_SHOW, 0, 1, 1 },
    { "display.native_panel", S_SCREENS, 0, "新队伍面板", "每位队员的耐力、力量和神恩以带数字的标签条显示。点击操作与原版相同（打开角色表；战斗中下达指令）。若要添加个人肖像，请将NAME.png（或.jpg）文件放入Darklands+文件夹下的portraits子文件夹，NAME即队员姓名。", OC_SHOW, 0, 1, 1 },
    { "display.panel_picture", S_SCREENS, "display.native_screens", "队伍面板中的图片", "每位队员旁边", OC_TASTE, 0, 0, 0, "你的肖像，否则为战斗精灵，否则为徽章|战斗精灵，否则为徽章|徽章|无图片", V_PICTURE, 0, 0, OF_COMBO },
    { "display.tooltip_delay", S_SCREENS, 0, "工具提示延迟", "鼠标指针停留时显示的说明：指针停留在某物上多久后才会显示说明（一旦显示，下一条立即出现）。", OC_TASTE, 0, 500, 500, "立即|片刻之后|一秒钟之后", V_TIPDELAY, 0, 0, OF_COMBO },
    { "display.native_menu", S_SCREENS, 0, "新菜单", "游戏菜单（游戏、命令、攻击、队伍等）可通过右键或F10打开，并持续显示，直到你选择或点击其他地方；各项条目及其功能保持不变。关闭时则显示原版菜单栏，仅在按住右键时可见。", OC_SHOW, 0, 1, 1, 0, 0, menuview_set },
    { "display.native_title", S_SCREENS, 0, "新开始画面", "继续（加载最新存档）、载入、新建队伍和快速开始，均采用原版游戏的艺术风格。", OC_SHOW, 0, 1, 1, 0, 0, titleview_set },
    { "display.native_cards", S_SCREENS, 0, "新卡牌屏幕", "卡牌文字采用你选择的字体，选项以可点击的行呈现，每行旁附带相应概率；使用某物品或圣人的选项会列出适用者名单。游戏仍决定所有结果。关闭时则显示原版卡牌（文字清晰，叠加于其上）。", OC_SHOW, 0, 1, 1, 0, 0, cardview_set },
    { "display.native_shops", S_SCREENS, 0, "新商店屏幕", "所有商品与你的全部物品分列两栏，每行设有买卖按钮，已装备物品亦予标注；屏幕底部描述指针下方的内容。库存与价格保持不变。", OC_SHOW, 0, 1, 1, 0, 0, barterview_set },
    { "display.native_sheet", S_SCREENS, 0, "新角色面板", "纸娃娃与物品，包括装备、卸下、饮用和丢弃功能，所有技能均在同一页面，队伍中的圣人与配方。将物品拖拽至队伍栏中的成员身上即可装备（按住Shift可从堆叠中选择一件）。装备、赠送与饮用操作与原版相同。", OC_SHOW, 0, 1, 1, 0, 0, sheetview_set },
    { "inventory.shared", S_SCREENS, "display.native_screens", "队伍共享背包", "原版中每位成员各自携带物品；而此版本（仅限Darklands+）则由队伍共享一个背包。每位成员仅保留已装备的物品及自己的药水，其余物品均放入队伍背包。装备与饮用均从背包中获取，商店也从背包中采购。与原版一样，只有已装备的物品才计入负重。当游戏需要某位成员持有特定物品时（如开锁时的撬锁工具、骑乘坐骑、战斗前的箭矢），Darklands+会先将其从背包移至该成员手中。标记物品后，该物品将永不被出售或丢弃。", OC_PLUS, 0, 1, 1 },
    { "display.native_loot", S_SCREENS, 0, "新增战利品界面", "整个战利品堆与每位成员的物品分列两栏；一键即可移动整堆物品。原版中的G键与P键仍可逐件移动物品。", OC_SHOW, 0, 1, 1, 0, 0, lootview_set },
    { "assist.loot_worth", S_SCREENS, "display.native_loot", "战利品界面中的商店估价", "列表下方显示指针所指物品在商店中的大致售价。", OC_SHOW, 0, 1, 1 },
    { "display.native_alchemy", S_SCREENS, 0, "新增炼金术界面", "所有已知配方集中于一栏，配料触手可及，通过“-”与“+”设定批次，并显示成功率。成功率与酿造过程均保持不变。", OC_SHOW, 0, 1, 1, 0, 0, alchemyview_set },
    { "assist.where_sold", S_SCREENS, "display.native_alchemy", "缺少原料的购买地点", "当某种原料即将耗尽时，显示附近城镇中出售该原料的店铺。", OC_SHOW, 0, 1, 1 },
    { "display.native_cache", S_SCREENS, 0, "新增储物室界面", "所有储存物品与成员随身携带之物分列两栏，每行均有“取”与“放”选项（可单件或整堆），另有“收纳”按钮用于存放未装备的物品。关闭时：恢复原版界面，每次只能处理四件物品。", OC_SHOW, 0, 1, 1, 0, 0, cacheview_set },
    { "store.shared", S_SCREENS, "display.native_screens", "所有旅店共享同一储物室", "在某家旅店留下的物品将在其他旅店等待，如同某座宅邸的物品箱一般；最好不要追问原因（最多容纳100种物品）。已在多处城镇储存的物品，首次开启储物室时将被统一归集。", OC_PLUS, 0, 1, 1 },
    { "assist.inn_earn", S_CARDS, 0, "旅店中，休息后的成员可赚取金钱", "充满耐力的成员每日开始时，默认设置为“赚取少量金钱”，而非“仅放松”（后者只恢复耐力）。可在旅店界面随意更改。", OC_EASE, 0, 1, 1 },
    { "display.native_lodge", S_SCREENS, 0, "新增旅店与营地界面", "每位成员当日活动、所需消耗、教师及其费用，以及“直至恢复体力”。", OC_SHOW, 0, 1, 1, 0, 0, lodgeview_set },
    { "display.native_pray", S_SCREENS, 0, "新增祈祷界面", "圣像、圣人的事迹、可消耗的神恩及概率（按原版规则）。", OC_SHOW, 0, 1, 1, 0, 0, prayview_set },
    { "display.native_create", S_SCREENS, 0, "新增角色创建界面", "每一步骤均置于一页，注明童年或职业所能带来的属性（以及三十岁后的衰老）、每项技能的提升幅度与上限、获得的美德与圣人。规则保持不变。", OC_SHOW, 0, 1, 1, 0, 0, createview_set },
    { "assist.creation_guide", S_SCREENS, "display.native_create", "分步指南", "左侧空白栏内附各步骤简要说明。", OC_SHOW, 0, 1, 1 },
    { "display.native_roster", S_SCREENS, 0, "新增队伍选择界面", "四个队伍槽位及其对应图标、已保存的角色，以及所选角色的属性与技能；点击选择，双击添加或移除。", OC_SHOW, 0, 1, 1, 0, 0, rosterview_set },
    { "display.native_info", S_SCREENS, 0, "新增队伍信息界面", "名声与头衔、日期、地点、财富，以及队伍在各城镇的声誉（原版仅显示指针所在城镇）。", OC_SHOW, 0, 1, 1, 0, 0, infoview_set },
    { "display.native_march", S_SCREENS, 0, "新增行军顺序界面", "提供一份名单，用于上下调整队员顺序，完成后直接确认，无需逐一点击。", OC_SHOW, 0, 1, 1, 0, 0, marchview_set },
    { "display.native_enemyinfo", S_SCREENS, 0, "新增敌人信息界面", "战斗中的E：场上各类敌人的护甲与武器，以及己方武器对其的克制效果。", OC_SHOW, 0, 1, 1, 0, 0, enemyview_set },
    { "display.native_mapmsg", S_SCREENS, 0, "新增陆地视图消息", "陆地视图的消息（如强盗骑士的地盘等）以卡牌风格绘制。", OC_SHOW, 0, 1, 1, 0, 0, mapmsg_set },
    { "display.native_saves", S_SCREENS, 0, "新增存档与载入界面", "每次存档均列出其位置、日期与队伍；存档时需在列表旁填写备注，载入时则列出所有存档（原版仅列九个）。", OC_SHOW, 0, 1, 1, 0, 0, savesview_set },

    { "assist.card_helpers", S_CARDS, 0, "卡牌上的概率与提示", "卡牌上标明每种可能失败选项的概率及尝试者；接受哪些圣人与物品、谁拥有它们；针对带来教师的选项设有讲台（技能、等级、费用）；各项选项同时呈现，无渐显效果，可供1至9种选择；码头处还附有小地图，标明各船的去向。工具提示说明物品、圣人或配方的用途。", OC_SHOW, 0, 1, 1, 0, 0, 0, 0, 0, 5, bundle_set },
    { "assist.chances", S_CARDS, 0, "成功几率", "在每个可能失败的卡牌选项旁，显示尝试该选项的队员（原版在按住Shift时一次显示一个）。", OC_SHOW, 0, 2, 2, "始终、按数字|根据难度等级|关闭", V_CHANCES, assist_set_chances },
    { "assist.best_leader", S_CARDS, 0, "最优秀的队员代表队伍发言", "当某张卡牌考验队长时，由最擅长该项的队员代为测试；而在商店里，则由魅力值最高的队员进行讨价还价；显示的几率为该队员的数值，并在其头像旁标注。几率末尾的箭头表示每位队员的状况：点击其中一位，即可让该队员代替发言。卡牌结束后，你所选择的队长将重新担任队长。（关闭时，仍由队长发言，但箭头依然可让你选择其他队员。）", OC_EASE, 0, 1, 1, 0, 0, assist_set_best_leader },
    { "assist.card_help", S_CARDS, 0, "卡牌上的圣人与物品", "卡牌接受哪些圣人与物品，以及谁知晓或携带它们（正如《Darklands Companion》这本玩家参考手册所示：即使尚无人知晓，圣人也会被列出）。", OC_SHOW, 0, 1, 1, 0, 0, assist_set_card_help },
    { "assist.shop_info", S_CARDS, 0, "工具提示", "指针下的物品、圣人或配方有何用途（在商店、战利品界面、角色面板以及祈祷与炼金术界面中）。", OC_SHOW, 0, 1, 1 },
    { "assist.fast_fades", S_CARDS, 0, "无淡入效果的卡牌", "卡牌一次性显示所有选项，不再采用缓慢的淡入效果。", OC_SHOW, 0, 1, 1 },
    { "assist.card_keys", S_CARDS, 0, "卡牌选项的数字键", "1至9可选择卡牌的各个选项。", OC_SHOW, 0, 1, 1, 0, 0, assist_set_card_keys },
    { "assist.goto_work", S_CARDS, "assist.city_goto", "谁有工作", "在“前往”列表中，在富格尔、美第奇、汉萨同盟、大教堂、议会、领主和集市摊位旁：显示询问是否能获得任务的几率、他们下次获得任务的日期，或注明其任务正在进行中或奖励已待领取。", OC_SHOW, 0, 1, 1 },
    { "assist.teachers", S_CARDS, 0, "教师", "在每个带来教师到旅店的卡牌选项旁，设有讲台，标明技能、教师等级及收费；边距与世界地图（M）则显示各城镇所能提供的教师。", OC_SHOW, 0, 1, 1 },
    { "assist.city_goto", S_CARDS, 0, "前往（G）", "在城镇卡牌上，G列出该地的各类场所；选中一处，队伍即前往，并自动为你逐级进入相关菜单。", OC_EASE, 0, 1, 1 },
    { "assist.docks_map", S_CARDS, 0, "码头的地图", "在码头的船只列表旁，附有一张小地图，标示各船将前往的城镇及其沿河沿海的行进路线；点击某个城镇，即可乘坐相应船只。", OC_EASE, 0, 1, 1 },
    { "journal.enabled", S_CARDS, 0, "记录各城镇的商店与商品", "各城镇的商店、其商品、品质与价格，均在抵达时记录（显示于世界地图，M）。可在F12 > 日志中切换查看。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "journal.chronicle", S_CARDS, "journal.enabled", "编年史", "卡牌所展示的每一段文字，都逐字逐句记录，包括日期、地点及所作选择，以便数周前读到的线索也能再次找到（F12 > 日志 > 编年史，可在其中切换）。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },

    { "assist.return_thrown", S_PARTY, 0, "战斗后拾取的投掷武器", "战斗结束后打开战利品界面时，每位队员投掷过的标枪、投斧、飞刀及飞镖都会从堆中取出并归还给他们。", OC_EASE, 0, 1, 1, 0, 0, return_thrown },
    { "assist.endless", S_PARTY, 0, "无限弹药与投掷武器", "箭矢、箭镞与箭丸，以及标枪、投斧、飞刀和飞镖，永不耗尽（一支即可射击或投掷）；而敌方的同类武器则会被消耗殆尽。关闭时：每次射击使用一支，约十分之七的箭矢与箭镞可在战利品中拾取；借助共享背包，这些武器便成为每位队员自己的物品，如同药剂一般列于清单（拾取的武器归投掷者所有；拖动至另一名队员即可移交）。", OC_PLUS, 0, 1, 1, 0, 0, 0, 0, 0, 6, bundle_set },
    { "assist.infinite_ammo", S_PARTY, 0, "无限弹药", "箭矢、箭镞与箭丸永不耗尽（一支即可射击）；而敌方的同类武器则会被消耗殆尽。关闭时：每次射击使用一支，约十分之七的箭矢与箭镞可在战利品中拾取；借助共享背包，这些武器便成为每位队员自己的物品，如同药剂一般列于清单。", OC_PLUS, 0, 1, 1, 0, 0, ammo_set },
    { "assist.infinite_thrown", S_PARTY, 0, "无限投掷武器", "标枪、投斧、飞刀和飞镖同样永不耗尽（一支即可投掷）。关闭时，借助共享背包，这些武器便成为每位队员自己的物品，如同药剂一般列于清单。", OC_PLUS, 0, 1, 1, 0, 0, thrown_set },
    { "rules.aging", S_PARTY, 0, "游戏中的衰老", "角色在游戏过程中会逐渐衰老（仅限Darklands+；原版仅在创建角色时才会衰老）：自三十岁起，每月略有衰退，随年龄增长愈发明显（至七十岁时，将累计损失25点耐力、21点力量与敏捷、10点感知与魅力，以及5点智力）。", OC_PLUS, 0, 1, 0 },
    { "assist.loss_alerts", S_PARTY, 0, "提醒永久性损失", "凡是永久性丧失的项目（如属性上限、技能、被酸或火侵蚀的护甲，以及岁月的损耗），都会暂停游戏，并弹出窗口明确告知具体损失内容及责任人。", OC_SHOW, 0, 1, 1 },

    { "battle.controls", S_BATTLE, 0, "更简便的战斗操作", "点击一名队员（或其在队伍面板中的方框），再点击敌人以发起攻击，或点击地面以行走：一次点击即可下达指令，而在暂停期间，系统会自动选择下一位未接指令的队员。每位射手方框上的弓或剑图标，表明该次点击是射击还是格斗。配有图标的指令按钮、右拖动可调整视角、选择队员不会改变视角、战斗结束后的等待时间更短，以及Z键可快进（再次按下Z则恢复原速）。", OC_EASE, 0, 1, 1, 0, 0, 0, 0, 0, 1, bundle_set },
    { "battle.helpers", S_BATTLE, 0, "屏幕上的战斗信息", "每个受伤的敌人都有一条横线，指向某人时会显示其身份、伤势程度、近战等级、瞄准情况和所持武器；射击轨迹及其途中的障碍物；点击后将执行的操作；对屏幕外敌人的箭头指示；战士技能提升进度条；射手下方的装填进度条；以及战斗日志（记录每一次攻击、格挡与倒地）。", OC_SHOW, 0, 1, 1, 0, 0, 0, 0, 0, 2, bundle_set },
    { "battle.pauses", S_BATTLE, 0, "战斗中的暂停", "当队伍成员倒下，或城堡、地牢、矿井中等待的敌人现身参战时，战斗便会暂停。", OC_EASE, 0, 1, 1, 0, 0, 0, 0, 0, 3, bundle_set },
    { "battle.scrolling", S_BATTLE, 0, "平滑、稳定的滚动", "战斗视图滑动而非跳跃，以稳定速度滚动，将指针停留在边缘即可在不按Shift的情况下滚动。", OC_SHOW, 0, 1, 1, 0, 0, 0, 0, 0, 4, bundle_set },
    { "battle.pause_down", S_BATTLE, 0, "成员倒下时暂停", "", OC_EASE, 0, 1, 1, 0, 0, ap_down },
    { "battle.pause_hurt", S_BATTLE, 0, "重伤时暂停", "当队伍成员的耐力降至四分之一以下时，战斗暂停。", OC_EASE, 0, 0, 0, 0, 0, ap_hurt },
    { "battle.pause_foes", S_BATTLE, 0, "敌人现身时暂停", "当城堡、地牢或矿井中等待的敌人现身参战时，战斗暂停。", OC_EASE, 0, 1, 1, 0, 0, autopause_set_foes },
    { "battle.improved", S_BATTLE, 0, "改进后的战斗系统", "队伍在指令之间合理作战。持有弓、弩、枪或投掷武器的队员优先射击并持续射击：目标倒下后，转向视野内最近的敌人；若无清晰视线（如墙壁、树木或中间有人阻挡，正如瞄准线所示），则移动至附近几步处有空隙的位置（或靠近直至出现空隙）；只有当敌人逼近或已无其他可射之物时才进行近身格斗。敌方倒下的队员会转向附近的敌人，格挡、狂暴与脆弱状态按指令维持，而点击敌人时若仅此招能穿透其护甲，则发动脆弱攻击（瞄准护甲缝隙）。离线时：沿用原版方式。", OC_EASE, 0, 1, 1 },
    { "battle.click_attack", S_BATTLE, 0, "点击敌人发起攻击", "点击敌人会使选定队员立即对其发起攻击，即便尚未选择具体攻击方式：标准攻击，或在改进战斗模式下使用脆弱攻击——唯有该招式才能突破敌方护甲。", OC_EASE, 0, 1, 1, 0, 0, click_attack },
    { "battle.volley_toggle", S_BATTLE, "battle.click_attack", "每位队员的装备栏上显示弓或剑", "在战斗中，手持远程武器的队员会在队伍面板的装备栏上显示弓或剑：点击敌人时是射击还是近身格斗。点击切换；每位队员保持自己的选择。", OC_SHOW, 0, 1, 1 },
    { "battle.panel_select", S_BATTLE, 0, "点击队员的装备栏下达指令", "在战斗中，点击队伍面板上队员的装备栏即可为其下达指令，如同1至5号操作一样（双击还可将视图移至该队员）；右键点击装备栏则打开其角色面板。离线时：点击装备栏即打开面板，与原版相同。", OC_EASE, 0, 1, 1 },
    { "battle.quick_orders", S_BATTLE, 0, "一次点击下达一条指令", "当选定队员后，点击敌人或地面即可立即下达指令，同样适用于无需目标的指令（停止、开门、上楼梯）。从“指令”菜单或按钮（标准、脆弱、狂暴、格挡）中选择的攻击将针对队员当前正在交战的敌人。若选中队员导致战斗暂停，则恢复时直接执行该指令。离线时：点击仅标记目标，按下Enter、Space或选择其他队员方可下达指令，与原版一致。", OC_EASE, 0, 1, 1 },
    { "battle.next_member", S_BATTLE, "battle.quick_orders", "随后选择下一位未接指令的队员", "战斗暂停期间，下达指令会自动选取屏幕上首位未接指令的队员，因此每场战斗开始时，每条指令都只需点击一次敌人即可。按下Space即可恢复战斗。", OC_EASE, 0, 1, 1 },
    { "battle.order_bar", S_BATTLE, 0, "战场上方的指令按钮", "当队员被选中时，其指令以按钮形式显示于战场底部的一条横带上，按功能分类（攻击、移动、随身物品、药剂与道具、战斗等），无需右键菜单。快进与恢复按钮位于右侧边栏顶部（窗口无右侧边栏时则置于横带顶端）。", OC_SHOW, 0, 1, 1 },
    { "battle.order_icons", S_BATTLE, 0, "指令按钮上的图标", "每个指令按钮名称旁均配有图标：标准攻击为剑，格挡为盾，前进为脚印，投降为白旗。", OC_SHOW, 0, 1, 1 },
    { "battle.los_line", S_BATTLE, 0, "显示射击轨迹", "当队员即将在指针下方向敌人射击或投掷时，一条连线会从其延伸至目标。途中任何障碍物（墙壁、树木、中间有人阻挡）均以红色勾勒，连线越过障碍物后亦变为红色：该位置无法完成射击。", OC_SHOW, 0, 1, 1 },
    { "battle.click_hint", S_BATTLE, 0, "标明点击后的作用", "当队员被选中时，指针旁会显示简短说明，解释该处点击在非普通攻击或行走时的具体作用（射击、投掷、逃跑、脆弱或狂暴攻击），或为何无法执行（无路径、无清晰射击）。", OC_SHOW, 0, 1, 1 },
    { "battle.drag_pan", S_BATTLE, 0, "拖动以移动视图", "按住右键（或中键）并移动鼠标即可拖动战斗视图；不移动时右键仍可打开菜单。", OC_SHOW, 0, 1, 1 },
    { "battle.select_stays", S_BATTLE, 0, "选中队员不会移动视图", "1至5号（或点击队员装备栏）选中队员时不会移动视图；连续按两次则可将视图移至该队员并跟随其行动。在另一层楼选中队员仍会移动视图至该队员。", OC_SHOW, 0, 1, 1 },
    { "battle.smooth_scroll", S_BATTLE, 0, "平滑滚动", "战斗视图每次到达新位置时都会平滑滑动，而非跳跃。", OC_SHOW, 0, 1, 1 },
    { "battle.scroll_paced", S_BATTLE, 0, "稳定滚动", "战场以稳定速度滚动（在边缘按住Shift不再能快速滚动至战场尽头）。", OC_SHOW, 0, 1, 1, 0, 0, bs_paced },
    { "battle.scroll_rest", S_BATTLE, "battle.scroll_paced", "无Shift的边缘滚动", "将指针停留在屏幕边缘即可滚动战场；无需按Shift。", OC_SHOW, 0, 1, 1, 0, 0, bs_rest },
    { "battle.info", S_BATTLE, 0, "指针下的信息", "在每个受伤的敌人上方显示其剩余的耐力条（Alt：显示所有）；指向某人时，会显示其身份、伤势程度、近战属性（近战攻击与防御）、远程武器的瞄准精度以及所持武器。", OC_SHOW, 0, 1, 1 },
    { "battle.info_numbers", S_EASYPLAY, 0, "敌人的耐力数值", "", OC_EASY, 0, 0, 1 },
    { "battle.foe_arrow", S_BATTLE, 0, "屏幕外敌人的箭头", "当屏幕上没有敌人时，边缘会有一根箭头指向最近的敌人；点击它即可将视角移至该处。", OC_EASE, 0, 1, 1 },
    { "battle.skill_gains", S_BATTLE, 0, "显示技能提升", "战斗中提升的技能会在战士头顶以金色浮标显示（+2 刀剑），同时在队伍面板成员栏中也会出现相关提示。", OC_SHOW, 0, 1, 1 },
    { "battle.reload_bar", S_BATTLE, 0, "重新装填进度条", "在准备射击的队员下方有一条细长的琥珀色进度条（弓、弩、枪或投掷武器），随着射击动作逐渐填满。", OC_SHOW, 0, 1, 1 },
    { "battle.log", S_BATTLE, 0, "战斗日志", "左侧边栏宽屏显示每一次击打、格挡、伤口与倒地；其他情况下则在战场上显示（F12 > 日志 > 战斗日志可保留）。", OC_SHOW, 0, 1, 1 },
    { "assist.level_map", S_BATTLE, 0, "关卡地图（Tab）", "战斗、城堡、矿井与地下城中的Tab键：显示队伍已探索过的关卡地图；点击即可带领队伍前往相应地点。", OC_SHOW, 0, 1, 1 },
    { "assist.level_map_all", S_EASYPLAY, 0, "关卡地图上的整个关卡", "Tab键显示的是整个关卡，而不仅仅是队伍已探索的部分。", OC_EASY, 0, 0, 1 },
    { "assist.more_jobs", S_PARTY, 0, "更多兼职工作", "原版未提供的九种职业（雇佣兵、卫队军士、保镖、宫廷侍从、书记员、文员、游吟诗人、艺人、传教修士）会在合适的城镇中出现：士兵类职业在军械库或要塞附近，领主家政在要塞内，文职在市政厅，大城市中的贵族府邸，修士则在修道院旁。每位成员仍会选择自己最擅长且薪酬最高的工作。", OC_EASE, 0, 1, 1 },
    { "battle.foes_left", S_EASYPLAY, 0, "敌人仍在等待", "在敌人列表下方，显示队伍在城堡、地牢或矿井中尚未遭遇的敌人数量，按楼层逐一列出，并标明其首领所在楼层（这是队伍原本无法得知的信息）。", OC_EASY, 0, 0, 1 },
    { "battle.end_words", S_BATTLE, 0, "战斗结束时的等待时间缩短", "当最后一名敌人倒下时，战场会静止一秒钟而非原先的三秒半，随后“胜利！战斗已获胜。”（或其他类似提示）会停留三秒而非八秒。原版漫长的等待时间曾让人感觉像是画面冻结。", OC_SHOW, 0, 1, 1 },
    { "fix.pathfinding", S_BATTLE, 0, "战士们能绕过障碍物", "战士们会规划路线避开墙壁、家具以及彼此；在原版中，他们常常因路径过长或曲折而放弃（成员甚至会在矿井里被困）。敌人也同样如此，因此战斗更加艰难。在人群拥挤时，被堵住的战士会立即寻找新路，而不是僵立数秒；而位于后方的敌人则会等待或另选目标，而不必绕远路。", OC_FIX, 0, 1, 1, 0, 0, pf_set },
    { "battle.speed", S_BATTLE, 0, "战斗速度", "行走（包括在地牢、城堡和矿井中）、攻击、服用药剂以及召唤圣人等动作都会加快；暂停（空格键）功能保持不变。", OC_TASTE, 100, 100, 100, "按原版|1.5倍|2倍|3倍", V_SPEED, battle_speed_set },
    { "battle.fast_key", S_BATTLE, 0, "快进（Z）", "Z键在战斗中可使游戏速度提升六倍；再次按下Z则恢复到之前选择的速度。", OC_SHOW, 0, 1, 1, 0, 0, fast_key },

    { "map.key_m", S_TRAVEL, 0, "世界地图（M）", "M打开一张涵盖全境的地图（城镇、任务、圣人与配方的分布地点）。", OC_SHOW, 0, 1, 1 },
    { "travel.auto", S_TRAVEL, "map.key_m", "自动旅行", "队伍会沿着道路自动前往你选定的目的地：在世界地图（M）上双击（或选择“前往”），或在陆地视图中Ctrl+点击。任何按键或点击均可停止自动旅行。", OC_EASE, 0, 1, 1, 0, 0, tr_auto },
    { "travel.fast", S_TRAVEL, "travel.auto", "自动旅行速度", "", OC_TASTE, 0, 1, 1, "行走速度|4倍|即时", V_PACE, tr_fast },
    { "map.overland_marks", S_TRAVEL, 0, "陆地视图中的名称与任务标记", "陆地视图中标注的城镇与地点、每个任务图标及其位置、以及城镇旁标注的禁止队伍进入或守卫驻扎的标志。", OC_SHOW, 0, 1, 1, 0, 0, 0, 0, 0, 7, bundle_set },
    { "map.names", S_TRAVEL, 0, "陆地视图中的地名", "陆地视图中标注的城镇与地点。", OC_SHOW, 0, 1, 1 },
    { "map.quest_marks", S_TRAVEL, 0, "陆地视图中的任务标记", "陆地视图中每个任务图标及其位置、以及城镇旁标注的禁止队伍进入或守卫驻扎的标志。", OC_SHOW, 0, 1, 1 },
    { "map.offers_unvisited", S_EASYPLAY, 0, "哪些未访问的地方提供", "圣人拥有的图书馆、炼金术士出售的配方，以及商店的商品——这些在城镇和修道院中队伍尚未造访过的地方：在世界地图上、角色面板上，以及当某种原料即将耗尽时的旁边。关闭时：仅显示队伍已见过的内容。（每个城镇的铁匠铺和商店有多出色，无论哪种方式都会显示。）", OC_EASY, 0, 0, 1 },
    { "travel.encounters", S_EASYPLAY, 0, "路途中的遭遇", "旅行过程中遇到敌人的频率。", OC_EASY, 100, 100, 50, "四分之一|一半|与原版相同|两倍", V_RATE },
    { "travel.street_encounters", S_EASYPLAY, 0, "夜间街道上的遭遇", "盗贼和守卫在夜间出现在城镇中的频率（原版中，在侧街里，对于一个隐蔽性和街头智慧较低的队伍，约有八成几率遇到歹徒；而在主街上则偶尔会出现守卫）。", OC_EASY, 100, 100, 50, "四分之一|一半|与原版相同|两倍", V_RATE },



    /* on other pages (Display, Sound, Saves): the presets set them */
    { "display.text_font", S_OTHER, 0, "字体", "卡牌和新界面的字体样式（游戏自带的字体清晰锐利，或选用多种时代风格的手写体）。", OC_LOOK, 6, 6, 6, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.overlay_font_follow", S_OTHER, 0, "边框也用同一字体", "侧边栏、工具提示，以及世界地图和陆地视图中的地名，也都采用所选字体。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.spell_out", S_OTHER, 0, "拆解缩写", "角色面板上的缩写被完整写出（wPol：长柄武器）。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.coins", S_OTHER, 0, "货币以硬币形式", "弗罗林、格罗申和芬尼作为小额货币（1 12 4），而非1弗罗林 12格罗申 4芬尼。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.text_edge", S_OTHER, 0, "字母边缘", "纯色、阴影或轮廓环绕字母，叠加于图像之上（0纯色，1阴影，2轮廓）。", OC_LOOK, 2, 2, 2, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.text_contrast", S_OTHER, 0, "加深浅色文字", "浅色文字在其背景上被加深（或提亮），以确保可读性。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.text_size", S_OTHER, 0, "文字大小", "边框、工具提示及地名（世界地图和陆地视图）中文字的大小，以百分比表示。", OC_LOOK, 100, 100, 100, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.skill_icons", S_OTHER, 0, "技能旁的图标", "技能和属性旁的小图标。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.skill_icon_look", S_OTHER, 0, "技能图标外观", "盾牌上的纹章图案（2）或彩色图案（0），无论何时显示某项技能或属性。", OC_LOOK, 2, 2, 2, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.potion_grade", S_OTHER, 0, "药剂等级", "药剂等级如何体现在其瓶身上（徽章上有4个箭头，3个箭头，条纹上1个圆点，0个小圆点）。", OC_LOOK, 4, 4, 4, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.pstone_look", S_OTHER, 0, "哲学家之石", "石头的呈现方式（2为方形圆圈，1为粗糙石头，0为鸡蛋）。", OC_LOOK, 2, 2, 2, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.clock", S_OTHER, 0, "时钟", "计时器随时间流逝而显示的样式（按数字区分）。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.ornate", S_OTHER, 0, "装饰", "窗户周围的编织带与镀金角饰。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.frame_style", S_OTHER, 0, "框架样式", "窗户周围的装饰带（按数字区分）。", OC_LOOK, 0, 0, 0, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.button_style", S_OTHER, 0, "按钮样式", "按钮的外观（按数字区分）。", OC_LOOK, 0, 0, 0, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.art_names", S_OTHER, 0, "替代美术：显示文件名", "对于制作替代美术的人，每张图片将从哪个文件替换而来。", OC_LOOK, 0, 0, 0, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.smooth", S_OTHER, 0, "软缩放", "游戏画面略显模糊，如同老式显示器一般，而非由清晰的像素构成（可在F12 > 设置 > 显示中调整“柔和”）。", OC_LOOK, 0, 0, 0, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.scaler", S_OTHER, 0, "圆角效果", "对角线边缘的阶梯状部分被圆滑处理（可在F12 > 设置 > 显示中调整“圆角”）：0为原样，1和2为圆角。", OC_LOOK, 0, 0, 0, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.system_cursor", S_OTHER, 0, "系统指针", "游戏界面中的系统鼠标指针。", OC_LOOK, 0, 0, 0, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.live_pointer", S_OTHER, 0, "平滑指针", "游戏鼠标指针的移动与系统指针一样流畅。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.pointer_drawn", S_OTHER, 0, "绘制指针", "游戏装甲手套重新绘制，无论大小都清晰锐利：拖动时呈现拳头形态；暂停战斗时袖口处显示停顿标志，与敌人交锋时交叉剑刃，前往某地时则显示沙漏（否则为游戏的小图标）。", OC_LOOK, 0, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.ui_scale", S_OTHER, 0, "菜单尺寸", "Darklands+菜单的尺寸，以百分比表示（0为系统缩放比例），重启后生效。", OC_LOOK, 0, 0, 0, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.pause_unfocused", S_OTHER, 0, "背景暂停", "当游戏窗口处于后台时，游戏会自动暂停。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.opening", S_OTHER, 0, "启动时的开场动画", "开场动画：0为无，1为最后一幕，2为全部内容（-1为首次启动时的全部内容）。", OC_LOOK, -1, -1, -1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.draw_cache", S_OTHER, 0, "绘制缓存", "精美边框仅绘制一次并保留，不再每次重复绘制：1（默认值，处理器将编织带制成图案，其余保持三角形）、0关闭、3全部以三角形保留、2以显卡绘制的图案保留。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "encounters.new", S_OTHER, 0, "模组新增遭遇", "模组添加的新遭遇（F12 > 模组）会在路途中出现。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "encounters.seal", S_OTHER, 0, "模组遭遇的标记", "来自模组而非原版游戏的卡片角落上，有一个小小的红色标记。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "encounters.share", S_OTHER, 0, "模组遭遇的出现频率", "以百分比表示：在100%时，每种遭遇与该地形原版遭遇的概率相同。", OC_LOOK, 100, 100, 100, 0, 0, 0, 0, OF_ELSEWHERE },
    { "input.prompts", S_OTHER, 0, "按钮提示", "屏幕上的控制器按钮：0为当前使用的设备，1为键盘与鼠标，2为Xbox，3为PlayStation，4为任天堂。", OC_LOOK, 0, 0, 0, 0, 0, 0, 0, OF_ELSEWHERE },
    { "saves.ironman", S_OTHER, 0, "铁人模式", "下一局新游戏仅保留一份存档，由游戏自身在进行过程中不断覆盖保存（每张卡片、每次到达、每次战斗前、退出时均如此）；运行期间无法加载回退；当队伍旅程结束时，存档将被删除。", OC_LOOK, 0, 0, 0, 0, 0, 0, 0, OF_ELSEWHERE },
    { "saves.backup", S_OTHER, 0, "备份存档", "每次存档被覆盖前的副本。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "saves.keep_auto", S_OTHER, 0, "自动存档保留数量", "自动存档保留的数量。", OC_LOOK, 3, 3, 3, 0, 0, 0, 0, OF_ELSEWHERE },
    { "saves.keep_quick", S_OTHER, 0, "快速存档保留数量", "快速存档保留的数量。", OC_LOOK, 3, 3, 3, 0, 0, 0, 0, OF_ELSEWHERE },
    { "sound.music_volume", S_OTHER, 0, "音乐音量", "以百分比表示。", OC_LOOK, 100, 100, 100, 0, 0, 0, 0, OF_ELSEWHERE },
    { "sound.effects_volume", S_OTHER, 0, "效果音量", "以百分比表示。", OC_LOOK, 100, 100, 100, 0, 0, 0, 0, OF_ELSEWHERE },
    { "sound.music_in_menus", S_OTHER, 0, "菜单中的音乐", "Darklands+菜单打开时，背景音乐持续播放。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "sound.mt32_lcd", S_OTHER, 0, "MT-32显示", "当游戏显示消息时，MT-32的小型显示屏会出现在右上角。", OC_LOOK, 1, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "quests.sort", S_OTHER, 0, "任务按以下方式排序", "边缘的任务：0 按截止日期，1 最近优先，2 按领取顺序。", OC_LOOK, 0, 0, 0, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.crisp_text", S_OTHER, 0, "新文本", "", OC_TASTE, 0, 2, 2, 0, 0, hdtext_set, 0, OF_ELSEWHERE },
    { "display.hd_art", S_OTHER, 0, "替代美术", "", OC_TASTE, 0, 1, 1, 0, 0, hdart_set, 0, OF_ELSEWHERE },
    { "sound.effects", S_OTHER, 0, "来自音效包的战斗音效", "", OC_LOOK, 0, 0, 0, 0, 0, sfx_set, 0, OF_ELSEWHERE },
    { "display.margins", S_OTHER, 0, "侧边栏", "", OC_SHOW, 0, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.full_names", S_OTHER, 0, "全名", "", OC_SHOW, 0, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "display.armour_worn", S_OTHER, 0, "盔甲穿戴位置", "", OC_SHOW, 0, 1, 1, 0, 0, 0, 0, OF_ELSEWHERE },
    { "saves.quick", S_OTHER, 0, "快速存档与自动存档", "", OC_EASE, 0, 1, 1, 0, 0, saves_enable, 0, OF_ELSEWHERE },
    { "saves.autosave_arrive", S_OTHER, 0, "抵达时自动存档", "", OC_EASE, 0, 1, 1, 0, 0, auto_arrive, 0, OF_ELSEWHERE },
    { "saves.autosave_battle", S_OTHER, 0, "战斗前自动存档", "", OC_EASE, 0, 1, 1, 0, 0, auto_battle, 0, OF_ELSEWHERE },
    { "saves.restart_on_end", S_OTHER, 0, "旅程结束时返回主界面", "", OC_EASE, 0, 1, 1, 0, 0, restart_end, 0, OF_ELSEWHERE },
    /* what MicroProse wrote, coded or left unfinished and never switched on, put back: experimental, off by default and in every preset */
    { "rules.fehde", S_RESTORED, 0, "Fehde！”伏击", "由私人仇杀士兵发动的路障伏击，写成雇佣兵伏击的一种变体但从未启用：一半情况下出现的是Fehde伏击，拥有独立的文本但概率和战斗相同（由骑士率领的士兵）。", OC_EXPERIMENT, 0, 0, 0 },
    { "rules.hospice", S_RESTORED, 0, "城镇收容所", "白天的城镇收容所，已编写但从未实装：施舍、在修女身边工作一天（治疗）、付费让队伍伤口得到护理，或乞讨免费护理。教堂区通往此处，且“前往”（G）中列有此地。", OC_EXPERIMENT, 0, 0, 0 },
    { "rules.war", S_RESTORED, 0, "战争中的城市", "已编码但从未启用。偶尔某城会为战争做数周准备，部分城市还会遭遇围攻：物价上涨、新闻报道、士兵之路及其市场开放、码头被守卫或封锁、城门拒绝旅客通行（交谈、虚张声势、潜行、用农夫的干草车，或借助城墙巡逻队误以为你是间谍）。战争期间存档，原版游戏加载时仍保留战争状态。", OC_EXPERIMENT, 0, 0, 0 },
    { "rules.politics", S_RESTORED, 0, "城市政治", "已编写但从未启用的竞选活动。偶尔某城会出现一段动荡期：新闻与公会区域的传闻会指向叛军首领或政府官员，队伍选边站队、张贴传单、影响公会、而战争委员会则决定何时游行至市政厅或实施逮捕。", OC_EXPERIMENT, 0, 0, 0 },
    { "battle.black_cloud", S_RESTORED, 0, "战斗中的黑云", "队伍也可在战斗中释放黑云。手册曾将其略去（“1992年的机器上屏幕显示时间过长”），但战斗规则已处理其烟雾效果：穿透黑云的射击命中率降至五分之一，而在黑云内作战则使命中加减15。敌人与卡牌早已使用该机制。", OC_EXPERIMENT, 0, 0, 0 },
    { "assist.hall_of_fame", S_RESTORED, 0, "荣誉榜（名人堂）", "游戏中未完成的名人堂（图片与表格存在于文件中，从未启用）：队伍在旅程结束时可进入，按下F12 > 日志 > 荣誉榜即可查看。", OC_EXPERIMENT, 0, 0, 0 },
};
#define NSTATIC ((int)(sizeof OPTIONS / sizeof OPTIONS[0]))

/* the fixes and the built-in changes, appended as they are */
static Option *dyn; static int ndyn = -1;
static char (*dyn_key)[48];
static void fix_apply(int i, int v) { fix_enable(i, v); }
static void mod_apply(int i, int v) { mod_enable(i, v); }
static void build_dyn(void)
{
    if (ndyn >= 0) return;
    int n = FIX_COUNT + MOD_COUNT;
    dyn = calloc((size_t)(n ? n : 1), sizeof *dyn); dyn_key = calloc((size_t)(n ? n : 1), sizeof *dyn_key);
    ndyn = 0;
    if (!dyn || !dyn_key) return;
    for (int i = 0; i < MOD_COUNT; i++) {            /* (easy: the ones once in the Easy preset; kept as the Easy column of docs/SWITCHES.md) */
        Option *o = &dyn[ndyn]; snprintf(dyn_key[ndyn], sizeof dyn_key[0], "mod.%s", MODS[i].id);
        int easy = !strcmp(MODS[i].id, "cheap_inn") || !strcmp(MODS[i].id, "quick_learning") || !strcmp(MODS[i].id, "heavy_blows") || !strcmp(MODS[i].id, "rich_start");
        o->key = dyn_key[ndyn]; o->section = "更易上手"; o->title = MODS[i].name; o->desc = MODS[i].desc;
        o->cls = OC_EASY; o->orig = 0; o->def = 0; o->easy = easy; o->arg = i; o->apply_i = mod_apply;
        o->note = MODS[i].needs_new_game ? "下一局新游戏开始时生效。" : 0;
        ndyn++;
    }
    for (int i = 0; i < FIX_COUNT; i++) {
        Option *o = &dyn[ndyn]; snprintf(dyn_key[ndyn], sizeof dyn_key[0], "fix.%s", FIXES[i].id);
        o->key = dyn_key[ndyn]; o->section = "修复原版漏洞"; o->title = FIXES[i].name; o->desc = FIXES[i].desc;
        o->cls = OC_FIX; o->orig = 0; o->def = 1; o->easy = 1; o->arg = i; o->apply_i = fix_apply;
        ndyn++;
    }
}
int options_count(void) { build_dyn(); return NSTATIC + ndyn; }
const Option *options_get(int i)
{
    build_dyn();
    if (i < 0) return 0;
    if (i < NSTATIC) return &OPTIONS[i];
    return i - NSTATIC < ndyn ? &dyn[i - NSTATIC] : 0;
}
const Option *options_find(const char *key)
{
    int n = options_count();
    for (int i = 0; i < n; i++) if (!strcmp(options_get(i)->key, key)) return options_get(i);
    return 0;
}
int option_value(const Option *o) { return config_get_int(o->key, o->def); }

static void (*listener)(const char *key, int v);
void options_set_listener(void (*fn)(const char *key, int v)) { listener = fn; }
void option_set(const Option *o, int v)
{
    config_set_int(o->key, v);
    if (o->apply) o->apply(v);
    if (o->apply_i) o->apply_i(o->arg, v);
    if (listener) listener(o->key, v);
}
int option_nchoices(const Option *o)
{
    if (!o->choices) return 0;
    int n = 1; for (const char *c = o->choices; *c; c++) n += *c == '|';
    return n;
}
const char *option_choice(const Option *o, int k, char *buf, int n)
{
    const char *c = o->choices ? o->choices : "";
    for (; k > 0 && *c; c++) if (*c == '|') k--;
    int len = 0; while (c[len] && c[len] != '|') len++;
    snprintf(buf, (size_t)n, "%.*s", len, c);
    return buf;
}

/* ---------------------------------------------------------------- presets */
const Preset PRESETS[PRESET_COUNT] = {
    { "完整原版", "1993年Darklands原版（版本483.07），含所有漏洞：Darklands+新增的一切玩法均被关闭，沿用原版游戏的音效设置。外观（文字、画面、屏幕）保持各部分原有设定。",
      "存档可在原版游戏与新版本间加载。" },
    { "纯粹主义者", "修复了所有漏洞的原版游戏：自有界面、规则与画面。",
      "存档可在原版游戏与新版本间加载。" },
    { "Darklands+", "修正原版漏洞后的全新规则，搭配新界面，屏幕上呈现原版隐藏于按键后或仅见于手册的内容，并减少繁琐操作：自动旅行、前往、更便捷的战斗控制、统一装备包、每家旅店设单独储物间、弹药无限。更易上手功能以内置模组形式单独提供（F12 > 模组）。",
      "存档可在原版游戏与新版本间加载，除非成员使用了敌方精灵。" },
};

int preset_value(const Option *o, int p)
{
    { int g = bundle_of(o->key); if (g >= 0) { const Option *m = options_find(BUNDLES[g].key); return m && preset_value(m, p) ? o->def : o->orig; } }
    if (o->cls == OC_TOOL || o->cls == OC_LOOK) return PRESET_KEEP;
    if (o->cls == OC_TASTE) return p <= PRESET_PURIST ? o->orig : PRESET_KEEP;
    if (o->cls == OC_EASY) return p <= PRESET_PURIST ? o->orig : PRESET_KEEP;   /* (a built-in mod: the player's own, but not in the originals) */
    int on = 0;
    switch (o->cls) {
    case OC_FIX: on = p >= PRESET_PURIST; break;
    case OC_SHOW: on = p >= PRESET_PLUS; break;
    case OC_EASE: on = p >= PRESET_PLUS; break;
    case OC_PLUS: on = p >= PRESET_PLUS; break;
    default: on = 0; break;
    }
    return on ? o->def : o->orig;
}
static int preset_flag_ok(int p) { return (config_get_int("original", 0) != 0) == (p == PRESET_BUGGY_PURIST); }
int preset_differences(int p)
{
    int n = options_count(), d = preset_flag_ok(p) ? 0 : 1;
    for (int i = 0; i < n; i++) {
        const Option *o = options_get(i);
        int pv = preset_value(o, p);
        if (pv == PRESET_KEEP) continue;
        if (option_value(o) != pv) d++;
    }
    return d;
}
int preset_current(void)
{
    if (config_get_int("original", 0)) return PRESET_BUGGY_PURIST;
    for (int p = PRESET_PURIST; p < PRESET_COUNT; p++) if (!preset_differences(p)) return p;
    return PRESET_CUSTOM;
}
int preset_nearest(int *diffs)
{
    int best = PRESET_PLUS, bd = 1 << 30;
    for (int p = PRESET_PURIST; p < PRESET_COUNT; p++) { int d = preset_differences(p); if (d < bd || (d == bd && p == PRESET_PLUS)) { bd = d; best = p; } }
    if (diffs) *diffs = bd;
    return best;
}

/* the player's own choices, kept before a preset replaces them: "custom.KEY" for each switch not at
   its Darklands+ value (and the sound device); "custom.saved" says there are some */
static void save_custom(void)
{
    char k[96];
    int n = options_count();
    for (int i = 0; i < n; i++) {
        const Option *o = options_get(i); int v = option_value(o);
        if (o->cls == OC_LOOK) continue;                 /* (no preset touches those: nothing to keep) */
        snprintf(k, sizeof k, "custom.%s", o->key);
        if (v != o->def) config_set_int(k, v); else config_unset(k);
    }
    config_set("custom.sound", config_get("sound.device", "auto"));
    config_set_int("custom.saved", 1);
}
int preset_custom_saved(void) { return config_get_int("custom.saved", 0) && preset_current() != PRESET_CUSTOM; }
void preset_restore_custom(void)
{
    if (!config_get_int("custom.saved", 0)) return;
    char k[96];
    config_set_int("original", 0);
    int n = options_count();
    for (int i = 0; i < n; i++) {
        const Option *o = options_get(i);
        if (o->cls == OC_LOOK) continue;
        snprintf(k, sizeof k, "custom.%s", o->key);
        int v = config_get_int(k, o->def);
        if (v != option_value(o)) option_set(o, v);
    }
    config_set("sound.device", config_get("custom.sound", "auto"));
    void datamods_scan(void), musicpack_reload(void); datamods_scan(); musicpack_reload();
    if (listener) listener("preset", PRESET_CUSTOM);
}

void preset_apply(int p)
{
    if (p < 0 || p >= PRESET_COUNT) return;
    if (preset_current() == PRESET_CUSTOM) save_custom();
    int purist = p <= PRESET_PURIST;
    int n = options_count();
    for (int i = 0; i < n; i++) {
        const Option *o = options_get(i);
        int v = option_value(o), pv = preset_value(o, p);
        char kk[96]; snprintf(kk, sizeof kk, "keep.%s", o->key);
        if (o->cls == OC_TASTE) {                   /* the player's own taste is kept for when a modern preset comes back */
            if (purist) { if (v != o->orig) config_set_int(kk, v); }
            else if (v == o->orig) pv = config_get_int(kk, o->def);
            else pv = PRESET_KEEP;
        }
        if (pv != PRESET_KEEP && pv != v) option_set(o, pv);
    }
    const char *dev = config_get("sound.device", "auto");    /* the purists hear the game's own sound setup */
    if (purist) { if (strcmp(dev, "game")) { config_set("keep.sound.device", dev); config_set("sound.device", "game"); } }
    else if (!strcmp(dev, "game")) config_set("sound.device", config_get("keep.sound.device", "auto"));
    config_set_int("original", p == PRESET_BUGGY_PURIST);
    config_set_int("preset", p);
    void datamods_scan(void), musicpack_reload(void); datamods_scan(); musicpack_reload();
    if (listener) listener("preset", p);
}

/* ---------------------------------------------------------------- the list for other engines */
/* docs/SWITCHES.md: every switch, its key (its name for good), what it does and why, its group, its
   class and its value in the original, in Darklands+ and in Easy; then the retired keys
   (DL_DUMP_SWITCHES=path, runtime/main.c) */
const char *option_why(const Option *o);
extern const char *const OPTIONS_RETIRED[];
static const char *cls_name(int c)
{
    switch (c) { case OC_FIX: return "fix"; case OC_SHOW: return "show"; case OC_EASE: return "ease"; case OC_PLUS: return "plus";
                 case OC_EASY: return "easy"; case OC_EXPERIMENT: return "experiment"; case OC_TASTE: return "taste"; case OC_TOOL: return "tool"; case OC_LOOK: return "look"; }
    return "?";
}
static void md(FILE *f, const char *t) { for (; t && *t; t++) { if (*t == '|') fputs("\\|", f); else if (*t == '\n') fputc(' ', f); else fputc(*t, f); } }
int options_dump_markdown(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs("# Darklands+切换\\n\\n"
          "每次切换：其密钥（名称良好：从未改名、从未重复使用）、作用、存在理由、所属群体"
          "设定它的切换（如有）、类别（何种变更：修复、展示、简化、附加、便捷、实验、口味等）"
          "工具）以及它在原版游戏/Darklands+/为更易上手所赋予的价值（前者的“简易预设”栏目）。由游戏自身制作"
          "（`DL_DUMP_SWITCHES=docs/SWITCHES.md`，源自`port/mods/options.c`与`port/mods/optwhy.c`）；`tools/check_switches.py`\\n"
          "若密钥消失且未被列入退役名单，则检查失败。", f);
    int n = options_count();
    const char *secs[64]; int ns = 0;                 /* the sections in the order they first appear; each one's switches together */
    for (int i = 0; i < n; i++) {
        const Option *o = options_get(i); int k = 0;
        while (k < ns && strcmp(secs[k], o->section)) k++;
        if (k == ns && ns < 64) secs[ns++] = o->section;
    }
    for (int sct = 0; sct < ns; sct++) {
    fprintf(f, "\n## %s\n\n| Key | Title | What | Why | Group | Class | Orig / D+ / Easy |\n|---|---|---|---|---|---|---|\n", secs[sct]);
    for (int i = 0; i < n; i++) {
        const Option *o = options_get(i);
        if (strcmp(o->section, secs[sct])) continue;
        int g = bundle_of(o->key);
        fputs("| `", f); md(f, o->key); fputs("` | ", f); md(f, o->title); fputs(" | ", f); md(f, o->desc && o->desc[0] ? o->desc : o->title); fputs(" | ", f);
        md(f, option_why(o)); fputs(" | ", f);
        if (g >= 0) { fputs("`", f); md(f, BUNDLES[g].key); fputs("`", f); }
        fprintf(f, " | %s | %d / %d / %d |\n", cls_name(o->cls), o->orig, o->def, o->easy);
    }
    }
    fputs("\n## Retired keys\n\nNo longer offered; not to be used for anything else.\n\n", f);
    for (int i = 0; OPTIONS_RETIRED[i]; i++) fprintf(f, "- `%s`\n", OPTIONS_RETIRED[i]);
    fclose(f);
    return 1;
}
