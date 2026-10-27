/* optwhy.c - why each switch exists, one line each: beside what it does (the table's description,
 * mods/options.c), the reason for it, for whoever carries a feature into another engine (the list
 * docs/SWITCHES.md is made from both: DL_DUMP_SWITCHES, runtime/main.c). The fixes need none: each
 * puts right a bug of the original (their descriptions say which). A switch's key is its name for
 * good: saved settings, presets and other engines know it by that; a switch retired is listed in
 * RETIRED, never reused. */
#include <string.h>
#include "options.h"

static const struct { const char *key, *why; } WHY[] = {
    /* groups */
    { "display.native_screens", "原版画面放大后难以辨认，而且许多信息都被按键遮挡；只需一个开关，就能让一半屏幕保持新版本、另一半保持旧版本。" },
    { "battle.controls", "原版每下达一项指令都要点击好几下、打开好几个菜单；只需一次点击即可完成指令，这样实时战斗才变得可行。一个开关：各部分相互依赖。" },
    { "battle.helpers", "原版在战场上几乎不显示任何战斗状态；这些界面只展示游戏已知的信息，不再额外添加。" },
    { "battle.pauses", "需要玩家关注的事情都在战斗进行时发生。" },
    { "battle.scrolling", "原版视角会突然跳动、快速移动，容易让玩家迷失方向。" },
    { "assist.card_helpers", "原版将概率和卡牌的适用条件藏在Shift键和说明书里；而游戏自身显示的数字则一目了然。" },
    /* screens */
    { "display.shop_compare", "比较装备与当前所持物品，必须先打开装备表。" },
    { "display.native_panel", "原版面板只显示条形图，没有具体数值。" },
    { "display.panel_picture", "这是个人喜好问题：肖像、精灵还是徽章。" },
    { "display.tooltip_delay", "这也是个人喜好问题。" },
    { "display.native_menu", "原版菜单栏只有在按住右键时才会显示。" },
    { "display.native_title", "加载最新存档需要翻阅好几屏内容。" },
    { "display.native_cards", "卡牌上的文字是游戏的核心，也是最难阅读的部分。" },
    { "display.native_shops", "原版每次只显示少量物品，且无法进行比较。" },
    { "display.native_sheet", "原版把装备表拆分到多屏小字中。" },
    { "display.native_loot", "原版每次只能点击一件物品。" },
    { "assist.loot_worth", "决定携带什么，需要根据其价值来判断。" },
    { "display.native_alchemy", "原版每次只调制一种配方，并隐藏成功率。" },
    { "assist.where_sold", "缺少材料时，只能凭记忆在城镇间寻找。" },
    { "display.native_cache", "原版每次只显示四个已存储的物品。" },
    { "display.native_lodge", "原版隐藏了费用以及教师的条款。" },
    { "display.native_pray", "原版隐藏了圣人能做的事以及成功率。" },
    { "display.native_create", "原版隐藏了每个选择所能带来的效果。" },
    { "assist.creation_guide", "角色创建是新人玩家面临的首道门槛。" },
    { "display.native_roster", "原版每次只点击一名队员，不显示任何细节。" },
    { "display.native_info", "原版每次只显示一个城镇的声望。" },
    { "display.native_march", "原版通过依次点击每位成员来确定顺序。" },
    { "display.native_enemyinfo", "原版的敌人信息每次只显示一个敌人，且不包含护甲或武器。" },
    { "display.native_mapmsg", "陆地视图中的消息风格老旧，在新屏幕上显得格格不入。" },
    { "display.native_saves", "原版仅列出九个存档，且不标注位置或队伍。" },
    /* the party and its items */
    { "inventory.shared", "在队员之间转移多余物品纯属徒劳；即使为队伍准备一个统一的背包，每位队员的装备仍会计入负重。" },
    { "store.shared", "每个城镇都设有储物室，导致储存的物品几乎等同于丢失。" },
    { "assist.inn_earn", "休息的队员放松下来毫无收获；唯有努力赚取才是玩家始终需要亲手做出的明智选择。" },
    { "assist.best_leader", "原版只测试队长，因此玩家每次换卡前都得手动更换队长。" },
    { "assist.chances", "原版只有在按住Shift键时才会显示机会，且每次只提供一个选项。" },
    { "assist.card_help", "卡片能接受哪些圣人和物品，只有《Darklands Companion》才知道。" },
    { "assist.shop_info", "每件物品、圣人或配方的用途，都只能在说明书里找到。" },
    { "assist.fast_fades", "淡入效果意味着每张卡都要等待片刻。" },
    { "assist.card_keys", "使用键盘选择比点击更快捷。" },
    { "assist.goto_work", "找到哪个办公室有工作，就得亲自去一趟。" },
    { "assist.teachers", "老师很容易被忽略：原作中没有任何地方说明某个选项会带来一位圣人。" },
    { "assist.city_goto", "在城镇里到达某个地点需要在菜单中点击很多次，而这些选择其实都不是真正的选项。" },
    { "assist.docks_map", "船只的目的地只是名称，没有地图。" },
    { "journal.enabled", "玩家其他时候只能在纸上做笔记。" },
    { "journal.chronicle", "原作并不记录旅程。" },
    { "assist.return_thrown", "每场战斗后收集投掷武器纯属浪费时间。" },
    { "assist.endless", "数箭矢、收标枪都是记账，不是游戏；敌人还是会耗光。" },
    { "map.overland_marks", "原作的陆地视图既不标注任何名称，也不显示任务。" },
    { "assist.infinite_ammo", "数箭矢是记账，不是游戏；敌人还是会耗光。" },
    { "assist.infinite_thrown", "投掷武器也是如此。" },
    { "rules.aging", "创造角色时会设定年龄，但游戏中不会体现，所以用年长的角色开局后期无需额外花费；这样就把年龄当作一种成本。" },
    { "assist.loss_alerts", "原作中永久性的损失都是悄然发生的。" },
    { "assist.more_jobs", "职业被写进了游戏的表格，却从未真正提供给玩家。" },
    /* battles */
    { "battle.pause_down", "（属于战斗暂停的一部分）" },
    { "battle.pause_hurt", "（属于战斗暂停的一部分）" },
    { "battle.pause_foes", "（属于战斗暂停的一部分）" },
    { "battle.improved", "在下达指令之间，原作中的队员要么闲站着，要么战斗表现很差（射手会冲进近战，没有火力线就僵住）；这正是玩家每次都会下达的指令。" },
    { "battle.click_attack", "（属于更易操作的战斗控制的一部分）" },
    { "battle.volley_toggle", "（属于更易操作的战斗控制的一部分）" },
    { "battle.panel_select", "（属于更易操作的战斗控制的一部分）" },
    { "battle.quick_orders", "（属于更易操作的战斗控制的一部分）" },
    { "battle.next_member", "（属于更易操作的战斗控制的一部分）" },
    { "battle.order_bar", "（属于更易操作的战斗控制的一部分）" },
    { "battle.order_icons", "（属于更易操作的战斗控制的一部分）" },
    { "battle.drag_pan", "（属于更易操作的战斗控制的一部分）" },
    { "battle.select_stays", "（属于更易操作的战斗控制的一部分）" },
    { "battle.end_words", "（属于更易操作的战斗控制的一部分）原作最后漫长的等待看起来就像卡住了。" },
    { "battle.smooth_scroll", "（属于流畅、稳定的滚动的一部分）" },
    { "battle.scroll_paced", "（属于流畅、稳定的滚动的一部分）" },
    { "battle.scroll_rest", "（属于流畅、稳定的滚动的一部分）" },
    { "battle.info", "（属于屏幕上的战斗信息的一部分）" },
    { "battle.los_line", "（属于屏幕上的战斗信息的一部分）" },
    { "battle.click_hint", "（属于屏幕上的战斗信息的一部分）" },
    { "battle.foe_arrow", "（属于屏幕上的战斗信息的一部分）" },
    { "battle.skill_gains", "（属于屏幕上的战斗信息的一部分）" },
    { "battle.reload_bar", "（属于屏幕上的战斗信息的一部分）" },
    { "battle.log", "（属于屏幕上的战斗信息的一部分）" },
    { "battle.info_numbers", "更容易：那些队伍无法得知的确切数字。" },
    { "battle.foes_left", "更容易：那些队伍本不可能掌握的知识。" },
    { "assist.level_map", "地下城和城堡如果没有地图，很难找到路。" },
    { "assist.level_map_all", "更容易：那些尚未见到的部分。" },
    { "fix.pathfinding", "在原作中，战士们会困在矿井和长长的房间里。" },
    { "battle.speed", "这是个口味问题：有些人觉得原作的战斗节奏太慢。" },
    { "battle.fast_key", "（属于更易操作的战斗控制的一部分）已经赢下的战斗反而要花很长时间才能结束。" },
    /* travel */
    { "travel.auto", "一步步在陆地视图上移动需要按很多键，而这些选择其实都不是真正的选项。" },
    { "travel.fast", "这也是个人喜好问题。" },
    { "map.names", "原作的陆地视图什么都没标注。" },
    { "map.quest_marks", "任务通向何方，只有在玩家的笔记里才有记载。" },
    { "map.offers_unvisited", "更容易：那些队伍本不可能掌握的知识。" },
    { "map.key_m", "原作根本没有整片土地的地图。" },
    { "travel.encounters", "更容易：减少干扰。" },
    { "travel.street_encounters", "更容易：原作的夜间街道几乎都是埋伏点。" },
    /* controls, tools, elsewhere */
    { "display.crisp_text", "原作的小字在大屏幕上很难看清。" },
    { "display.hd_art", "替代图片，供那些制作它们的人使用。" },
    { "sound.effects", "这也是个人喜好问题。" },
    { "display.margins", "宽屏有原作留下的黑色区域。" },
    { "display.full_names", "原作为了适应屏幕，把名字都截短了。" },
    { "display.armour_worn", "V:/L：前缀晦涩难懂。" },
    { "saves.quick", "保存需要经过好几个菜单。" },
    { "saves.ironman", "玩家们希望有一款每个选择都真正有意义的游戏。" },
    { "saves.autosave_arrive", "一次崩溃或一场糟糕的战斗就能耗费数小时。" },
    { "saves.autosave_battle", "一场失败的战斗，代价了此前的一切。" },
    { "saves.restart_on_end", "原版在最后会退出到 DOS。" },
    /* restored cut content */
    { "rules.fehde", "由 MicroProse 编写，但从未使用过。" },
    { "rules.hospice", "由 MicroProse 编写，但从未可玩。" },
    { "rules.war", "由 MicroProse 编码，但从未启用。" },
    { "rules.politics", "由 MicroProse 编写，但从未启用。" },
    { "battle.black_cloud", "1992 年因机器速度问题而被剔除；规则已对此作出处理。" },
    { "assist.hall_of_fame", "在游戏文件中未完成。" },
};

/* switches no longer offered: their keys are not to be used for anything else (settings may still
   hold them; nothing reads them) */
const char *const OPTIONS_RETIRED[] = {
    "display.panel_losses",          /* the party panel's pale losses: always shown with the new panel */
    "battle.bars", "battle.bars_hurt",   /* bars over every fighter: now only over wounded enemies (battle.info) */
    "map.icon_set",                  /* the world map's place pictures: the seals only */
    "map.boat_style",                /* boat routes: red ink with ships only */
    "display.card_numbers",          /* the card options' key marks: red numerals only */
    "party.new_member_looks", "party.keep_looks",   /* members' own sprites and colors: always, but in the original game */
    "party.more_figures", "party.figure_picker",    /* enemy sprites offered, in a picker: always, but in the original game */
    "assist.level_map_walk",         /* a click on the level map walks there: always, with the map */
    "assist.stairs_together",        /* the party takes the stairs together: always, but in the original game */
    "assist.wheel_lists", "assist.menu_click", "input.gamepad",   /* controls: always, but in the original game (the controller always) */
    "editor.show",                   /* the Editor tab: always, but in the original game */
    "assist.loot_all",               /* the loot's and shops' keys: always, as the other keys */
    "display.integer_scale",         /* whole-number scaling: three ways to scale instead (Sharp pixels, Rounded edges, Soft) */
    "display.idle_rest",             /* the game waits without spinning: always */
    "sound.hit_sounds",              /* hits make a sound: the fix "hitsound" */
    0 };

const char *option_why(const Option *o)
{
    if (o->cls == OC_FIX) return "修复了原版的一个错误（说明中指明是哪个）。";
    if (o->cls == OC_LOOK) return "这纯属个人喜好，或与运行的电脑有关；预设设置不会影响它。";
    if (!strncmp(o->key, "mod.", 4)) return "更简单：更注重剧情而非战斗（内置模组）。";
    for (size_t i = 0; i < sizeof WHY / sizeof WHY[0]; i++) if (!strcmp(WHY[i].key, o->key)) return WHY[i].why;
    return "";
}
