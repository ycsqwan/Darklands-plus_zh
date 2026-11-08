// ui_windows.cpp - the interface's side windows: About, the welcome on the first start, getting the
// MT-32 working, and F11's keys and clicks for the screen in view.
// Split from ui/ui.cpp, which opens them (its flags open_help, open_welcome, open_mt32).
#include <SDL.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <string>
#include "padglyphs.h"
#include "imgui.h"
#include "tablesort.h"
#include "skillicons.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "drawcache.h"
#include "gamedata.h"
#include "quests.h"
#include "uiinternal.h"
#include "uitabs.h"
#include "townicons.h"
#include "libraries.h"

extern "C" int saves_loading_cover(void);   /* mods/saves.c */
void ui_draw_hourglass(ImDrawList *dl, ImVec2 tl, float kx, float k, int hour, int day, int month);   /* ui/hourglass.cpp */
extern "C" void banner_draw(void);                                   /* ui/banner.cpp */
extern "C" void worldmap_icon_sheet_draw(void); extern "C" void orn_test_sheet_draw(void); extern "C" void niche_test_sheet_draw(void); extern "C" void clock_test_sheet_draw(void);                      /* ui/worldmap.cpp (a test) */
extern "C" {
#include "../mods/options.h"
#include "../runtime/cpu.h"
#include "../runtime/game.h"
#include "../runtime/config.h"
#include "../mods/mods.h"
#include "../mods/assist.h"
#include "../mods/cardview.h"
#include "../mods/travel.h"
#include "../mods/fixes.h"
#include "../mods/saves.h"
#include "../mods/hdtext.h"
#include "../mods/musicpack.h"
#include "../mods/hdart.h"
#include "../mods/sfx.h"
#include "../mods/datamods.h"
#include "../mods/lootview.h"
#include "../mods/sheetview.h"
#include "ui.h"
#include "money.h"
extern char game_dir[1024];
extern const unsigned char FONT_MEDIUM[], FONT_SEMIBOLD[], FONT_ALEGREYA[], FONT_FELL[], FONT_MEDIEVAL[], FONT_ATKINSON[], FONT_FRAKTUR[], FONT_GRENZE[], FONT_COMIC[];
extern const unsigned int FONT_MEDIUM_SIZE, FONT_SEMIBOLD_SIZE, FONT_ALEGREYA_SIZE, FONT_FELL_SIZE, FONT_MEDIEVAL_SIZE, FONT_ATKINSON_SIZE, FONT_FRAKTUR_SIZE, FONT_GRENZE_SIZE, FONT_COMIC_SIZE;
}
#include "ornament.h"

extern bool open_help, open_welcome, open_mt32;
static void bulletf(const char *fmt, ...)   /* a bullet whose text wraps (ImGui's BulletText does not) */
{
    va_list ap; va_start(ap, fmt); ImGui::Bullet(); ImGui::TextWrappedV(fmt, ap); va_end(ap);
}
#include "ui_decls.h"
extern ImFont *ui_font_small, *ui_font_text, *ui_font_overlay, *ui_font_head, *ui_font_bold;
extern ImFont *ui_font_game[UI_GAME_FONTS], *ui_font_game_hi[UI_GAME_FONTS];
extern int ui_game_font, ui_text_edge, ui_sys_cursor, ui_dirty_config;
extern float ui_scale;
extern SDL_Window *ui_window;
extern ImFont *font_body, *font_head, *font_tab, *font_tab_small;   /* (ui/ui.cpp) */

extern "C" const char dlp_version[];
void tab_about(void)
{
    ImGui::PushFont(font_head); ImGui::Text("Darklands+"); ImGui::PopFont();
    ImGui::SameLine(); ImGui::TextDisabled("构建 %s", dlp_version);
    ImGui::TextWrapped("Darklands+由Yassine Meskhout制作。");
    ImGui::TextWrapped("Darklands（c）1992 MicroProse。这是原版游戏，现已重新构建，可直接在当今电脑上运行，使用的是"
                       "您自己副本中的游戏文件。F12 > 设置中有“完整原版”预设，即游戏最初运行时的状态，并提供了Darklands+所新增功能的大部分开关选项。");
    ImGui::Spacing();
    ImGui::TextWrapped("F11：当前屏幕的快捷键。F12：此窗口（打开时游戏会暂停）。Alt+Enter：全屏模式。"
                       "Ctrl+F12：截图。Ctrl+S：快速保存。F9或Ctrl+L：快速载入。M：世界地图。G：前往（城市内）。");
    ImGui::PushFont(font_head); orn_septext("文件"); ImGui::PopFont();
    {   extern char dos_shadow[];
        if (dos_shadow[0]) bulletf("存档及其他所有游戏生成的文件：%s（Darklands+无法写入游戏目录；但其SAVES文件夹中的存档仍可正常加载）"
                                             "存档：位于游戏目录下的SAVES文件夹，与原版一致。", dos_shadow);
        else bulletf("保存的游戏：游戏文件夹中的 SAVES 文件夹，与原版一致"); }
    {   extern char dlp_data_dir[];
        bool own = strcmp(dlp_data_dir, game_dir) != 0;    /* the game folder could not be written */
        bulletf("游戏：%s", game_dir);
        bulletf("设置：%s 中的 darklands_port.ini", own ? dlp_data_dir : "游戏文件夹");
        extern char dlp_user_dir[];
        bulletf("截图（Ctrl+F12，以您看到的画面为准）：位于%s中的screenshots文件夹", own ? dlp_user_dir : "游戏文件夹");
        bulletf("会话日志：位于%s中的darklands_port.log（Windows）；下方的“报告问题”按钮会将其与报告所需的其他内容一同打包成zip文件", own ? dlp_data_dir : "游戏文件夹");
        bulletf("游戏程序：首次启动时由您的DARKLAND.EXE生成：%s（删除后可重新生成）", own ? dlp_data_dir : "游戏文件夹");
#ifdef DL_RTSPLIT
        bulletf("游戏程序，首次启动时由您的 DARKLAND.EXE 生成：%s（删除后将重新生成）",
#ifdef _WIN32
                          "darklands_game.dll");
#else
                          "libdarklands_game.so");
#endif
#endif
    }
    ImGui::PushFont(font_head); orn_septext("制作"); ImGui::PopFont();
    { void ui_report_section(void); ui_report_section(); }   /* (ui/report.cpp) */
    ImGui::PushFont(font_head); orn_septext("使用"); ImGui::PopFont();
    ui_hint("Dear ImGui（MIT许可证）；Nuked OPL3，Sound Blaster FM合成（LGPL 2.1）；Munt的libmt32emu，Roland MT-32（LGPL 2.1）；stb_image、stb_image_write、"
         "stb_vorbis（公有领域）；SDL2（zlib 许可证）；字体 EB Garamond、Alegreya、Atkinson Hyperlegible、IM Fell English，"
         "MedievalSharp、UnifrakturMaguntia、Grenze Gotisch 和 Comic Neue（SIL 开源字体许可证 1.1）。");
    ui_hint("Darklands 由 Arnold Hendrick 设计，MicroProse 制作；此处的一切均基于他们的游戏。");
    ui_hint("感谢 Darklands 社区：vvendigo 的工具、Merle 与 Joel “Quadko” McIntyre 对文件格式的文档，"
         "Darklands Companion 以及 Arkana Mechanika（Darklands 修复版）提供的错误报告。");
}

/* F11: the keys and clicks of the moment (F1 ... F5 are the game's: a member's sheet) (the game's own and Darklands+'s) */
/* the first start: what Darklands+ is, the keys that matter, and the choice to play the
 * original game untouched (the purists' switch), once */
void welcome_window(void)
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, 140));
    float w = fminf(io.DisplaySize.x * 0.9f, UIS(760));
    ImGui::SetNextWindowSize(ImVec2(w, 0), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushFont(font_body);
    if (ImGui::Begin("欢迎来到Darklands+", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
        extern char dos_shadow[];
        if (dos_shadow[0])
            ImGui::TextWrapped("这是 Darklands（MicroProse，1992），已重新构建，可在当今电脑上直接运行，并使用您自己副本的游戏文件。"
                               "Windows 不允许程序写入其安装的游戏文件夹，因此您的存档会保存在 %s 中，"
                               "（已存放在游戏文件夹中的存档仍可照常加载）。", dos_shadow);
        else
            ImGui::TextWrapped("这是 Darklands（MicroProse，1992），已重新构建，可在当今电脑上直接运行，并使用您自己副本的游戏文件。"
                               "您的存档依然保留在游戏文件夹中，一如既往。");
        ImGui::Spacing();
        ImGui::TextWrapped("Darklands+ 增加了玩家长久以来的期待：清晰的文字、卡牌上的机会、全队伍的角色表，"
                           "日记、世界地图、战斗日志、快速存档以及旧有漏洞的修复。请在下方选择启用多少功能；"
                           "其中大部分还可单独开关。");
        bool done = false;
        {   /* how much of Darklands+: the presets (mods/options.h), the recommended one chosen */
            ImGui::PushFont(font_head); orn_septext("您希望如何游玩？"); ImGui::PopFont();
            static int pick = -1;
            if (pick < 0) { pick = preset_current(); if (pick < 0) pick = PRESET_PLUS; }
            for (int p : PRESET_SHOWN) {                /* (Darklands+ first, Full Original last) */
                ImGui::PushID(p);
                std::string nm = std::string(PRESETS[p].name) + (p == PRESET_PLUS ? "（推荐）" : "");
                if (ImGui::RadioButton("##p", pick == p)) pick = p;
                ImGui::SameLine();
                if (ui_title_desc(nm.c_str(), PRESETS[p].desc)) pick = p;
                ImGui::PopID();
            }
            {   /* Ironman, as on Settings' Preset page (its home): for the next new game, whatever the preset */
                ImGui::PushID("iron");
                bool im = config_get_int("saves.ironman", 0) != 0, click = false;
                if (ImGui::Checkbox("##iron", &im)) click = true;
                ImGui::SameLine();
                if (ui_title_desc("铁人模式", "一种存档，由游戏本身随您进程自动保存，无需再次加载。适用于您的下一场新游戏，三种模式任选。")) { im = !im; click = true; }
                if (click) { config_set_int("saves.ironman", im); config_save(); }
                ImGui::PopID();
            }
            ImGui::TextColored(ImVec4(0.18f, 0.40f, 0.16f, 1), "无论您选择哪一种，存档都与原版游戏保持兼容。");
            if (ui_pad_prompts()) ui_pad_text("您可随时更改设置，或单独开启/关闭各项功能：{START} > 设置。{LSTICK} 移动光标，{A} 点击。", true);
            else ImGui::TextDisabled("您可随时更改设置，或单独开启/关闭各项功能：F12 > 设置。");
            ImGui::Spacing();
            if (ImGui::Button("游戏") || ImGui::IsKeyPressed(ImGuiKey_Enter)) { if (pick != preset_current()) preset_apply(pick); done = true; }
        }
        ImGui::Spacing();
        ImGui::PushFont(font_head); orn_septext("键盘"); ImGui::PopFont();
        { ImGui::Bullet(); ImGui::TextWrapped("%s", "F12：Darklands+ 窗口（窗口打开时游戏暂停）"); }
        { ImGui::Bullet(); ImGui::TextWrapped("%s", "F11：当前屏幕的按键与点击操作，包括游戏隐藏的功能"); }
        { ImGui::Bullet(); ImGui::TextWrapped("%s", "左侧点击队伍成员（战斗中右键：点击后可为其下达指令）：查看其角色表，包含全队的圣人与配方"); }
        { ImGui::Bullet(); ImGui::TextWrapped("%s", "M：世界地图，用于寻找地点并前往"); }
        { ImGui::Bullet(); ImGui::TextWrapped("%s", "右键：游戏菜单栏（存档、载入、战斗中的指令）"); }
        { ImGui::Bullet(); ImGui::TextWrapped("%s", "Alt+Enter：全屏   Ctrl+S：快速存档   F9：快速载入"); }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) done = true;
        if (done) { open_welcome = false; config_set_int("ui.welcome_seen", 1); config_save(); }
    }
    ImGui::End();
    ImGui::PopFont();
}

/* Darklands was made for the Roland MT-32: say so, and help find the ROM files */
extern "C" int plat_choose_folder(const char *title, char *out, int n);
void mt32_window(void)
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, 140));
    float w = fminf(io.DisplaySize.x * 0.92f, UIS(780));
    ImGui::SetNextWindowSize(ImVec2(w, 0), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushFont(font_body);
    if (ImGui::Begin("《Darklands》在 Roland MT-32 上听起来最佳", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextWrapped("《Darklands》是为 Roland MT-32 编写的，那是1987年的一台合成器（或其兄弟型号 LAPC-I，一块插在计算机里的卡）。游戏播放的一切都通过它以音符形式输出，战斗音效也不例外。"
                           "游戏会先向其中载入自己的乐器，因此音乐能按原作编排呈现，带有长笛、弦乐和号角；战斗听起来就像刀剑与盾牌……算是吧：每一击仍是合成器在奏一个音。");
        ImGui::Spacing();
        ImGui::TextWrapped("其他所有声音设置都是替代品。Sound Blaster（当时常见的声卡）播放的音乐版本更简单，战斗音效的模仿也更粗糙；"
                           "而普通合成器（例如 Windows 内置的那个）则完全用错了乐器。");
        ImGui::Spacing();
        ImGui::TextWrapped("《Darklands+》内置了一台 MT-32（Munt 对它的模拟）。它只需要从真机内存中复制两个文件，即其 ROM 文件："
                           "控制区和 PCM 区，MT-32 或后来的 CM-32L 均可（两者都可用，听起来非常相似；若两者都有，则播放 MT-32，除非你在 F12 > 设置 > 声音中选择 CM-32L）。"
                           "它们通常被命名为 MT32_CONTROL.ROM 和 MT32_PCM.ROM（CM-32L 则为 CM32L_CONTROL.ROM 和 CM32L_PCM.ROM）；名称随意，Darklands+ 能通过文件内容识别它们。"
                           "它们属于 Roland，因此无法随本次下载提供。如果你用 Munt 或 DOSBox 搭配 MT-32 玩其他游戏，那么这些文件你早已拥有："
                           "把 Darklands+ 指向那个文件夹即可。");
        ImGui::Spacing();
        ImGui::Separator();
        static char romdir[1024]; static bool init; static std::string msg;
        if (!init) { snprintf(romdir, sizeof romdir, "%s", config_get("sound.mt32_roms", "")); init = true; }
        bool have = mt32_available();
        if (have) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.40f, 0.16f, 1)); ImGui::TextWrapped("%s", mt32_status()); ImGui::PopStyleColor(); }
        else {
#ifdef _WIN32
            if (ImGui::Button("选择包含 ROM 文件的文件夹……")) {
                char d[1024];
                if (plat_choose_folder("包含 MT-32 或 CM-32L ROM 文件（控制区和 PCM 区）的文件夹：", d, sizeof d)) {
                    snprintf(romdir, sizeof romdir, "%s", d); config_set("sound.mt32_roms", romdir); config_save(); mt32_rescan();
                    msg = mt32_available() ? "" : "该文件夹中没有 MT-32 或 CM-32L 的 ROM 文件。";
                }
            }
            ImGui::SameLine(); ImGui::TextDisabled("或手动输入：");
#endif
            ImGui::SetNextItemWidth(UIS(380));
            if (ImGui::InputTextWithHint("##mt32dir", "C:\\path\\to\\the\\roms", romdir, sizeof romdir)) config_set("sound.mt32_roms", romdir);
            ImGui::SameLine();
            if (ImGui::Button("重新查找")) { config_save(); mt32_rescan(); msg = mt32_available() ? "" : "那里还没有找到 ROM 文件。"; }
            if (!msg.empty()) ui_hint("%s", msg.c_str());
            ui_hint("Darklands+ 也会查找 Munt 的 ROM 文件夹、DOSBox Staging 的 mt32-roms 文件夹，以及 Darklands+ 文件夹或游戏文件夹中的“roms”文件夹。");
        }
        ImGui::Spacing();
        bool done = false;
        if (have && !mt32_is_open()) {
            if (ImGui::Button("以 MT-32 重新启动")) { config_save(); plat_restart(); }
            ImGui::SameLine();
        }
        static bool never;
        if (ImGui::Button(have ? "  关闭  " : "  现在不用  ")) done = true;
        if (!have) tips_never_box(&never);
        ImGui::TextDisabled("F12 > 设置 > 声音 中也有以上所有内容。");
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) done = true;
        if (done) { open_mt32 = false; if (never && !have) config_set_int("sound.mt32_prompt", 0); config_save(); }
    }
    ImGui::End();
    ImGui::PopFont();
}

void help_window(void)
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, 100));
    float w = fminf(io.DisplaySize.x * 0.92f, UIS(900));
    ImGui::SetNextWindowSizeConstraints(ImVec2(w, 0), ImVec2(w, io.DisplaySize.y * 0.94f));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushFont(font_body);
    if (ImGui::Begin("按键与点击（F11）", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
        ImGui::SetWindowFontScale(fminf(1.0f, fmaxf(0.7f, io.DisplaySize.y / 1000.0f)));
        auto row = [](const char *k, const char *d) {     /* (a label too long for its column wraps within it) */
            ImGui::BeginGroup();
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + UIS(222));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.18f, 0.10f, 1)); ImGui::TextWrapped("%s", k); ImGui::PopStyleColor();
            ImGui::PopTextWrapPos();
            ImGui::EndGroup();
            ImGui::SameLine(UIS(230)); ImGui::TextWrapped("%s", d);
        };
        int gm = game_mode();
        int sv_loading = 0;
        if (savesview_showing(&sv_loading)) {
            orn_septext(sv_loading ? "当前：载入游戏" : "当前：保存游戏");
            if (sv_loading) { row("双击某个存档", "载入它（或点击它，再点载入）"); row("上、下、Enter", "选择一个存档并载入"); }
            else { row("输入", "为存档命名（最多22个字符；Backspace 删除一个）"); row("Enter 或 保存", "保存"); }
            row("Esc 或 取消", "返回游戏原状");
        } else if (lodgeview_showing()) {          /* (a stay: the game counts it as a card) */
            orn_septext(lodgeview_camping() ? "当前：营地" : "当前：旅店住宿");
            row("一名成员，或 1 … 5", "选择你要安排日程的成员");
            row("J R P A E G T", "恢复 {耐力}（仅放松）、恢复 {力量}、祈祷、炼金术工作、赚点小钱、守卫营地、训练或学习（原版按键）");
            row("S 或 A day", "过去一天；A week：七天；Until rested：力量不足且仅放松的成员改为恢复 {力量}，随后天数流逝直到无人再恢复 {力量}或祈祷");
            row("L、Esc 或 离开", "离开");
        } else if (gm == GM_CARD) {
            orn_septext("当前：卡牌");
            row("点击一个选项", "或按它的编号，1 到 9");
            row("在选项上按住 Shift", "以原版游戏的方式显示成功几率（Darklands+ 的徒标会显示每个选项的几率）");
            row("G", "前往本城的某个地点（Darklands+）");
            row("M", "世界地图（Darklands+）");
            row("城镇图标", "（在右侧边栏）本镇拥有而多数城镇没有的东西，以及其著名商店：指向一个即可查看其提供的商品");
        } else if (gm == GM_MAP) {
            orn_septext("当前：陆地视图");
            row("点击", "朝该地点步行");
            row("Ctrl+点击", "沿道路前往那里（Darklands+；按任意键或点击即可停止）");
            row("M", "世界地图（Darklands+）：寻找一个地点，双击即可前往");
            row("指向任务的图标", "该任务通向那里的原因（城镇旁的闸门或枷锁：城镇拒绝队伍入内，或其守卫想见队伍）");
            row("左侧边栏中的任务", "点击打开它，然后“前往那里”");
            row("城镇图标", "（在右侧边栏，靠近某城镇）该城镇拥有而多数城镇没有的东西，以及其著名商店（指向一个即可查看其提供的商品）");
        } else if (gm == GM_BATTLE) {
            orn_septext("当前：战斗");
            row("空格", "暂停/恢复；暂停时可以下达指令");
            if (!ui_original_mode() && config_get_int("battle.quick_orders", 1)) {
                row("点击一名成员", "然后点击一名敌人（攻击）或一个地点（前往）：该次点击即下达指令");
                row("1 到 5", "选择一名成员；Esc：取消正在下达的指令");
                if (config_get_int("battle.panel_select", 1)) row("左侧的成员信息框", "战斗中，点击：选中他们（双击：视图随之移动）；右键：查看其资料。在城堡、矿井或地城中行走且未遇敌时，点击：查看其资料");
                if (config_get_int("battle.volley_toggle", 1) && config_get_int("battle.improved", 1)) row("成员信息框左下角的图标", "该成员的作战方式，即你下达指令前他的行为：点击它可另选一种");
                row("战场底部的按钮", "被选中成员的指令，位置固定，键位标在按钮上（不可用的指令变暗）；其上方是以网格显示的药水和可投掷或使用的物品。对已在战斗中的成员，会立即对其敌方发动攻击。快进与继续位于右侧边栏顶部");
            } else
                row("点击一名成员", "然后点击一名敌人、一个地点或菜单选项；Enter 下达指令");
            row("Ctrl+点击一名成员", "让该成员成为当前活跃者：视图随之移动（在边缘滚动时会停止）");
            row("指向任意一人", "确认他是谁以及伤势有多重");
            if (levelmap_available()) row("Tab", "显示队伍已探索的楼层地图；点击楼层可让队伍走到那里（Darklands+）");
            if (!ui_original_mode()) row("点击关闭的门", "在战斗外：队伍会走到门前并打开它（Darklands+）");
            if (!ui_original_mode() && config_get_int("battle.fast_key", 1)) row("Z", "快进（六倍速度）/ 返回至所选速度（Darklands+）");
            { extern int ui_battle_log; if (ui_battle_log && !ui_original_mode()) row("战斗日志", "记录每一次攻击与伤口：在宽屏窗口中显示于左边缘，否则位于右下角；F12 > 日记 > 战斗记录可保存全部信息"); }
        } else if (loot_active && lootview_showing()) {
            orn_septext("当前：战利品");
            row("拾取/放回", "在指针所在行上：整堆（1：只拿一件）");
            if (loot_model.shared) { row("全部拾取", "全部放入队伍背包"); row("药水", "交给右侧列表上方显示的成员（各自的）；其余放入队伍背包"); }
            else { row("全部拾取/分发", "全部交给所显示的成员 / 分给已有该物品的人，否则分给空间最多的人（A / S）");
                   row("右侧列表上方的名字，D", "切换到另一位成员"); }
            row("L、Esc 或 离开", "离开（剩下的留在原处）");
        } else if (loot_active) {
            orn_septext("当前：战利品");
            row("G / P", "从堆中取走所选物品 / 放回一件（光标处的列表）");
            row("D", "切换到下一位成员（你拿到的物品会给他）");
            if (loot_enabled()) { row("A", "将整堆拿走给所显示的成员"); row("S", "将整堆分发：每件物品给已有它的人，否则给空间最多的人"); }
            row("L 或 Esc", "离开（剩下的留在原处）");
        } else if (barter_active && barterview_showing()) {
            orn_septext("当前：商店");
            row("购买/出售", "在指针所在行上；按住 Shift：十件/整堆");
            row("你列表上方的名字", "列出的是谁的物品（B：下一位成员）；右键点击名字：该成员掌管钱包");
            row("滚轮、上、下", "滚动指针所在的列表");
            row("L、Esc 或 离开", "离开");
        } else if (barter_active) {
            orn_septext("当前：商店");
            row("P / S", "购买商人所选的物品 / 出售你选择的物品（一次一件）");
            if (loot_enabled()) { row("T", "购买商人所选物品的十件（直到钱用完）"); row("A", "出售你选择物品的整堆"); }
            row("B", "为下一位成员讨价还价");
            row("L 或 Esc", "离开");
        } else if (cacheview_showing()) {
            orn_septext("当前：储物室");
            row("取出/放入", "在指针所在行上：整堆（1：只取一件）");
            row("收好", "所有未装备的物品（在队伍背包下，Darklands+：已保留的物品和药水除外）");
            if (cacheview_shared_pack()) row("药水", "取出的药水会交给右侧列表上方显示的成员；其余放入队伍背包");
            if (cacheview_every_inn()) row("储物室", "在每家旅店都相同（Darklands+；F12 > 设置 > 新屏幕）");
            row("L、Esc 或 离开", "离开");
        } else if (sheetview_showing()) {
            orn_septext("当前：角色面板");
            if (sheet_model.shared) { row("装备/卸下/饮用/保留/丢弃", "在物品页指针所在行上：在队伍背包下（Darklands+），备用物品位于队伍背包中，装备会将物品从背包移到该成员身上并装上（按住 Shift 丢弃：全部）");
                                      row("药水", "每位成员各自拥有：将其拖到左侧的某人身上即可给予");
                                      row("保留", "标记某物品永不被出售或丢弃（其行上有一个小印）");
                                      row("负重", "只有已装备的物品计入负重，与原版一致"); }
            else row("装备/卸下/饮用/给予/丢弃", "在物品页指针所在行上（按住 Shift 给予/丢弃：整堆）");
            row("物品、技能、圣人、配方", "顶部的页面（Tab 或 右：下一页；Shift+Tab 或 左：上一页）");
            row("肖像，F1 … F5", "查看另一位成员的面板；该成员自己的肖像、Esc 或 关闭：返回");
            row("Ctrl+F1 … Ctrl+F5", "让成员 1 … 5 成为队长（或“成为队长”）");
            row("滚轮、上、下", "滚动列表");
        } else if (hdart_on_screen("ARMBACK")) {        /* the character sheet (s031's keys) */
            orn_septext("当前：角色面板");
            row("Ctrl+F1 … Ctrl+F5", "让成员 1 … 5 成为队长（或顶部的“成为队长”）");
            row("上、下、PgUp、PgDn", "在列表中移动（鼠标滚轮也可以）");
            row("1 … 5", "在装备列表中：将所选物品交给成员 1 … 5（按住 Shift：全部）");
            row("A", "在装备列表中：准备或穿上所选物品");
            row("D", "在装备列表中：丢弃所选物品（按住 Shift：全部）");
        }
        orn_septext("原版游戏的按键");
        if (!ui_original_mode()) row("右键", "菜单栏：点击一个条目（或按住按钮并在其上松开）；点击其他地方或按任意键关闭；键盘可用 F10");
        else row("鼠标右键", "菜单栏，按住时显示：在某个条目上松开（键盘可用 F10）");
        row("Alt+S / Alt+L", "保存/载入，并附备注（原版界面列出九个存档）");
        row("Alt+O", "行军顺序");
        row("Alt+D", "难度    Alt+M 音乐    Alt+F 音效");
        row("Alt+C", "圣人变化的消息 开/关");
        row("F6", "队伍信息");
        row("Alt+Q", "退出到桌面");
        orn_septext("Darklands+");
        row("F12", "Darklands+ 窗口（暂停游戏）");
        row("Ctrl+S / F9 或 Ctrl+L", "快速存档/快速载入");
        row("Alt+Enter", "全屏    Ctrl+F12 截图");
        { if ((plat_controller_connected() || ui_pad_prompts()) ) {
            /* the buttons drawn as the controller in use has them (padglyphs.h), else named */
            orn_septext("控制器");
            auto prow = [](const char *keys, const char *d) {     /* keys: "{A} {B}" */
                float x0 = ImGui::GetCursorPosX();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.18f, 0.10f, 1)); ui_pad_text(keys); ImGui::PopStyleColor();
                (void)x0; ImGui::SameLine(UIS(230)); ImGui::TextWrapped("%s", d);
            };
            prow("{LSTICK}", "移动指针（按住左扳机时变慢）");
            prow("{RSTICK}", "鼠标滚轮；按下：Z（战斗中快进）");
            prow("{A} {B}", "点击/右键（菜单栏）");
            prow("{X} {Y}", "Enter / Esc");
            prow("{DPAD}", "方向键");
            prow("{LB} {RB}", "Tab（关卡地图）/ 空格（暂停）");
            prow("{START} {BACK}", "F12（Darklands+ 窗口）/ M（世界地图）");
            prow("{RT}", "按住：Ctrl（在陆地视图中：Ctrl+点击前往那里）");
            prow("{LS}", "按下：F11（此列表）");
        } }
    }
    ImGui::End();
    ImGui::PopFont();
}
