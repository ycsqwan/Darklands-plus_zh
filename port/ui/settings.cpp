// settings.cpp - the Darklands+ window's Settings page (F12), and its Mods and Reconstructed Content
// tabs. Each switch is a config key (runtime/config.h); a change marks the settings to be written
// when the window closes (ui_dirty_config). The sections on what things look like are in
// ui/settings_look.cpp, those on the computer's setup in ui/settings_setup.cpp (ui/settings_parts.h).
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

void ui_hint(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::TextWrappedV(fmt, ap);
    ImGui::PopStyleColor();
    va_end(ap);
}

/* ------------------------------------------------------------------ tabs */
extern "C" int encounters_count(void);
extern "C" int opening_mode(void);                 /* mods/opening.c */
extern "C" void opening_set_mode(int);
extern "C" int encounters_errors(void);
static void replacement_art(void)
{
    bool ha = hdart_on;
    if (ImGui::Checkbox("使用“art”文件夹中的替换素材", &ha)) { hdart_set(ha); ui_dirty_config = 1; }
    {
        int n = hdart_count();
        extern char dlp_user_dir[];
        if (n) ui_hint("%d 张替换图片%s 在 %s%sart 中。", n, n == 1 ? "" : "s", dlp_user_dir, PATH_SEP);
        else ui_hint("尚无替换图片（%s%sart）。", dlp_user_dir, PATH_SEP);
        static char msg[600];
        if (ImGui::Button("再次查看")) { hdart_rescan(); snprintf(msg, sizeof msg, "找到 %d 张替换图片。", hdart_count()); }
        ImGui::SameLine();
        if (ImGui::Button("导出原始图片")) hdart_export(msg, sizeof msg);
        if (msg[0]) ui_hint("%s", msg);
        bool sn = hdart_show_names;
        if (ImGui::Checkbox("鼠标悬停时显示图片名称", &sn)) { hdart_show_names = sn; config_set_int("display.art_names", sn); ui_dirty_config = 1; }
        ui_hint("供美术师使用：一个名为 art%sNAME.png 的文件（任意尺寸；在图片颜色为0处保留透明度）将替换"
             "游戏中任何位置显示的 NAME 图片，按窗口分辨率进行替换。导出时会将每张原始图片"
             "以自身色彩保存为模板，存入 art%soriginal 文件夹。无论游戏在图片上绘制了什么内容（文字、"
             "鼠标指针、其他图片），这些内容都将保持在最上层。", PATH_SEP, PATH_SEP);
        ui_hint("更多徽章：art%sSHIELDP.png、SHIELDQ.png……直至 SHIELDZ.png，可在游戏原有的十五个徽章之后再添加"
             "（纹章学，组建队伍时）。使用其中某个徽章的成员所保存的游戏存档，仅能在 Darklands+ 中加载。", PATH_SEP);
    }

}
static void option_row(const Option *o, int depth);
int ui_original_mode(void);
void tab_mods(void)
{
    ImGui::PushFont(ui_font_head); orn_septext("模组文件夹"); ImGui::PopFont();
    {
        static char msg[1400];
        char md[1100]; datamods_folder(md, sizeof md);
        extern char dlp_user_dir[];
        ImGui::TextWrapped("模组是 %s 中的一个文件夹" PATH_SEP "模组的文件布局与游戏文件夹相同："
                           "当勾选该模组时，游戏会用模组文件替换原版文件（游戏文件夹本身不会被修改）。", dlp_user_dir);
        if (!datamods_n) ui_hint("尚未找到任何模组。");
        else if (datamods_n > 1) ui_hint("当两个模组同时修改同一文件时，列表中位置靠下的模组优先生效。可通过拖动模组手柄（或使用箭头）来调整顺序。");
        int move_from = -1, move_to = -1;
        for (int i = 0; i < datamods_n; i++) {
            ImGui::PushID(1000 + i);
            ImGui::BeginGroup();
            if (datamods_n > 1) {                                /* the handle: dragged to another mod's place */
                float h = ImGui::GetFrameHeight();
                ImVec2 a = ImGui::GetCursorScreenPos();
                ImGui::InvisibleButton("##grip", ImVec2(h * 0.7f, h));
                bool hv = ImGui::IsItemHovered() || ImGui::IsItemActive();
                if (hv) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
                ImDrawList *dl = ImGui::GetWindowDrawList();
                for (int k = 0; k < 3; k++) {
                    float y = a.y + h * (0.32f + 0.18f * k);
                    dl->AddLine(ImVec2(a.x + h * 0.12f, y), ImVec2(a.x + h * 0.58f, y), ImGui::GetColorU32(hv ? ImGuiCol_Text : ImGuiCol_TextDisabled), fmaxf(1.5f, h * 0.07f));
                }
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip)) {
                    ImGui::SetDragDropPayload("DL_MOD", &i, sizeof i);
                    ImGui::EndDragDropSource();
                }
                ImGui::SameLine();
                ImGui::BeginDisabled(i == 0);
                if (ImGui::ArrowButton("##up", ImGuiDir_Up)) { move_from = i; move_to = i - 1; }
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::BeginDisabled(i == datamods_n - 1);
                if (ImGui::ArrowButton("##down", ImGuiDir_Down)) { move_from = i; move_to = i + 1; }
                ImGui::EndDisabled();
                ImGui::SameLine();
            }
            bool on = datamods[i].on != 0;
            if (ImGui::Checkbox(datamods[i].title, &on)) { datamods_set(i, on); ui_dirty_config = 1; }
            ImGui::Indent(datamods_n > 1 ? ImGui::GetFrameHeight() * 2.7f + ImGui::GetStyle().ItemSpacing.x * 3 + UIS(34) : UIS(34));
            if (datamods[i].desc[0]) ui_hint("%s", datamods[i].desc);
            { std::string fp = datamods[i].path; for (char &c : fp) if (c == '/' || c == '\\') c = PATH_SEP[0]; ui_hint("文件夹：%s", fp.c_str()); }
            ImGui::Unindent(datamods_n > 1 ? ImGui::GetFrameHeight() * 2.7f + ImGui::GetStyle().ItemSpacing.x * 3 + UIS(34) : UIS(34));
            ImGui::EndGroup();
            if (ImGui::BeginDragDropTarget()) {                  /* (dropped on a mod: takes its place) */
                ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
                if (const ImGuiPayload *pl = ImGui::AcceptDragDropPayload("DL_MOD", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) { move_from = *(const int *)pl->Data; move_to = i; }
                else if (const ImGuiPayload *pk = ImGui::GetDragDropPayload()) {
                    int from = *(const int *)pk->Data;              /* a gilt line where it will go */
                    float y = from < i ? b.y + 2 : a.y - 2;
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(a.x, y), ImVec2(b.x, y), IM_COL32(220, 180, 90, 255), 3.0f);
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::PopID();
        }
        if (move_from >= 0) { datamods_move(move_from, move_to); ui_dirty_config = 1; }
        {   /* mods with ENCOUNTERS folders (mods/encounters.c) */
            int ne = encounters_count(), nerr = encounters_errors();
            if (ne || nerr) {
                ImGui::Spacing();
                ImGui::TextWrapped("来自模组的新遭遇：%d%s", ne, nerr ? "" : ".");
                if (nerr) { ImGui::SameLine(); ImGui::TextColored(ImVec4(1, 0.6f, 0.4f, 1), "（%d 个含错误的文件%s已被排除：请参阅 darklands_port.log）", nerr, nerr == 1 ? "" : "s"); }
                ImGui::Indent(UIS(34));
                int share = config_get_int("encounters.share", 100);
                ImGui::SetNextItemWidth(UIS(220));
                if (ImGui::SliderInt("How often new encounters come up##encshare", &share, 0, 300, "%d%%")) { config_set_int("encounters.share", share); ui_dirty_config = 1; }
                ui_hint("在 100%% 的情况下，每次新遭遇出现的概率与该地形原有遭遇的概率相同（若其文件赋予了权重，则概率更高）。");
                bool seal = config_get_int("encounters.seal", 1) != 0;
                if (ImGui::Checkbox("在卡片角落用印章标记新遭遇", &seal)) { config_set_int("encounters.seal", seal); ui_dirty_config = 1; }
                ImGui::Unindent(UIS(34));
            }
        }
        if (ImGui::Button("再次查找模组")) { datamods_scan(); hdart_rescan(); msg[0] = 0; }
        ImGui::SameLine();
        static char newname[64] = "我的模组";
        ImGui::SetNextItemWidth(UIS(200)); ImGui::InputText("##newmod", newname, sizeof newname);
        ImGui::SameLine();
        if (ImGui::Button("创建一个新的模组文件夹") && newname[0]) {
            char w[1300]; int r = datamods_new(newname, w, sizeof w);
            snprintf(msg, sizeof msg, r > 0 ? "已创建 %s（附带一份 mod.txt，说明各部分用途）。" : r == 0 ? "%s 已存在。" : "无法创建 %s。", w);
        }
        if (ImGui::Button("导出卡片文本")) {
            char w[1300]; int r = datamods_export_cards(w, sizeof w);
            if (r >= 0) snprintf(msg, sizeof msg, "%d 张卡片文件已写入 %s，以文本形式保存。请将您修改过的文件复制到您的模组 MSGFILES 文件夹中。", r, w);
            else snprintf(msg, sizeof msg, "无法读取 MSGFILES。");
        }
        ImGui::SameLine(); ui_hint("每个城镇和遭遇的文本均可编辑，并带有相应选项");
        { extern int ui_collect_texts; bool ct = ui_collect_texts != 0, click = false;   /* (the words wrapped beside the box) */
          ImGui::PushID("collect");
          if (ImGui::Checkbox("##ct", &ct)) click = true;
          ImGui::SameLine();
          if (ui_title_desc("将显示的每段文本写入文件", "用于翻译：模组" PATH_SEP "_export" PATH_SEP "texts_seen.txt.")) { ct = !ct; click = true; }
          if (click) ui_collect_texts = ct;
          ImGui::PopID(); }
        ui_hint("模组的 text.txt 文件会替换游戏菜单中内置的文本，每行一条，例如：购买物品 => Kaufen（需要新文本：F12 > 设置 > 文本）。");
        if (msg[0]) ImGui::TextWrapped("%s", msg);
        ui_hint("模组按上述列表顺序应用；若两个模组修改同一文件，则位置靠下的优先。"
             "MSGFILES 和 .CAT 目录中的单个文件，可用同名文件夹替换，仅保留这些文件。"
             "卡牌文本和图片会同时更新；物品列表、圣人、城市等则在下次启动游戏时更新。");
    }
    ImGui::Spacing();
    ImGui::PushFont(ui_font_head); orn_septext("替换美术素材（供美术师使用）"); ImGui::PopFont();
    replacement_art();
    /* the built-in mods: easier play, no preset's (each its own switch, as a mod folder is) */
    ImGui::Spacing();
    ImGui::PushFont(ui_font_head); orn_septext("内置模组：让游戏更易上手"); ImGui::PopFont();
    if (ui_original_mode()) { ui_hint("原版游戏运行时关闭（全原版）：可选择其他预设来启用这些功能。"); return; }
    ui_hint("更注重剧情而非战斗。每个选项都是独立开关；无预设会全部开启，而“全原版”和“纯粹派”则会全部关闭。"
            "部分效果仅在下一次新游戏开始时生效（其说明如此注明）。");
    int n = options_count();
    for (int i = 0; i < n; i++) { const Option *o = options_get(i); if (o->cls == OC_EASY) option_row(o, 0); }
}

/* the presets (mods/options.h): "the original game" is Full Original; the player's own choices are
   kept when a preset replaces them (Custom) */
int ui_original_mode(void) { return config_get_int("original", 0); }
bool ui_modding(void) { return !ui_original_mode(); }      /* (the Editor: always but in the original game; editor.show retired) */
void ui_set_original(bool on)
{
    if (on) preset_apply(PRESET_BUGGY_PURIST);
    else if (config_get_int("custom.saved", 0)) preset_restore_custom();
    else preset_apply(PRESET_PLUS);
    ui_dirty_config = 1;
}
extern "C" void ui_set_original_c(int on)          /* (the test command uoriginal: 0 off, 1 on; 10 + n preset n) */
{
    if (on >= 10) preset_apply(on - 10); else ui_set_original(on != 0);
    fprintf(stderr, "original %d preset %d: fixes", ui_original_mode(), preset_current());
    for (int i = 0; i < FIX_COUNT; i++) fprintf(stderr, " %d", FIXES[i].on);
    fprintf(stderr, " mods");
    for (int i = 0; i < MOD_COUNT; i++) fprintf(stderr, " %d", MODS[i].enabled);
    fprintf(stderr, "\n");
}

/* a switch's words as one paragraph: the short title in bold, a colon, and what it does in grey,
   wrapped together at the page's width; true when the words are clicked (they switch it too) */
bool ui_title_desc(const char *title, const char *desc, bool dim, const char *id)
{
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImFont *fr = ImGui::GetFont(), *fb = ui_font_bold ? ui_font_bold : fr;
    float fs = ImGui::GetFontSize(), w = ImGui::GetContentRegionAvail().x, ls = ImGui::GetTextLineHeight() + UIS(1);   /* (as TextWrapped's lines) */
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImU32 ink = ImGui::GetColorU32(dim ? ImGuiCol_TextDisabled : ImGuiCol_Text);
    ImU32 grey = ImGui::GetColorU32(ImGuiCol_TextDisabled, dim ? 0.75f : 1.0f);
    float x = 0, y = 0;
    auto words = [&](const char *s, ImFont *f, ImU32 col) {
        float sp = f->CalcTextSizeA(fs, FLT_MAX, 0, " ").x;
        while (*s) {
            while (*s == ' ') s++;
            const char *e = s; while (*e && *e != ' ') e++;
            if (e == s) break;
            float wd = f->CalcTextSizeA(fs, FLT_MAX, 0, s, e).x;
            if (x > 0 && x + wd > w) { x = 0; y += ls; }
            dl->AddText(f, fs, ImVec2(p0.x + x, p0.y + y), col, s, e);
            x += wd + sp; s = e;
        }
    };
    std::string t = std::string(title) + (desc && desc[0] ? ":" : "");
    words(t.c_str(), fb, ink);
    if (desc) words(desc, fr, grey);
    ImGui::SetCursorScreenPos(p0);
    bool clicked = ImGui::InvisibleButton(id, ImVec2(w > 1 ? w : 1, y + ImGui::GetTextLineHeight()));
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    return clicked;
}

int ui_option_pics(const char *key, const char **caps);                           /* ui/optpreview.cpp */
void ui_option_pic(const char *key, int i, ImDrawList *dl, ImVec2 a, ImVec2 b);
static const char *pcaps[4];
extern "C" bool screenpics_draw(ImDrawList *dl, ImVec2 a, ImVec2 b);   /* ui/screenpics.cpp */
extern "C" bool screenpics_scaling(ImDrawList *dl, ImVec2 a, ImVec2 b, int mode);
extern "C" bool screenpics_map_draw(ImDrawList *dl, ImVec2 a, ImVec2 b);
extern "C" bool screenpics_land_draw(ImDrawList *dl, ImVec2 a, ImVec2 b);
extern "C" void saves_prefs(void);   /* ui/saves.cpp */
static void opt_set(const Option *o, int v) { option_set(o, v); ui_dirty_config = 1; }
static void option_row(const Option *o, int depth);
static void option_children(const Option *o, int depth)
{
    int n = options_count();
    for (int i = 0; i < n; i++) { const Option *c = options_get(i); if (c->parent && !strcmp(c->parent, o->key)) option_row(c, depth); }
}
/* a switch: the box, its words; a choice: its words, then the choices in a row (or a list); then
   what it means for saved games, then the switches that belong to it while it is on */
static void option_row(const Option *o, int depth)
{
    ImGui::PushID(o->key);
    float ind = depth * (ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x);
    if (ind > 0) ImGui::Indent(ind);
    int v = option_value(o);
    if (!o->choices) {
        bool on = v != 0;
        if (ImGui::Checkbox("##o", &on)) opt_set(o, on);
        ImGui::SameLine();
        if (ui_title_desc(o->title, o->desc)) opt_set(o, option_value(o) ? 0 : 1);
    } else {
        ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight())); ImGui::SameLine();
        ui_title_desc(o->title, o->desc);
        ImGui::Indent(ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x);
        int nc = option_nchoices(o), sel = -1;
        for (int k = 0; k < nc; k++) if (o->vals[k] == v) sel = k;
        char lab[96];
        if (o->flags & OF_COMBO) {
            float wmax = 0; for (int k = 0; k < nc; k++) wmax = fmaxf(wmax, ImGui::CalcTextSize(option_choice(o, k, lab, sizeof lab)).x);
            ImGui::SetNextItemWidth(wmax + ImGui::GetFrameHeight() + UIS(24));
            if (ImGui::BeginCombo("##c", sel >= 0 ? option_choice(o, sel, lab, sizeof lab) : "")) {
                for (int k = 0; k < nc; k++) if (ImGui::Selectable(option_choice(o, k, lab, sizeof lab), k == sel)) opt_set(o, o->vals[k]);
                ImGui::EndCombo();
            }
        } else {
            float right = ImGui::GetContentRegionMax().x;
            for (int k = 0; k < nc; k++) {
                option_choice(o, k, lab, sizeof lab);
                float bw = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize(lab).x;
                if (k) { ImGui::SameLine(); if (ImGui::GetCursorPosX() + bw > right) ImGui::NewLine(); }
                ImGui::PushID(k);
                if (ImGui::RadioButton(lab, k == sel)) opt_set(o, o->vals[k]);
                ImGui::PopID();
            }
        }
        ImGui::Unindent(ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x);
    }
    float ti = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x;
    if (!strcmp(o->key, "display.native_screens")) {     /* pictures of the new screens, from this game (ui/screenpics.cpp) */
        ImGui::Indent(ti);
        float pw = fminf(UIS(480), ImGui::GetContentRegionAvail().x), ph = pw * 9 / 16;
        ImVec2 a = ImGui::GetCursorScreenPos();
        if (screenpics_draw(ImGui::GetWindowDrawList(), a, ImVec2(a.x + pw, a.y + ph))) ImGui::Dummy(ImVec2(pw, ph + UIS(4)));
        else ui_hint("新界面的图片会在你游戏中遇到时在此处显示：卡牌、商店、角色面板、旅店、炼金术等。");
        ImGui::Unindent(ti);
    } else if (!strcmp(o->key, "map.key_m")) {          /* the world map as the player last saw it (ui/screenpics.cpp) */
        ImGui::Indent(ti);
        float pw = fminf(UIS(420), ImGui::GetContentRegionAvail().x), ph = pw * 9 / 16;
        ImVec2 a = ImGui::GetCursorScreenPos();
        if (screenpics_map_draw(ImGui::GetWindowDrawList(), a, ImVec2(a.x + pw, a.y + ph))) ImGui::Dummy(ImVec2(pw, ph + UIS(4)));
        else ui_hint("世界地图的图片会在你游戏中首次打开（M）后在此处出现。");
        ImGui::Unindent(ti);
    } else if (!strcmp(o->key, "map.overland_marks")) {          /* the overland view as the player last saw it, names and quest marks on it */
        ImGui::Indent(ti);
        float pw = fminf(UIS(420), ImGui::GetContentRegionAvail().x), ph = pw * 9 / 16;
        ImVec2 a = ImGui::GetCursorScreenPos();
        if (screenpics_land_draw(ImGui::GetWindowDrawList(), a, ImVec2(a.x + pw, a.y + ph))) ImGui::Dummy(ImVec2(pw, ph + UIS(4)));
        else ui_hint("陆地视图的图片，带有名称和任务标记，会在队伍外出探索后在此处显示。");
        ImGui::Unindent(ti);
    } else if (int np = ui_option_pics(o->key, pcaps)) {   /* what it turns on, in pictures */
        ImGui::Indent(ti);
        float gap = UIS(10), pw = fminf(UIS(np == 1 ? 260 : 170), (ImGui::GetContentRegionAvail().x - gap * (np - 1)) / np), ph = pw * 0.6f;
        for (int k = 0; k < np; k++) {
            if (k) ImGui::SameLine(0, gap);
            ImVec2 a = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2(pw, ph + ImGui::GetTextLineHeightWithSpacing()));
            ImDrawList *dl = ImGui::GetWindowDrawList();
            ui_option_pic(o->key, k, dl, a, ImVec2(a.x + pw, a.y + ph));
            dl->AddRect(ImVec2(a.x - 1, a.y - 1), ImVec2(a.x + pw + 1, a.y + ph + 1), IM_COL32(120, 100, 80, 160), 3);
            ImVec2 ts = ImGui::CalcTextSize(pcaps[k]);
            float sc = ts.x > pw ? pw / ts.x : 1.0f;
            dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * sc, ImVec2(a.x + (pw - ts.x * sc) * 0.5f, a.y + ph + 2), ImGui::GetColorU32(ImGuiCol_Text), pcaps[k]);
        }
        ImGui::Unindent(ti);
    }
    if (o->note) {
        ImGui::Indent(ti);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.32f, 0.08f, 1));
        ImGui::TextWrapped("%s", o->note);
        ImGui::PopStyleColor();
        ImGui::Unindent(ti);
    }
    if (ind > 0) ImGui::Unindent(ind);
    if (option_value(o)) option_children(o, depth + 1);
    ImGui::Spacing();
    ImGui::PopID();
}

/* the presets, as switches are: a name in bold, what it is in grey; the one in force marked, or
   "Custom" with the nearest and how many switches differ */
static void presets_block(void)
{
    int cur = preset_current();
    for (int p : PRESET_SHOWN) {                    /* (Darklands+ first, Full Original last) */
        ImGui::PushID(p);
        if (ImGui::RadioButton("##p", cur == p)) { preset_apply(p); ui_dirty_config = 1; cur = preset_current(); }
        ImGui::SameLine();
        std::string nm = std::string(PRESETS[p].name) + (p == PRESET_PLUS ? " (recommended)" : "");
        if (ui_title_desc(nm.c_str(), PRESETS[p].desc)) { preset_apply(p); ui_dirty_config = 1; cur = preset_current(); }
        ImGui::PopID();
    }
    ImGui::PushID("custom");
    bool kept = config_get_int("custom.saved", 0) != 0;
    if (cur == PRESET_CUSTOM) {
        int d = 0, near = preset_nearest(&d), shown = 0;
        std::string t = std::string("你自己的选择：") + PRESETS[near].name + "，但";
        int n = options_count();
        for (int i = 0; i < n; i++) {                   /* (the switches that differ, by name: the first few) */
            const Option *o = options_get(i); int pv = preset_value(o, near), v = option_value(o);
            if (pv == PRESET_KEEP || v == pv) continue;
            if (shown < 3) {
                char lab[96]; std::string val = o->choices ? option_choice(o, 0, lab, sizeof lab) : (v ? "on" : "off");
                if (o->choices) for (int k = 0; k < option_nchoices(o); k++) if (o->vals[k] == v) val = option_choice(o, k, lab, sizeof lab);
                t += std::string(shown ? ", " : "") + o->title + " " + val;
            }
            shown++;
        }
        if (config_get_int("original", 0) && near != PRESET_BUGGY_PURIST) { t += shown ? ", " : ""; t += "原版游戏"; shown++; }
        if (shown > 3) { char b[48]; snprintf(b, sizeof b, "以及 %d 个更多", shown - 3); t += b; }
        t += ".";
        ImGui::RadioButton("##p", true); ImGui::SameLine(); ui_title_desc("自定义", t.c_str());
    } else if (kept) {
        bool c = false;
        if (ImGui::RadioButton("##p", false)) c = true;
        ImGui::SameLine();
        if (ui_title_desc("自定义", "你在选择预设前的个人设置，仍被保留：点击即可恢复。") || c) { preset_restore_custom(); ui_dirty_config = 1; }
    }
    ImGui::PopID();
    ImGui::Spacing();
    {   /* Ironman (mods/saves.c): chosen for the next new game, whatever the preset */
        ImGui::PushID("iron");
        bool im = config_get_int("saves.ironman", 0) != 0, click = false;
        if (ImGui::Checkbox("##iron", &im)) click = true;
        ImGui::SameLine();
        if (ui_title_desc("铁人模式，适用于你的下一次新游戏", "一份存档，由游戏本身保存：每次翻牌、到达地点、每场战斗前以及退出时都会自动覆盖保存。"
                          "在该局游戏进行期间无法加载回退，且当队伍旅程结束时，存档将被删除。无论何种预设。")) { im = !im; click = true; }
        if (click) { config_set_int("saves.ironman", im); config_save(); }
        if (saves_ironman()) ImGui::TextColored(ImVec4(0.62f, 0.32f, 0.08f, 1), "正在进行的游戏即为铁人模式。");
        ImGui::PopID();
    }
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.40f, 0.16f, 1));
    ImGui::TextWrapped("存档与原版游戏双向兼容，不受预设影响。");   /* (wrapped: a narrow window has no room for it on one line) */
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    ImGui::TextWrapped("Darklands+ 以原版游戏格式保存：其存档可在原版中加载（包括 DOS 版），而原版的存档也可在此加载；"
                       "Darklands+ 在此基础上记录的内容（如队伍对女巫安息日的了解、标记为“保留”的物品、精灵与色彩等）会单独存入专属文件。切换预设"
                       "或随时单独开关均安全。");
    ImGui::PopStyleColor();
    ImGui::TextColored(ImVec4(0.62f, 0.32f, 0.08f, 1), "唯一例外：");
    ImGui::SameLine();
    ImGui::TextWrapped("若存档中有成员使用了敌方精灵或添加了徽章（Darklands+ 同时提供这两项功能），则只能在 Darklands+ 中加载。");
}

/* the frames, the buttons, the clock and the potions' grade marks: one section */
static void look_all(void)
{
    ImGui::TextUnformatted("窗户周围的边框");
    look_ornaments();
    ImGui::Spacing(); ImGui::TextUnformatted("时间流逝时的时钟");
    look_clock();
    ImGui::Spacing();
    display_icons();
}
static const char *const LOOKS[] = { "文本", "框架与时钟" };
static void (*const LOOK_FN[])(void) = { look_text, look_all };
static int look_index(const char *s) { for (int i = 0; i < 2; i++) if (!strcmp(s, LOOKS[i])) return i; return -1; }
void ui_section_icon(ImDrawList *dl, ImVec2 c, float s, const char *name);   /* ui/skillicons.cpp */
static char h_jump_to[64];
extern "C" void settings_helpers_jump(const char *name) { snprintf(h_jump_to, sizeof h_jump_to, "%s", name); }   /* (test: uhsect) */
void tab_helpers(void)
{
    /* the sections, from the switches' table: bookmarks at the left (a click opens the section and
       brings it to the top), each section folding away under its heading; the bookmark of the
       section at the top of the view is marked */
    std::vector<const char *> sect; sect.push_back("预设");
    bool orig = ui_original_mode();
    static const char *const SETUP[] = { "屏幕", "开场", "音效", "控制器" };   /* (the computer's side: shown in every preset) */
    static void (*const SETUP_FN[])(void) = { setup_screen, setup_intro, setup_sound, controller_prefs };
    auto setup_index = [](const char *s) { for (int i = 0; i < 4; i++) if (!strcmp(s, SETUP[i])) return i; return -1; };
    if (!orig) {
        int n = options_count();
        for (int i = 0; i < n; i++) {
            const Option *o = options_get(i);
            if ((o->flags & OF_ELSEWHERE) || option_bundled(o) || o->cls == OC_EASY || !strcmp(o->section, "重建内容")) continue;   /* (those: their own pages, tab_restored, tab_mods) */
            bool seen = false; for (const char *s : sect) if (!strcmp(s, o->section)) seen = true;
            if (!seen) sect.push_back(o->section);
            if (!seen && !strcmp(o->section, "新屏幕"))      /* what things look like, after the screens */
                for (const char *l : LOOKS) sect.push_back(l);
        }
    }
    for (const char *l : SETUP) sect.push_back(l);
    for (size_t i = 0; i < sect.size(); i++)                                  /* the fixes, a long list: at the bottom */
        if (!strcmp(sect[i], "Fixes for bugs of the original")) { const char *f = sect[i]; sect.erase(sect.begin() + i); sect.push_back(f); break; }
    const int NSECT = (int)sect.size();
    static int jump = -1, at_top = 0;
    if (h_jump_to[0]) { for (int i = 0; i < NSECT; i++) if (!strcmp(sect[i], h_jump_to)) jump = i; h_jump_to[0] = 0; }
    static bool fold_all = false;
    ImGui::BeginChild("##hnav", ImVec2(UIS(230), 0), ImGuiChildFlags_Borders);
    float ih = ImGui::GetTextLineHeight();
    for (int i = 0; i < NSECT; i++) {
        const char *s = sect[i]; char lab[80];
        snprintf(lab, sizeof lab, "%s###nav%d", !strcmp(s, "Fixes for bugs of the original") ? "修复内容" : s, i);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ih * 1.5f);           /* (room for the section's picture) */
        if (ImGui::Selectable(lab, i == at_top)) jump = i;
        ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
        ui_section_icon(ImGui::GetWindowDrawList(), ImVec2(a.x - ih * 0.8f, (a.y + b.y) * 0.5f), ih * 1.1f, s);
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##hbody", ImVec2(0, 0));
    int sec_i = 0, top_found = -1; float view_top = ImGui::GetWindowPos().y + UIS(30);   /* (a heading brought to the top counts as in view) */
    auto section = [&](const char *name, bool foldable) -> bool {
        int i = sec_i++;
        ImGui::PushID(name);
        if (jump == i) ImGui::SetNextItemOpen(true);
        else if (fold_all && foldable) ImGui::SetNextItemOpen(false);
        ImGui::PushFont(ui_font_head);
        bool open = foldable ? ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen) : (orn_septext(name), true);
        ImGui::PopFont();
        if (jump == i) { ImGui::SetScrollHereY(0.0f); jump = -1; }
        if (ImGui::GetItemRectMin().y <= view_top) top_found = i;       /* (the last heading above the top: the section in view) */
        ImGui::PopID();
        return open;
    };
    static int wlog = -1; if (wlog < 0) wlog = getenv("DL_UI_WIDTH") != 0;   /* (test: a section wider than the page, logged) */
    int open_group = -2;                                                       /* (the section whose width is being measured; -1 Preset) */
    auto too_wide = [&](const char *name) {
        ImGui::EndGroup();
        float over = ImGui::GetItemRectMax().x - (ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x);
        if (over > 0.5f) fprintf(stderr, "settings: \"%s\" is %.0f px wider than the page\n", name, over);
    };
    section("预设", false);
    if (wlog) { ImGui::BeginGroup(); open_group = -1; }
    presets_block();
    if (orig) {
        ImGui::Spacing();
        ui_hint("Darklands+ 所有新增内容均处于关闭状态，其开关也隐藏；如需更改个别开关，请选择其他预设。");
    }
    {
        int n = options_count();
        for (int k = 1; k < NSECT; k++) {
            if (open_group > -2) { too_wide(open_group < 0 ? "预设" : sect[open_group]); open_group = -2; }
            ImGui::Spacing();
            if (!section(sect[k], true)) continue;
            if (wlog) { ImGui::BeginGroup(); open_group = k; }
            {   int l = look_index(sect[k]);
                if (l >= 0) { LOOK_FN[l](); continue; } }
            {   int l = setup_index(sect[k]);
                if (l >= 0) { SETUP_FN[l](); continue; } }
            bool fixes = !strcmp(sect[k], "Fixes for bugs of the original");
            if (fixes) {                                  /* (a long list, each its own switch since what counts as a bug is open to argument: folded) */
                int nf = 0, on = 0;
                for (int i = 0; i < n; i++) { const Option *o = options_get(i); if (o->cls == OC_FIX && !strcmp(o->section, sect[k])) { nf++; on += option_value(o) != 0; } }
                ui_hint("原版游戏的漏洞已修复（%d 个中的 %d）。每个漏洞都有独立开关，因为何谓漏洞（以及如何修补）尚存争议；"
                        "漏洞按严重程度从明显破损到存疑分类排列。", on, nf);
                for (int g = 0; g < FIX_NGROUPS; g++) {     /* (fixgroups.c) */
                    int gn = 0, gon = 0;
                    for (int i = 0; i < n; i++) { const Option *o = options_get(i); if (o->cls == OC_FIX && !strcmp(o->section, sect[k]) && fix_group(o->key + 4) == g) { gn++; gon += option_value(o) != 0; } }
                    if (!gn) continue;
                    char lab[128]; snprintf(lab, sizeof lab, "%s (%d of %d on)###fixg%d", FIX_GROUPS[g].name, gon, gn, g);
                    if (wlog) ImGui::SetNextItemOpen(true);                  /* (measuring: every group open) */
                    if (!ImGui::TreeNodeEx(lab, ImGuiTreeNodeFlags_SpanAvailWidth)) continue;
                    ui_hint("%s", FIX_GROUPS[g].what);
                    for (int i = 0; i < n; i++) {
                        const Option *o = options_get(i);
                        if (o->cls != OC_FIX || strcmp(o->section, sect[k]) || o->parent || fix_group(o->key + 4) != g) continue;
                        option_row(o, 0);
                    }
                    ImGui::TreePop();
                }
                continue;
            }
            for (int i = 0; i < n; i++) {
                const Option *o = options_get(i);
                if ((o->flags & OF_ELSEWHERE) || option_bundled(o) || o->parent || strcmp(o->section, sect[k])) continue;
                option_row(o, 0);
            }
        }
        if (open_group > -2) { too_wide(open_group < 0 ? "预设" : sect[open_group]); open_group = -2; }
    }
    at_top = top_found < 0 ? 0 : top_found;
    fold_all = false;
    if (wlog && ImGui::GetScrollMaxX() > 0) fprintf(stderr, "settings: the page scrolls sideways by %.0f px\n", ImGui::GetScrollMaxX());
    ImGui::EndChild();
    if (wlog && ImGui::GetScrollMaxX() > 0) fprintf(stderr, "settings: the window scrolls sideways by %.0f px\n", ImGui::GetScrollMaxX());
}

/* what MicroProse wrote, coded or left unfinished and never switched on, reconstructed by
   Darklands+: a page of its own, experimental, every switch off by default and in every preset;
   in two parts, by how much Darklands+ had to supply (docs/cut_content.md) */
void tab_restored(void)
{
    ImGui::PushFont(ui_font_head); orn_septext("重建内容"); ImGui::PopFont();
    ImGui::TextColored(ImVec4(0.75f, 0.35f, 0.1f, 1), "Experimental.");
    ImGui::SameLine();
    ImGui::TextWrapped("MicroProse 曾为 Darklands 编写或编码但从未启用的内容，现由 Darklands+ 重新补足。1992 年无人实际体验过："
                       "请做好心理准备，可能存在粗糙之处。此处所有开关默认关闭，除非你主动勾选；而每个预设也会再次将其全部关闭。");
    ui_hint("每个内容在游戏文件中的原貌，以及 Darklands+ 必须补充或修正的部分，均记录于 docs/cut_content.md（及 docs/cut_politics.md）。");
    ImGui::Spacing();
    if (ui_original_mode()) { ui_hint("原版游戏运行时关闭（全原版）：可选择其他预设来启用这些功能。"); return; }
    static const char *const FINISHED[] = { "rules.politics", "rules.war", "battle.black_cloud", 0 };   /* (the rest: rules.fehde, rules.hospice, assist.hall_of_fame) */
    auto finished = [](const char *k) { for (int i = 0; FINISHED[i]; i++) if (!strcmp(FINISHED[i], k)) return true; return false; };
    int n = options_count();
    for (int part = 0; part < 2; part++) {
        ImGui::PushFont(ui_font_head); orn_septext(part ? "已编写，但从未完成" : "已完成，但从未启用"); ImGui::PopFont();
        ui_hint(part ? "文本或图片存在于游戏文件中，但游戏从未显示它们：Darklands+ 提供了缺失的部分。"
                     : "由 MicroProse 编写并编码，只是从未启用（或者对于 Black Cloud 而言，被排除在菜单之外）：Darklands+ 将其启用。");
        for (int i = 0; i < n; i++) {
            const Option *o = options_get(i);
            if ((o->flags & OF_ELSEWHERE) || option_bundled(o) || o->parent || strcmp(o->section, "重建内容") || finished(o->key) != (part == 0)) continue;
            option_row(o, 0);
        }
        ImGui::Spacing();
    }
}

