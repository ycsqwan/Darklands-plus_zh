// tips.cpp - the first words of advice, each in a window the game waits behind (ui_is_open): at the
// first battle of a session with Improved combat, how the party fights now (the start screen's note
// pointing to the New Player Guide went with the guide; the welcome, the very first time, stays). Each shows at every
// start until "Never show again" is ticked, which a player can only do from the fourth start
// (ui.launches counts them; the box says how many more times it comes first), so a new player sees them a few times. The MT-32 window before the
// intro (ui/ui_windows.cpp) takes the same box. Not in the test sessions (DL_CTL, DL_SCRIPT)
// unless DL_TIPS; the test command utip:N opens one.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "imgui.h"
#include "padglyphs.h"
#include "uiinternal.h"
#include "uitabs.h"
#include "ornament.h"
extern "C" {
#include "../runtime/cpu.h"
#include "../runtime/game.h"
#include "../runtime/gamevars.h"
#include "../runtime/config.h"
#include "../mods/battlerules.h"
#include "../mods/nscreen.h"
#include "ui.h"
int levelmap_available(void);
u32 plat_ticks_ms(void);
void ui_open_tab(int t);
}
extern ImFont *font_body, *font_head;
extern bool open_welcome;

enum { TIP_NONE, TIP_GUIDE, TIP_BATTLE };
static int open_tip, launches, battle_seen, title_seen;
static u32 title_at;            /* the start screen came: its note a moment later, once it is drawn (the game waits while a window is open) */

static bool quiet(void) { return (getenv("DL_CTL") || getenv("DL_SCRIPT") || getenv("DL_RESTARTED")) && !getenv("DL_TIPS"); }

/* at each start (not a test's, nor the restart after the party's end): one more */
extern "C" void tips_init(void)
{
    launches = config_get_int("ui.launches", 0);
    if (quiet()) return;
    config_set_int("ui.launches", ++launches); config_save();
}

extern "C" int tips_open(void) { return open_tip != TIP_NONE; }

/* "Never show again", for a window shown at every start: greyed until the fourth start, saying how
   many more times the window comes first */
bool tips_never_box(bool *never)
{
    bool can = launches > 3;
    if (!can) *never = false;
    ImGui::BeginDisabled(!can);
    int more = 4 - (launches > 0 ? launches : 1);
    char l[96]; snprintf(l, sizeof l, "永远不再显示（这将再提醒你%d次%s）", more, more == 1 ? "" : "s");
    ImGui::Checkbox(can ? "永远不再显示" : l, never);
    ImGui::EndDisabled();
    return *never;
}

/* the start screen (mods/titleview.c, the native one or the original's): once a session */
extern "C" void tips_title_reached(void)
{
    if (title_seen) return;
    title_seen = 1;
    if (quiet()) return;
    title_at = plat_ticks_ms() | 1;
}
static void title_check(void)
{
    if (!title_at || plat_ticks_ms() - title_at < 700) return;
    title_at = 0;
    fprintf(stderr, "tips: the start screen's note\n");
    if (!config_get_int("ui.welcome_seen", 0)) { open_welcome = true; return; }   /* (the first start: the welcome, which says it too) */
    /* (the note pointing to the New Player Guide: withdrawn with the guide, ui/ui.cpp tab_guide_all) */
}

extern "C" void tips_show(int which) { if (which == 3) open_welcome = true; else if (which == 2) open_tip = which; }   /* (utip:N: 2 the battle tips, 3 the welcome) */

static bool foes_up(void)
{
    for (int i = 5; i < 13; i++) if (g_rb((u16)(GV_combatants + i * 0x80 + 0x14)) == 1) return true;
    return false;
}

/* every frame: the first battle of the session with enemies in it, after a moment (it is drawn) */
static void battle_check(void)
{
    static u32 since;
    if (battle_seen || open_tip || quiet() || !combat_improved() || !config_get_int("tips.battle", 1)) return;
    if (game_mode() != GM_BATTLE || ns_showing() || ui_is_open() || !foes_up()) { since = 0; return; }
    u32 now = plat_ticks_ms();
    if (!since) since = now ? now : 1;
    if (now - since < 800) return;
    battle_seen = 1; open_tip = TIP_BATTLE;
    fprintf(stderr, "tips: the battle tips\n");
}

static void bullet(const char *t) { ImGui::Bullet(); ImGui::TextWrapped("%s", t); }   /* (BulletText does not wrap) */

static bool begin_tip(const char *title, float width)
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, 120));
    ImGui::SetNextWindowSize(ImVec2(fminf(io.DisplaySize.x * 0.92f, UIS(width)), 0), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowFocus();
    ImGui::PushFont(font_body);
    return ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
}
static void end_tip(void) { ImGui::End(); ImGui::PopFont(); }

static void battle_tip(void)
{
    static bool never;
    bool done = false;
    if (begin_tip("战斗，采用改进后的战斗系统", 660)) {
        ImGui::PushFont(font_head); orn_septext("你的部下会在命令之间继续战斗"); ImGui::PopFont();
        bullet("持弓、弩、枪或投掷武器的队员会持续射击；当目标倒下时，会瞄准视野内最近的敌人。");
        bullet("在没有明显遮挡物（如墙、树或友军）的情况下，射手会侧身挪动几步，寻找合适的射击位置。");
        bullet("当敌人靠近、无弹药可射，或为附近遭到围攻或重伤的友军提供支援时，射手会拔出近战武器。");
        bullet("敌人倒下的队员会转向另一名近战敌人；无指令的队员则会加入附近的战斗，除非被命令坚守。");
        ImGui::PushFont(font_head); orn_septext("你的指令"); ImGui::PopFont();
        if (ui_pad_prompts()) ui_pad_text("按键盘上的F11可查看所有按键。", true);
        bullet("按空格键可暂停或继续游戏。暂停时仍可下达指令。");
        if (!ui_original_mode() && config_get_int("battle.quick_orders", 1)) bullet("先点击一名队员，再点击一名敌人（攻击）或一个地点（前往）。");
        else bullet("先点击一名队员，再点击一名敌人、一个地点或菜单选项；按回车键即可执行指令。");
        bullet("点击敌人后，队员可选择攻击方式：针对弱点的攻击，专攻敌甲间隙，只有这种攻击才能穿透敌甲；否则为普通攻击。你从指令中自行选择的攻击方式（普通攻击、弱点攻击、狂暴攻击、格挡）将被保留。");
        std::string keys = "F11列出所有按键";
        if (levelmap_available()) keys += "; Tab键，切换到关卡地图";
        if (config_get_int("battle.fast_key", 1)) keys += "; Z键，快进";
        keys += "."; bullet(keys.c_str());
        ImGui::PushFont(font_head); orn_septext("攻击方式"); ImGui::PopFont();
        bullet("普通攻击：攻守平衡。");
        bullet("弱点攻击：缓慢而精准的打击，专攻敌甲缝隙；穿透力更强（多1至4点），但出现频率较低且命中率稍低。适用于敌甲无法被普通武器突破的情况。");
        bullet("狂暴攻击：快速而果断的打击，完全不考虑防御。用于迅速消灭敌人。");
        bullet("格挡：以防守为主；难以被击中，反击速度较慢且把握度较低。用于在援军到达前牵制敌人，或保护受伤队友。");
        ImGui::Spacing();
        ui_hint("可在F12 > 设置 > 战斗中关闭强化战斗模式。");
        ImGui::Spacing();
        if (ImGui::Button("开始战斗") || ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_Enter)) done = true;
        tips_never_box(&never);
    }
    end_tip();
    if (done) { open_tip = TIP_NONE; if (never) config_set_int("tips.battle", 0); config_save(); }
}

/* every frame, from ui/ui.cpp */
void tips_frame(void)
{
    title_check();
    battle_check();
    if (open_tip == TIP_BATTLE) battle_tip();
}
