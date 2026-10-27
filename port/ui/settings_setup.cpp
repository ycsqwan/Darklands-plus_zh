// settings_setup.cpp - the Settings page's sections on the game on this computer (ui/settings.cpp
// draws the page): Screen, Intro, Sound and Controller.
#include "../mods/playlist.h"
#include <SDL.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <string>
#include <vector>
#include <functional>
#include "imgui.h"
#include "padglyphs.h"
#include "../mods/fixgroups.h"
#include "skillicons.h"
#include "gamepics.h"
#include "ornament.h"
#include "tablesort.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "gamedata.h"
#include "quests.h"

extern "C" {
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
void soundpreview_start(const char *dev, int what); void soundpreview_stop(void); int soundpreview_playing(const char *dev, int what); int soundpreview_possible(void);
void musicpack_preview(int n); int musicpack_first_piece(void); void sfx_preview(int k); u32 plat_ticks_ms(void);
#include "../mods/datamods.h"
#include "../mods/lootview.h"
#include "../mods/sheetview.h"
#include "../mods/options.h"
#include "ui.h"
#include "money.h"
extern char game_dir[1024];
extern const unsigned char FONT_MEDIUM[], FONT_SEMIBOLD[], FONT_ALEGREYA[], FONT_FELL[], FONT_MEDIEVAL[], FONT_ATKINSON[], FONT_FRAKTUR[], FONT_GRENZE[];
extern const unsigned int FONT_MEDIUM_SIZE, FONT_SEMIBOLD_SIZE, FONT_ALEGREYA_SIZE, FONT_FELL_SIZE, FONT_MEDIEVAL_SIZE, FONT_ATKINSON_SIZE, FONT_FRAKTUR_SIZE, FONT_GRENZE_SIZE;
}

#include "uiinternal.h"

#include "settings_parts.h"
extern "C" bool screenpics_scaling(ImDrawList *dl, ImVec2 a, ImVec2 b, int mode);   /* ui/screenpics.cpp */
extern "C" int opening_mode(void); extern "C" void opening_set_mode(int);           /* mods/opening.c */

extern "C" { int mpu_out_count(void); const char *mpu_out_name(int i); int mt32_families(void); int mt32_switch_pending(void); int mt32_switch(void); const char *mt32_status(void); void mt32_rescan(void); int mt32_dir_count(void); const char *mt32_dir(int); extern int sound_music_volume; int mt32_available(void); void ui_show_mt32_help(void); }
extern "C" void settings_helpers_jump(const char *name);
extern "C" void settings_display_jump(int k) { static const char *N[4] = { "屏幕", "开场", "音效", "控制器" }; if (k >= 0 && k < 4) settings_helpers_jump(N[k]); }   /* (test: udsect) */
/* a game controller: always on (but in the original game's menus it is a mouse too); which one is
   connected, its buttons drawn as any of the three kinds, and the jobs of the main ones to change */
extern "C" int plat_controller_kind(const char **name);
extern "C" int plat_pad_bind(int k);
extern "C" void plat_pad_bind_set(int k, int a);
int settings_pack_preview;                 /* (a pack previewed: ui/ui.cpp lets the music through) */
void controller_prefs(void)
{
    const char *nm; int kind = plat_controller_kind(&nm);
    static const char *KIND[4] = { "", "Xbox", "PlayStation", "任天堂" };
    if (kind) ImGui::Text("连接：%s（%s 风格）", nm, KIND[kind]);
    else ImGui::TextWrapped("当前未连接手柄：随时可插入。下方的按钮可提前设置。");
    static int view = 0;
    if (!view) view = kind ? kind : 1;
    ImGui::TextUnformatted("按钮显示方式："); 
    for (int k = 1; k <= 3; k++) { ImGui::SameLine(); if (ImGui::RadioButton(KIND[k], view == k)) view = k; }
    static const char *JOB[9] = { "单击", "右键单击（菜单）", "进入", "Esc", "Tab（关卡地图）", "空格（暂停）", "F12（Darklands+窗口）", "M（世界地图）", "F11（按键与点击）" };
    int keep = ui_pad_style_force; ui_pad_style_force = view;
    float gh = ImGui::GetFrameHeight();
    if (ImGui::BeginTable("##padmap", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("b", ImGuiTableColumnFlags_WidthFixed, UIS(70));
        ImGui::TableSetupColumn("j", ImGuiTableColumnFlags_WidthFixed, UIS(440));
        auto glyph = [&](int k, bool phys) {
            ImVec2 p = ImGui::GetCursorScreenPos();
            float w = ui_pad_glyph_width(gh * 0.9f, k);
            ImGui::Dummy(ImVec2(UIS(60), gh));
            if (phys) ui_pad_button_glyph(ImGui::GetWindowDrawList(), ImVec2(p.x + w * 0.5f + 4, p.y + gh * 0.5f), gh * 0.9f, k);
            else ui_pad_glyph(ImGui::GetWindowDrawList(), ImVec2(p.x + w * 0.5f + 4, p.y + gh * 0.5f), gh * 0.9f, k);
        };
        for (int k = PB_A; k <= PB_LS; k++) {
            ImGui::TableNextRow(); ImGui::TableNextColumn(); glyph(k, true);
            ImGui::TableNextColumn();
            int a = plat_pad_bind(k);
            ImGui::PushID(k); ImGui::SetNextItemWidth(UIS(290));
            if (ImGui::Combo("##job", &a, JOB, 9)) { plat_pad_bind_set(k, a); ui_dirty_config = 1; }
            ImGui::PopID();
        }
        struct F { int b; const char *what; } fixed[4] = { { PB_LSTICK, "光标（左扳机按住时：缓慢）" }, { PB_RSTICK, "滚动列表；按下时：Z（快进）" },
                                                            { PB_DPAD, "方向键" }, { PB_RT, "按住时：Ctrl（在陆地视图中Ctrl+点击可快速移动到该处）" } };
        for (const F &f : fixed) { ImGui::TableNextRow(); ImGui::TableNextColumn(); glyph(f.b, true); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(f.what); }
        ImGui::EndTable();
    }
    ui_pad_style_force = keep;
    if (ImGui::Button("返回常用按钮")) { for (int k = PB_A; k <= PB_LS; k++) plat_pad_bind_set(k, k); ui_dirty_config = 1; }
    /* the button prompts (ui/padglyphs.h): the controller's buttons on screen while it is in use */
    static const char *PM[5] = { "匹配当前使用的设备", "始终使用按键与鼠标", "始终使用Xbox", "始终使用PlayStation", "始终使用任天堂" };
    int pm = config_get_int("input.prompts", 0); if (pm < 0 || pm > 4) pm = 0;
    ImGui::SetNextItemWidth(UIS(260));
    if (ImGui::Combo("提示中显示的按钮", &pm, PM, 5)) { config_set_int("input.prompts", pm); ui_dirty_config = 1; }
    ui_hint("使用手柄时，提示与屏幕上的按钮均显示其对应按键。其中“始终”选项适用于用Steam Deck的触控板游玩的情况，此时触控板被视为鼠标。");
}
/* the setup of the game on this computer: four sections of the Settings page (tab_helpers) */
void setup_screen(void)
{
    {   /* the game's picture scaled up: three ways, each shown on a piece of the screen now (ui/screenpics.cpp);
           (the picture is always 4:3, as on a 1992 monitor) */
        int cur = ui_settings.scaler ? 1 : ui_settings.smooth ? 2 : 0;
        static const char *L[3] = { "锐利的像素", "圆角效果", "柔和" };
        float cw = fminf((ImGui::GetContentRegionAvail().x - 2 * ImGui::GetStyle().ItemSpacing.x) / 3, UIS(240)), ph = cw * 0.625f, ch = ph + ImGui::GetTextLineHeightWithSpacing() * 1.3f;
        for (int j = 0; j < 3; j++) {
            if (j) ImGui::SameLine();
            ImGui::PushID(400 + j);
            ImVec2 p = ImGui::GetCursorScreenPos();
            if (ImGui::InvisibleButton("sc", ImVec2(cw, ch))) {
                ui_settings.scaler = j == 1 ? 2 : 0; ui_settings.smooth = j != 0; ui_settings.integer = 0;
                config_set_int("display.scaler", ui_settings.scaler); config_set_int("display.smooth", ui_settings.smooth); config_set_int("display.integer_scale", 0);
                ui_dirty_config = 1; cur = j;
            }
            bool hv = ImGui::IsItemHovered();
            ImDrawList *dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(p, ImVec2(p.x + cw, p.y + ch), j == cur ? IM_COL32(140, 61, 33, 255) : hv ? IM_COL32(200, 170, 120, 255) : IM_COL32(60, 44, 30, 255), 6);
            float m = cw * 0.04f;
            if (!screenpics_scaling(dl, ImVec2(p.x + m, p.y + m), ImVec2(p.x + cw - m, p.y + ph - m * 0.4f), j))
                dl->AddText(ImVec2(p.x + 10, p.y + 10), IM_COL32(246, 236, 214, 255), "（暂无画面）");
            ImVec2 ts = ImGui::CalcTextSize(L[j]);
            dl->AddText(ImVec2(p.x + (cw - ts.x) * 0.5f, p.y + ch - ImGui::GetTextLineHeightWithSpacing() * 1.1f), IM_COL32(246, 236, 214, 255), L[j]);
            ImGui::PopID();
        }
        ui_hint(cur == 0 ? "每个像素都是清晰的正方形，正如游戏当初绘制的模样。"
              : cur == 1 ? "对角线边缘的阶梯状效果被圆滑处理，不会出现模糊。但在没有合适显卡的电脑上，启用此选项会使游戏运行变慢。"
                         : "有些许模糊，如同老式显示器一般。");
    }
    { extern int ui_margins_on; bool mg = ui_margins_on != 0, click = false;   /* (a long line: the words wrapped beside the box, not one line wider than the page) */
      ImGui::PushID("margins");
      if (ImGui::Checkbox("##mg", &mg)) click = true;
      ImGui::SameLine();
      if (ui_title_desc("侧边栏", "在宽屏上，日期、地点、钱包、任务、城镇及各类效果会显示在屏幕两侧（战斗时则显示日志与所有角色的状态）。")) { mg = !mg; click = true; }
      if (click) { ui_margins_on = mg; config_set_int("display.margins", mg); ui_dirty_config = 1; }
      ImGui::PopID(); }
    {   /* grows with the picture; this scales it */
        static const int P[] = { 70, 80, 90, 100, 115, 130, 150, 175, 200 };
        int cur = config_get_int("display.text_size", 100), sel = 3;
        for (int i = 0; i < 9; i++) if (P[i] == cur) sel = i;
        char lab[16]; snprintf(lab, sizeof lab, "%d%%", P[sel]);
        ImGui::SetNextItemWidth(UIS(160));
        if (ImGui::BeginCombo("边距、工具提示及地名的文字大小", lab)) {
            for (int i = 0; i < 9; i++) { char t[16]; snprintf(t, sizeof t, "%d%%", P[i]); if (ImGui::Selectable(t, i == sel)) { config_set_int("display.text_size", P[i]); ui_dirty_config = 1; } }
            ImGui::EndCombo();
        }
    }
    { bool pd = config_get_int("display.pointer_drawn", 1) != 0;
      if (ImGui::Checkbox("游戏重新绘制的手套式光标，无论尺寸如何都保持清晰锐利。", &pd)) { config_set_int("display.pointer_drawn", pd); ui_dirty_config = 1; }
      if (ImGui::IsItemHovered()) ImGui::SetTooltip("拖动物体时显示拳头；袖口处显示暂停状态——暂停中的战斗、敌对双方剑刃交叉、以及“前往”时的沙漏。关闭时：显示游戏的小图标。"); }
    { bool sc = ui_sys_cursor != 0;
      if (ImGui::Checkbox("同时在游戏内显示系统鼠标指针", &sc)) { ui_sys_cursor = sc; config_set_int("display.system_cursor", sc); ui_dirty_config = 1; } }
    {   /* the menus' size (fonts are baked at start-up) */
        static const int P[] = { 0, 100, 125, 150, 175, 200, 250 };
        static const char *N[] = { "自动（系统缩放）", "100%", "125%", "150%", "175%", "200%", "250%" };
        int cur = config_get_int("display.ui_scale", 0), sel = 0;
        for (int i = 0; i < 7; i++) if (P[i] == cur) sel = i;
        ImGui::SetNextItemWidth(UIS(260));
        if (ImGui::Combo("这些菜单的尺寸（重启后）", &sel, N, 7)) { config_set_int("display.ui_scale", P[sel]); ui_dirty_config = 1; }
    }
    { bool pu = config_get_int("display.pause_unfocused", 1) != 0;
      if (ImGui::Checkbox("当游戏窗口处于后台时暂停游戏", &pu)) { config_set_int("display.pause_unfocused", pu); ui_dirty_config = 1; } }
    bool fs = (SDL_GetWindowFlags(ui_window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
    if (ImGui::Checkbox("全屏模式（Alt+Enter）", &fs)) SDL_SetWindowFullscreen(ui_window, fs ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}
void setup_intro(void)
{
    {   /* the opening at start (mods/opening.c): the first start plays it whole */
        static const char *N[] = { "无：直接进入开始界面", "仅显示最后一幕：Darklands的女士及其欢迎", "整个开场" };
        int m = opening_mode();
        for (int i = 0; i < 3; i++) if (ImGui::RadioButton(N[i], m == i)) { opening_set_mode(i); ui_dirty_config = 1; }
        ui_hint("每次启动游戏时显示（首次启动会完整播放）。按键盘或点击可跳过。");
    }
}
void setup_sound(void)
{
    /* two previews, at the top right of the section: what the sound chosen below sounds like now (the
       second tune of the overland view, or ten seconds of a real battle's sounds), through the device chosen, or the
       custom pack when it is ticked (mods/soundpreview.c, mods/musicpack.c, mods/sfx.c) */
    const char *cur = config_get("sound.device", "auto");
    bool can_hear = soundpreview_possible() != 0;
    static u32 pack_until; static bool pack_on;
    static u32 seq_t; static int seq_k = -1;
    static const int SEQ[] = { 1, 2, 0, 3, 5, 4 };   /* parry, hit, miss, knock-back, bow, death */
    if (pack_on && plat_ticks_ms() > pack_until) { musicpack_preview(-1); pack_on = false; }
    if (seq_k >= 0 && plat_ticks_ms() >= seq_t) { sfx_preview(SEQ[seq_k]); seq_t = plat_ticks_ms() + 650; if (++seq_k >= 6) seq_k = -1; }
    settings_pack_preview = pack_on || seq_k >= 0;
    {
        const char *dev = !strcmp(cur, "auto") || !strcmp(cur, "game") ? (mt32_available() ? "mt32" : "fm") : cur;
        bool off = !strcmp(cur, "off");
        bool pack_music = musicpack_on && musicpack_versions(20) > 0, pack_sfx = sfx_mode != 0 || sfx_mod_count() > 0;
        float bw = ImGui::CalcTextSize("预览：战斗音效").x + ImGui::GetStyle().FramePadding.x * 2;
        float bm = ImGui::CalcTextSize("预览：音乐").x + ImGui::GetStyle().FramePadding.x * 2;
        float x = ImGui::GetContentRegionMax().x - bw - bm - ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX(x);
        bool spk = !strcmp(dev, "speaker") && !pack_music;            /* (the game plays no music on the PC speaker: its driver only beeps the battle sounds) */
        ImGui::BeginDisabled(off || spk || (!can_hear && !pack_music));
        bool mon = pack_music ? pack_on : soundpreview_playing(dev, 0) != 0;
        if (ImGui::Button(mon ? "Stop##pm" : "预览：音乐", ImVec2(bm, 0))) {
            if (mon) { if (pack_on) { musicpack_preview(-1); pack_on = false; } else soundpreview_stop(); }
            else if (pack_music) { musicpack_preview(20); pack_on = true; pack_until = plat_ticks_ms() + 15000; }
            else soundpreview_start(dev, 0);
        }
        ImGui::EndDisabled();
        if (spk && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("游戏在PC扬声器上不播放音乐，仅播放战斗音效。");
        ImGui::SameLine();
        ImGui::BeginDisabled(off || (!can_hear && !pack_sfx));
        bool bon = pack_sfx ? seq_k >= 0 : soundpreview_playing(dev, 1) != 0;
        if (ImGui::Button(bon ? "Stop##pb" : "预览：战斗音效", ImVec2(bw, 0))) {
            if (bon) { if (seq_k >= 0) seq_k = -1; else soundpreview_stop(); }
            else if (pack_sfx) { seq_k = 0; seq_t = 0; }
            else soundpreview_start(dev, 1);
        }
        ImGui::EndDisabled();
        if (!can_hear && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("预览需要在非战斗状态下、游戏正在进行时进行。");
    }
    /* the devices, in three tiers: the best (the MT-32, as composed), an OK one (the Sound Blaster,
       as most heard it in 1992), and those for history's sake */
    auto device = [&](const char *id, const char *title, const char *what, bool chosen) {
        ImGui::PushID(id);
        if (ImGui::RadioButton(title, chosen)) { config_set("sound.device", id); ui_dirty_config = 1; }
        if (what) { ImGui::Indent(UIS(30)); ui_hint("%s", what); ImGui::Unindent(UIS(30)); }
        ImGui::PopID();
    };
    auto tier = [&](const char *t) { ImGui::Spacing(); ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 52, 20, 255)); ImGui::TextUnformatted(t); ImGui::PopStyleColor(); };
    tier("最佳");
    device("mt32", "罗兰 MT-32", "罗兰在1987年推出的合成器，正是《Darklands》所适配的型号。游戏中的音乐与战斗音效皆由它奏出：音乐按原作编排，而打击声则尽可能贴近钢铁之感。",
           !strcmp(cur, "mt32") || !strcmp(cur, "auto"));
    {
        ImGui::Indent(UIS(30));
        bool roms = mt32_available() != 0;
        static bool more;
        if (!roms) {      /* the one thing that pokes out: in red, a click for the rest */
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(170, 30, 20, 255));
            if (ImGui::Selectable(more ? "未找到MT-32 ROM文件（点击关闭）" : "未找到MT-32 ROM文件（点击查看如何使其正常运行）", more, 0, ImVec2(ImGui::CalcTextSize("未找到MT-32 ROM文件（点击查看如何使其正常运行）").x, 0))) more = !more;
            ImGui::PopStyleColor();
        }
        if (roms) { ImGui::SetNextItemOpen(more); more = ImGui::TreeNode("MT-32详细信息"); if (more) ImGui::TreePop(); }
        if (more) {
            ImGui::Indent(UIS(14));
            if (!roms) {
                ImGui::TextWrapped("MT-32是罗兰于1987年推出的一款合成器；《Darklands》的音乐与战斗音效均为此设备而设计（两者皆由其演奏），而此处的其他选项均为"
                                   "较低版本。《Darklands+》内置了该设备（由Munt模拟实现），只需从真实设备的内存中复制两个文件（即ROM文件）即可，这些文件归罗兰所有，"
                                   "因此无法随游戏一同发布。若您已使用过Munt或DOSBox并搭配MT-32，则相关文件早已存在。在找到这些文件之前，将改用Sound Blaster播放。");
                if (ImGui::Button("在哪里能找到它们……")) ui_show_mt32_help();
            } else ui_hint("%s", mt32_status());
            static char romdir[1024]; static bool init;
            if (!init) { snprintf(romdir, sizeof romdir, "%s", config_get("sound.mt32_roms", "")); init = true; }
            ImGui::SetNextItemWidth(UIS(420));
            if (ImGui::InputTextWithHint("##mt32roms", "ROM文件所在的文件夹", romdir, sizeof romdir)) { config_set("sound.mt32_roms", romdir); ui_dirty_config = 1; }
            ImGui::SameLine();
            if (ImGui::Button("Look again##mt32")) { mt32_rescan(); mt32_switch(); }
            ui_hint("《Darklands+》会根据文件内容自动查找，无论文件名为何：在该文件夹、游戏主文件夹及其“roms”子文件夹，以及Munt和DOSBox存放它们的位置。");
            {   bool lcd = config_get_int("sound.mt32_lcd", 1), click = false;
                ImGui::PushID("lcd");
                if (ImGui::Checkbox("##lcd", &lcd)) click = true;
                ImGui::SameLine();
                if (ui_title_desc("MT-32的显示界面", "游戏在屏幕右上角显示消息时，那块小小的显示屏。")) { lcd = !lcd; click = true; }
                if (click) { config_set_int("sound.mt32_lcd", lcd); ui_dirty_config = 1; }
                ImGui::PopID(); }
            ImGui::Unindent(UIS(14));
        }
        ImGui::Unindent(UIS(30));
    }
    tier("OK");
    device("fm", "声卡专业版", "这是1992年大多数玩家所使用的声卡：为更简单的合成器所编排的音乐，相比MT-32更为单薄且带有些许嗡鸣。", !strcmp(cur, "fm"));
    device("midi", "MIDI", "《Darklands+》以MIDI格式输出音乐与战斗音效：适用于连接至电脑的真实MT-32或CM-32L，亦或单独安装的Munt（MT-32的仿制品）。"
           "普通合成器，例如Windows自带的那款，所奏出的乐器并不正确。", !strcmp(cur, "midi"));
    if (!strcmp(cur, "midi")) {
        int n = mpu_out_count();
        const char *out = config_get("sound.midi_out", "");
        if (!n) ui_hint("此电脑无MIDI输出端口（无法连接合成器）。");
        else {
            ImGui::SetNextItemWidth(UIS(420));
            if (ImGui::BeginCombo("MIDI输出", out[0] ? out : "系统默认设置")) {
                if (ImGui::Selectable("系统默认设置", !out[0])) { config_set("sound.midi_out", ""); ui_dirty_config = 1; }
                for (int i = 0; i < n; i++) { const char *nm = mpu_out_name(i); if (nm[0] && ImGui::Selectable(nm, !strcmp(nm, out))) { config_set("sound.midi_out", nm); ui_dirty_config = 1; } }
                ImGui::EndCombo();
            }
        }
    }
    tier("为历史记录起见");
    device("adlib", "AdLib（较早一代的声卡）", "音乐大致相同；战斗音效也只是粗糙的模仿。", !strcmp(cur, "adlib"));
    device("speaker", "PC扬声器", "用于战斗音效的蜂鸣声；游戏中不播放任何音乐。", !strcmp(cur, "speaker"));
    device("game", "按照原版游戏INSTALL程序中的设定", 0, !strcmp(cur, "game"));
    device("off", "关闭", 0, !strcmp(cur, "off"));
    ui_hint("下一场战斗或下一次启动时，便会听到变化。");
    {
        int v = sound_music_volume;
        ImGui::SetNextItemWidth(UIS(260));
        if (ImGui::SliderInt("音乐音量", &v, 0, 150, "%d%%")) { sound_music_volume = v; config_set_int("sound.music_volume", v); ui_dirty_config = 1; }
    }

    /* the soundtrack: every piece of the game's music, through the device chosen above (mods/playlist.c) */
    {
        ImGui::Spacing();
        const char *pdev = !strcmp(cur, "auto") || !strcmp(cur, "game") ? (mt32_available() ? "mt32" : "fm") : cur;
        bool can = soundpreview_possible() && strcmp(cur, "off") != 0;
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(120, 52, 20, 255)); ImGui::TextUnformatted("原声配乐"); ImGui::PopStyleColor();
        ui_hint(can ? "游戏中的每一段音乐，均由上方选定的设备播放。点击某段音乐即可试听。"
                    : "游戏中的每一段音乐，均由上方选定的设备播放：需在游戏进行中、非战斗状态下方可生效。");
        auto mmss = [](int ms) { char t[16]; int s2 = ms / 1000; snprintf(t, sizeof t, "%d:%02d", s2 / 60, s2 % 60); return std::string(t); };
        int now = playlist_now(), len = now >= 0 ? playlist_length_ms(now) : -1;
        bool paused = playlist_paused() != 0, meas = playlist_measuring() != 0;
        ImGui::BeginDisabled(!can || now < 0 || meas);
        if (ImGui::Button(now >= 0 && !paused ? "Pause##st" : "Play##st", ImVec2(UIS(64), 0))) playlist_pause(!paused);
        ImGui::SameLine();
        if (ImGui::Button("Stop##st", ImVec2(UIS(56), 0))) playlist_stop();
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (now >= 0) {
            const char *d = musicpack_desc(now);
            ImGui::Text("%d  %s", now, d[0] ? d : "");
            static int drag = -1;
            int pos = drag >= 0 ? drag : playlist_pos_ms();
            ImGui::TextUnformatted(meas ? "-:--" : mmss(pos).c_str());
            ImGui::SameLine();
            ImGui::SetNextItemWidth(UIS(300));
            int v = pos;
            ImGui::BeginDisabled(meas || len <= 0);
            if (ImGui::SliderInt("##seek", &v, 0, len > 0 ? len : 1, "")) drag = v;
            if (ImGui::IsItemDeactivatedAfterEdit() && drag >= 0) { playlist_seek(drag); drag = -1; }
            else if (!ImGui::IsItemActive()) drag = -1;
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (meas) ui_hint("查找其长度（静默，几秒钟）");
            else ImGui::TextUnformatted(len > 0 ? (mmss(len) + (musicpack_loops(now) ? "然后再次" : "")).c_str() : len == -2 ? "repeats" : "");
        } else ImGui::TextDisabled("没有音乐正在播放");
        ImGui::BeginDisabled(!can);
        if (ImGui::BeginChild("##pieces", ImVec2(0, UIS(210)), ImGuiChildFlags_Borders)) {
            for (int k = 8; k <= 47; k++) {
                const char *d = musicpack_desc(k);
                ImGui::PushID(k);
                float x0 = ImGui::GetCursorPosX();
                if (ImGui::Selectable("##piece", now == k)) playlist_play(pdev, k);
                ImGui::SameLine(x0 + UIS(4)); ImGui::TextDisabled("%d", k);   /* (the numbers in a column of their own) */
                ImGui::SameLine(x0 + UIS(40)); ImGui::TextUnformatted(d[0] ? d : "（某段音乐尚未确定位置）");
                ImGui::PopID();
                int l = playlist_length_dev_ms(pdev, k);
                if (l > 0 || l == -2) {
                    std::string t = l > 0 ? mmss(l) : std::string("repeats");
                    ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(t.c_str()).x - UIS(6));
                    ImGui::TextDisabled("%s", t.c_str());
                }
            }
        }
        ImGui::EndChild();
        ImGui::EndDisabled();
    }

    /* custom packs: recordings in place of the synthesizer's music, or of the battle sounds (folded away) */
    ImGui::Spacing();
    extern char dlp_user_dir[];
    if (ImGui::TreeNode("自定义音效包")) {
        ui_hint("替代合成器音乐或战斗音效的录音。战斗音效包作为模组随《Darklands+》附赠（F12 > Mods）；而音乐包则没有。上述预览在勾选后即可播放。");
        {   /* music packs (mods/musicpack.c) */
            bool mp = musicpack_on;
            if (ImGui::Checkbox("来自音乐包的音乐（存于“music”文件夹中的录音）", &mp)) { musicpack_set(mp); ui_dirty_config = 1; }
            int n = musicpack_count();
            if (n) {
                const char *f0 = ""; for (int k = 0; k < 60 && !f0[0]; k++) f0 = musicpack_from(k);
                ui_hint("包内%d首曲目%s（%s）；其余部分仍由合成器播放。", n, n == 1 ? "" : "s", strcmp(f0, "music") ? f0 : "音乐文件夹");
            }
            else ui_hint("未找到音乐包：某个模组的音乐文件夹，或 %s%smusic，其中每首曲目的录音都以其编号命名（10.ogg、“11 market.ogg”）。", dlp_user_dir, PATH_SEP);
            {   int fp = 0, now = musicpack_now(&fp);              /* (the piece playing: to make a pack by listening) */
                if (now > 0) { const char *d = musicpack_desc(now); ui_hint("游戏当前播放的音乐：第 %d%s%s%s首，%s（设置界面打开时静音）。", now, d[0] ? " (" : "", d, d[0] ? ")" : "", fp ? "来自该包" : "由合成器播放"); } }
            if (ImGui::TreeNode("这些曲目")) {
                ui_hint("当某首曲目的文件名以数字开头时（一个数字最多可有四首，依次播放：“27 a.ogg”、“27 b.ogg”……），其录音将取代原曲目。"
                        "游戏中循环播放的曲目在包中也会循环；当包中的某首曲目正在播放时，合成器会静音。");
                for (int k = 8; k <= 47; k++) {
                    const char *f = musicpack_file(k), *d = musicpack_desc(k);
                    const char *bn = f + strlen(f); while (bn > f && bn[-1] != '/' && bn[-1] != '\\') bn--;
                    ui_hint("%2d  %s%s%s", k, d, d[0] ? ", " : "", musicpack_loops(k) ? "loops" : "播放一次");
                    if (f[0]) { ImGui::SameLine(); int nv = musicpack_versions(k); if (nv > 1) ImGui::Text("%s 和另外 %d 首，依次播放", bn, nv - 1); else ImGui::Text("  %s", bn); }
                }
                if (ImGui::SmallButton("Look again##music")) musicpack_reload();
                ImGui::TreePop();
            }
        }
        {   /* battle sounds packs (mods/sfx.c) */
            ImGui::Spacing();
            bool sp = sfx_mode;   /* (hits: the fix "Ordinary hits make a sound") */
            int nm = sfx_mod_count(), n = sfx_user_count() - nm;
            if (ImGui::Checkbox("来自音效包（“sounds”文件夹）的战斗音效", &sp)) { sfx_set(sp); ui_dirty_config = 1; }
            if (nm) ui_hint("在 F12 > 模组中启用的模组会提供 %d 条战斗音效%s：它们将替代合成器的声音（可在该处关闭此模组以使用游戏自带的音效）。", nm, nm == 1 ? "" : "s");
            if (n > 0) ui_hint("%d 声音%s 在 %s%s声音%s 中；其余部分则采用游戏自带音效。", n, n == 1 ? "" : "s", dlp_user_dir, PATH_SEP, sp ? "" : "（勾选后仅播放一次）");
            else if (!nm) ui_hint("替代合成器录制的打击、招架与呐喊声：可在 %s%ssounds 中的包，或通过“Battle sounds”模组（F12 > Mods）实现。", dlp_user_dir, PATH_SEP);
            if (sp || nm) {
                ImGui::Indent(UIS(30));
                int v = sfx_volume;
                ImGui::SetNextItemWidth(UIS(260));
                if (ImGui::SliderInt("包的音量", &v, 0, 150, "%d%%")) { sfx_set_volume(v); ui_dirty_config = 1; }
                static char msg[400];
                if (ImGui::Button("Look again##sfx")) { sfx_reload(); snprintf(msg, sizeof msg, "找到%d个声音%s。", sfx_user_count(), sfx_user_count() == 1 ? "" : "s"); }
                if (msg[0]) ui_hint("%s", msg);
                ImGui::Unindent(UIS(30));
            }
            if (ImGui::TreeNode("音效包的文件名")) {
                for (int k = 0; k < SFX_COUNT; k++) ui_hint("%s.wav 或 .ogg：%s", SFX_NAMES[k], SFX_DESC[k]);
                ui_hint("同一音效的多个版本（parry1.wav、parry2.wav 等）会随机轮流播放。");
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
}


