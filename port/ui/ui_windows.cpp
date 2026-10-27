// ui_windows.cpp - the interface's side windows: About, the welcome on the first start, the news
// of a new version, getting the MT-32 working, and F11's keys and clicks for the screen in view.
// Split from ui/ui.cpp, which opens them (its flags open_help, open_welcome, open_mt32, open_news).
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

extern bool open_help, open_welcome, open_mt32, open_news;
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
        bulletf("the game: %s", game_dir);
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
        if (done) { open_welcome = false; config_set_int("ui.welcome_seen", 1); if (!news_key().empty()) config_set("ui.news_seen", news_key().c_str()); config_save(); }
    }
    ImGui::End();
    ImGui::PopFont();
}

/* what is new in this version (its first two numbers: "1.2" from "1.2-preview35"; "" for a build
   that is no release), shown once to a player of an earlier one */
extern "C" const char dlp_release[];
std::string news_key(void)
{
    std::string r = dlp_release;
    size_t a = r.find('.'); if (a == std::string::npos) return "";
    size_t b = a + 1; while (b < r.size() && isdigit((unsigned char)r[b])) b++;
    return r.substr(0, b);
}
void news_window(void)
{
    ImGuiIO &io = ImGui::GetIO();
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(0, 0, 0, 140));
    float w = fminf(io.DisplaySize.x * 0.9f, UIS(760));
    ImGui::SetNextWindowSize(ImVec2(w, 0), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::PushFont(font_body);
    std::string title = "Darklands+ 新增内容" + news_key();
    if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
        static const char *NEWS[] = {
            "Darklands+ 窗口（F12）拥有更少、更清晰的标签页：日记、设置、重构内容、模组、编辑器、附录、关于。"
            "设置界面左侧列出了各部分。",
            "任务显示在世界地图上（M），按类型分组并优先显示最近的任务，同时提供窗口以查找炼金术原料及各类物品。",
            "存档与载入均在游戏自带的存档与载入界面进行，界面全新绘制，选项亦在此呈现。",
            "改进后的战斗：在您下达指令期间，射手持续射击，为确保视线畅通而让出位置，并近身格斗以协助被攻击的队友；"
            "无指令的队员也可加入战斗。下达指令只需一次点击（先选队员，再选敌人或目标）。首次战斗将详细说明操作。",
            "卡牌上每个机会旁均标注尝试者及其依据（技能、属性、城镇舆论、声望）。",
            "铁人模式：一种存档，由游戏随您进程自动保存，无需再次加载（F12 > 设置 > 预设，适用于您的下一场新游戏）。",
            "整个配乐可在 F12 > 设置 > 声音中播放，选择您偏好的设备。",
            "更多界面全新绘制：旅店与营地、炼金术师的书桌、队伍信息、行军命令（拖动队员至指定位置）、敌人信息。",
            "城市政治、交战城市及其他 MicroProse 曾撰写却从未启用的内容，均由 Darklands+ 重构（实验性，默认全部关闭，"
            "直到您勾选启用）：F12 > 构建内容。",
            "控制器按键在屏幕上显示并可自定义：F12 > 设置 > 控制器。",
            "您的存档将在每个新版本首次启动时复制到“Darklands+ 存档备份”中。" };
        for (const char *t : NEWS) { ImGui::Bullet(); ImGui::TextWrapped("%s", t); }
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("大多数改动都可以在F12 > 设置中关闭（“完整原版”预设会关闭所有影响游戏体验的改动）。F11列出当前屏幕的按键。");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        bool done = ImGui::Button("返回游戏") || ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Escape);
        if (done) { open_news = false; config_set("ui.news_seen", news_key().c_str()); config_save(); }
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
    if (ImGui::Begin("Darklands在罗兰MT-32上听起来效果最佳", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextWrapped("Darklands是为罗兰MT-32这款1987年的合成器编写的（或者它的兄弟型号LAPC-I，一种内置于电脑中的扩展卡）。游戏中的一切音效都以音符的形式经过它处理，战斗音效也不例外。游戏会先将自身内置的乐器载入其中，因此音乐听起来就像精心创作的一样，"
                           "有长笛、弦乐和号角，而战斗声则像是刀剑与盾牌……大致如此：每一次击打其实都是合成器在弹奏一个音符。"
                           "其他所有的声音设置都只是替代品。当时的主流声卡Sound Blaster只能播放更简单的音乐版本，并且对战斗音效的模仿也较为粗糙；而普通的合成器，比如Windows自带的那种，则完全使用了");
        ImGui::Spacing();
        ImGui::TextWrapped("错误的乐器。"
                           "Darklands+内置了一台MT-32（Munt对其的仿制品）。它只需要从真正的MT-32内存中复制两个文件——其ROM文件："
                           "wrong instruments altogether.");
        ImGui::Spacing();
        ImGui::TextWrapped("这两个文件通常被命名为MT32_CONTROL.ROM和MT32_PCM.ROM（如果是CM-32L，则分别为CM32L_CONTROL.ROM和CM32L_PCM.ROM）；尽管名称可以随意取，Darklands+依然能通过文件内容识别它们。"
                           "它们属于罗兰公司，因此无法随本次下载提供。"
                           "如果你用Munt或DOSBox搭配MT-32玩其他游戏，那么这些文件你早已拥有："
                           "把Darklands+指向那个文件夹即可。"
                           "选择包含ROM文件的文件夹……"
                           "包含MT-32或CM-32L ROM文件（控制区和PCM区）的文件夹：");
        ImGui::Spacing();
        ImGui::Separator();
        static char romdir[1024]; static bool init; static std::string msg;
        if (!init) { snprintf(romdir, sizeof romdir, "%s", config_get("sound.mt32_roms", "")); init = true; }
        bool have = mt32_available();
        if (have) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.40f, 0.16f, 1)); ImGui::TextWrapped("%s", mt32_status()); ImGui::PopStyleColor(); }
        else {
#ifdef _WIN32
            if (ImGui::Button("该文件夹中没有MT-32或CM-32L的ROM文件。")) {
                char d[1024];
                if (plat_choose_folder("或者手动输入：", d, sizeof d)) {
                    snprintf(romdir, sizeof romdir, "%s", d); config_set("sound.mt32_roms", romdir); config_save(); mt32_rescan();
                    msg = mt32_available() ? "" : "目前尚未找到任何ROM文件。";
                }
            }
            ImGui::SameLine(); ImGui::TextDisabled("或者直接输入：");
#endif
            ImGui::SetNextItemWidth(UIS(380));
            if (ImGui::InputTextWithHint("##mt32dir", "C:\\path\\to\\the\\roms", romdir, sizeof romdir)) config_set("sound.mt32_roms", romdir);
            ImGui::SameLine();
            if (ImGui::Button("再次查看")) { config_save(); mt32_rescan(); msg = mt32_available() ? "" : "使用MT-32重新启动"; }
            if (!msg.empty()) ui_hint("%s", msg.c_str());
            ui_hint("关闭");
        }
        ImGui::Spacing();
        bool done = false;
        if (have && !mt32_is_open()) {
            if (ImGui::Button("现在：保存游戏")) { config_save(); plat_restart(); }
            ImGui::SameLine();
        }
        static bool never;
        if (ImGui::Button(have ? "现在：加载游戏" : "暂不")) done = true;
        if (!have) tips_never_box(&never);
        ImGui::TextDisabled("加载它（或者先点击再选择“加载”）");
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
    if (ImGui::Begin("上下键、回车键", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar)) {
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
            orn_septext(sv_loading ? "选择一个存档并加载" : "输入");
            if (sv_loading) { row("为存档命名（最多22个字符；按Backspace可删除一个）", "按Enter或Save"); row("保存它", "Esc或取消"); }
            else { row("返回到游戏初始状态", "现在：营地"); row("现在：旅店住宿", "保存它"); }
            row("Esc 或 取消", "返回游戏初始状态");
        } else if (lodgeview_showing()) {          /* (a stay: the game counts it as a card) */
            orn_septext(lodgeview_camping() ? "现在：一个营地" : "现在：在旅店休息");
            row("一名成员，或 1 … 5", "选择你要安排一天的成员");
            row("J R P A E G T", "恢复耐力（只需放松）、恢复力量、祈祷、炼金术工作、赚点钱、守营、训练或学习（原版的按键）");
            row("S 或 A 的一天", "一天过去；一周：七天；直到休息为止：那些力量不足、只放松的成员会切换为恢复力量，然后直到没有人再恢复力量或祈祷为止");
            row("L、Esc 或离开", "leave");
        } else if (gm == GM_CARD) {
            orn_septext("现在：一张卡片");
            row("点击一个选项", "或按下其编号，1 到 9");
            row("将 Shift 键按住某个选项上", "成功几率，按照原版游戏显示（Darklands+ 的徽章会显示每个选项的成功几率）");
            row("G", "前往这座城市中的某个地点（Darklands+）");
            row("M", "世界地图（Darklands+）");
            row("城镇图标", "（在右侧边栏）这个城镇拥有的、大多数城镇所缺乏的东西，以及其著名的商店：指向其中一个即可查看它提供的服务");
        } else if (gm == GM_MAP) {
            orn_septext("现在：陆地视图");
            row("单击", "向那个地点走去");
            row("Ctrl+点击", "沿着道路前往那里（Darklands+；任何按键或点击都会停止）");
            row("M", "世界地图（Darklands+）：找到一个地点，双击即可前往");
            row("指向某个任务的图标", "引导你前往那里的任务（城镇旁的吊闸或枷锁：城镇禁止队伍进入，或其守卫想要队伍）");
            row("左侧边栏中的一个任务", "点击打开它，然后前往");
            row("城镇图标", "（在右侧边栏、靠近某个城镇的地方）这个城镇拥有的、大多数城镇所缺乏的东西，以及其著名的商店（指向其中一个即可查看它提供的服务）");
        } else if (gm == GM_BATTLE) {
            orn_septext("现在：一场战斗");
            row("空间", "暂停/继续；暂停时仍可下达命令");
            if (!ui_original_mode() && config_get_int("battle.quick_orders", 1)) {
                row("点击一名成员", "然后点击一名敌人（攻击）或一个地点（前往）：这次点击即下达命令");
                row("1 到 5", "选择一名成员；Esc：取消正在下达的命令");
                if (config_get_int("battle.panel_select", 1)) row("左侧的成员信息框", "点击：选中他们（双击：视图随之移动到他们身上）；右键：查看他们的资料");
                if (config_get_int("battle.volley_toggle", 1)) row("成员信息框上的弓或剑", "当你点击敌人时，该成员会射击或进行近身格斗（点击切换）");
                row("场地底部的按钮", "被选中成员的命令，分组排列（攻击、移动、在此、药剂与物品、战斗）；对于已经交战的成员，会立即对其敌方发动攻击。快进与继续位于右侧边栏的顶部");
            } else
                row("点击一名成员", "然后点击一名敌人、一个地点或菜单选项；Enter 键下达命令");
            row("Ctrl+点击一名成员", "让该成员成为当前活跃者：视图随之移动（边缘滚动时会停止）");
            row("指向任意一人", "确认他是谁以及伤势有多重");
            if (levelmap_available()) row("切换标签页", "显示队伍已探索过的关卡地图（Darklands+）");
            if (!ui_original_mode() && config_get_int("battle.fast_key", 1)) row("Z", "快进（速度为正常六倍）/ 返回至所选速度（Darklands+）");
            { extern int ui_battle_log; if (ui_battle_log && !ui_original_mode()) row("战斗日志", "记录每一次攻击与伤口：在宽屏窗口中显示于左边缘，否则位于右下角；按F12进入“日志”→“战斗记录”即可保存全部信息"); }
        } else if (loot_active && lootview_showing()) {
            orn_septext("现在：战利品");
            row("取出 / 放回", "在指针下方的行上：整叠物品（1：仅取一件）");
            if (loot_model.shared) { row("全部取出", "将所有物品放入队伍背包"); row("药剂", "转到右侧列表上方显示的成员（每位成员专属）；其余物品放入队伍背包"); }
            else { row("全部取出 / 分配", "所有物品分配给显示的成员 / 或分配给已有物品者，若无人持有则分配给空间最大的人（A / S）");
                   row("右侧列表上方显示姓名，D", "切换至其他成员"); }
            row("L、Esc 或离开", "离开（剩余物品留在原地）");
        } else if (loot_active) {
            orn_septext("现在：战利品");
            row("G / P", "从堆中取出选定物品 / 放回一件（光标所在列表）");
            row("D", "切换至下一位成员（即获得你所取物品的人）");
            if (loot_enabled()) { row("A", "为显示的成员取走整堆物品"); row("S", "分配整堆物品：每件物品分给已有者，若无人持有则分给空间最大的人"); }
            row("L或Esc", "离开（剩余物品留在原地）");
        } else if (barter_active && barterview_showing()) {
            orn_septext("现在：商店");
            row("买卖", "在指针下方的行上；按Shift可选择十件 / 整叠物品");
            row("你的列表上方显示姓名", "哪些人的物品被列出（B：下一位成员）；右键点击某姓名：该成员掌管钱包");
            row("鼠标滚轮、向上、向下", "滚动指针下方的列表");
            row("L、Esc 或离开", "leave");
        } else if (barter_active) {
            orn_septext("现在：商店");
            row("P / S", "购买商人选定的物品 / 出售你选定的物品（每次一件）");
            if (loot_enabled()) { row("T", "购买商人选定的十件物品（资金允许范围内）"); row("A", "出售你选定物品的整叠"); }
            row("B", "与下一位成员以物易物");
            row("L或Esc", "leave");
        } else if (cacheview_showing()) {
            orn_septext("现在：储物间");
            row("取出 / 放入", "在指针下方的行上：整叠物品（1：仅取一件）");
            row("收起", "所有未装备的物品（随队伍背包携带，Darklands+：除保留物品与药水外）");
            if (cacheview_shared_pack()) row("药剂", "服用的药水会分配给右侧列表上方显示的成员；其余物品放入队伍背包");
            if (cacheview_every_inn()) row("仓库", "各旅店均同此规则（Darklands+；F12 > 设置 > 新界面）");
            row("L、Esc 或离开", "leave");
        } else if (sheetview_showing()) {
            orn_septext("现在：角色面板");
            if (sheet_model.shared) { row("装备 / 除装 / 饮用 / 保留 / 抛弃", "在物品页面指针下方的行上：使用共享背包（Darklands+）时，备用物品在队伍的背包里，装备会将物品从背包移至该成员并装备（按Shift抛弃：全部）");
                                      row("药剂", "每个成员自己的：将一件拖到左侧某人身上即可赠送");
                                      row("保留", "标记某件物品永不出售或丢弃（其所在行上的小印章）");
                                      row("负重", "只有已装备的物品计入负重，与原版相同"); }
            else row("装备 / 除装 / 饮用 / 赠送 / 抛弃", "在物品页面指针下方的行上（按Shift赠送和抛弃：整叠物品）");
            row("物品、技能、圣人、配方", "顶部的页面（Tab或右键：下一页；Shift+Tab或左键：上一页）");
            row("肖像、F1…F5", "另一个成员的界面；本成员的肖像，Esc或关闭：返回");
            row("Ctrl+F1 ... Ctrl+F5", "将成员1…5设为队长（或设为队长）");
            row("鼠标滚轮、向上、向下", "滚动列表");
        } else if (hdart_on_screen("ARMBACK")) {        /* the character sheet (s031's keys) */
            orn_septext("现在：角色面板");
            row("Ctrl+F1 ... Ctrl+F5", "将成员1…5设为队长（或在顶部设为队长）");
            row("上、下、PgUp、PgDn", "在列表中移动（鼠标滚轮亦可）");
            row("1 ... 5", "在装备列表中：将选定物品赠予成员1…5（按Shift：全部）");
            row("A", "在装备列表中：准备或穿戴选定物品");
            row("D", "在装备列表中：抛弃选定物品（按Shift：全部）");
        }
        orn_septext("原版游戏的按键");
        if (!ui_original_mode()) row("右键单击", "菜单栏：点击某项（或按住按钮后松开）；其他位置点击或任意按键均会关闭；键盘F10");
        else row("右键", "菜单栏，按住时：松开某项（键盘F10）");
        row("Alt+S / Alt+L", "保存/载入，并可添加备注（原版屏幕列出九个存档）");
        row("Alt+O", "行军顺序");
        row("Alt+D", "难度    Alt+M 音乐    Alt+F 音效");
        row("Alt+C", "关于圣人状态切换的消息开关");
        row("F6", "队伍信息");
        row("Alt+Q", "退出到桌面");
        orn_septext("Darklands+");
        row("F12", "Darklands+窗口（暂停游戏）");
        row("Ctrl+S / F9 或 Ctrl+L", "快速存档 / 快速载入");
        row("Alt+Enter", "全屏    Ctrl+F12 截图");
        { if ((plat_controller_connected() || ui_pad_prompts()) ) {
            /* the buttons drawn as the controller in use has them (padglyphs.h), else named */
            orn_septext("控制器");
            auto prow = [](const char *keys, const char *d) {     /* keys: "{A} {B}" */
                float x0 = ImGui::GetCursorPosX();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.18f, 0.10f, 1)); ui_pad_text(keys); ImGui::PopStyleColor();
                (void)x0; ImGui::SameLine(UIS(230)); ImGui::TextWrapped("%s", d);
            };
            prow("{LSTICK}", "移动指针（在按住左扳机时缓慢移动）");
            prow("{RSTICK}", "鼠标滚轮；按下时：Z（战斗中快进）");
            prow("{A} {B}", "点击 / 右键点击（菜单栏）");
            prow("{X} {Y}", "回车 / Esc");
            prow("{DPAD}", "方向键");
            prow("{LB} {RB}", "Tab（关卡地图）/ 空格（暂停）");
            prow("{开始} {返回}", "F12（Darklands+窗口）/ M（世界地图）");
            prow("{RT}", "按住时：Ctrl（在陆地视图中：Ctrl+点击即可前往）");
            prow("{LS}", "按下时：F11（此列表）");
          } }
        ImGui::TextDisabled("F11、Esc 或点击均可关闭此界面。");
    }
    ImGui::End();
    ImGui::PopFont();
}

