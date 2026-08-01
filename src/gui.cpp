#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>

#include "imgui.h"
#include "file_operations.h"
#include "player_data.h"
#include "player_stats.h"
#include "upgrading.h"
#include "graph.h"

static Player g_player = {0};
static ProgressData g_progress[300] = {0};
static int g_progress_count = 0;
static bool g_data_loaded = false;
static char g_status_message[256] = "";

static int g_tab_index = 0;
static int g_upgrade_type = 0;
static long long g_upgrade_from = 1;
static long long g_upgrade_to = 2;
static double g_upgrade_cost = 0.0;
static unsigned long long g_upgrade_cost_value = 0ULL;
static bool g_upgrade_ready = false;

static float g_ratio_leader = 0.0f;
static float g_ratio_colony = 0.0f;
static float g_ratio_town_archer = 0.0f;
static float g_ratio_castle = 0.0f;
static double g_colony_gold = 0.0;
static double g_gold_xp = 0.0;
static double g_gold_whip = 0.0;
static char g_custom_hero_name[64] = "";
static float g_custom_hero_target_ratio = 0.04f;
static int g_custom_hero_level = 1;
static CustomHero g_custom_heroes[32] = {0};
static int g_custom_hero_count = 0;
static double g_save_confirmation_until = 0.0;
static char g_save_confirmation_target[64] = "";

static void MarkDataSaved(const char* target)
{
    snprintf(g_save_confirmation_target, sizeof(g_save_confirmation_target), "%s", target);
    g_save_confirmation_until = ImGui::GetTime() + 2.5;
}

static void DrawSaveConfirmation(const char* target)
{
    if (g_save_confirmation_until > ImGui::GetTime() && strcmp(g_save_confirmation_target, target) == 0) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Data Saved");
    }
}

static void DrawSectionHeading(const char* text)
{
    ImGui::PushFont(GetUiBoldFont());
    ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s", text);
    ImGui::PopFont();
}

static bool BeginBoldTabItem(const char* label)
{
    ImGui::PushFont(GetUiBoldFont());
    bool open = ImGui::BeginTabItem(label);
    ImGui::PopFont();
    return open;
}

static void RefreshPlayerData() {
    if (load_last_player_data(&g_player)) {
        g_data_loaded = true;
        snprintf(g_status_message, sizeof(g_status_message), "Loaded last saved player data: %s, wave=%d", g_player.last_update, g_player.wave);
    } else {
        g_data_loaded = false;
        g_player.recommended_ratios = (RecommendedRatios){
            DEFAULT_LEADER_RATIO,
            DEFAULT_TOWN_ARCHER_RATIO,
            DEFAULT_CASTLE_RATIO
        };
        snprintf(g_status_message, sizeof(g_status_message), "No previous saved player data found.");
    }
}

static void RefreshProgressHistory() {
    g_progress_count = read_progress_history("data/player_data.csv", g_progress, 300);
    if (g_progress_count <= 0) {
        g_progress_count = 0;
    }
}

static void RefreshCustomHeroes() {
    g_custom_hero_count = load_custom_heroes(g_custom_heroes, 32);
}

static void AddCustomHeroFromGui() {
    if (g_custom_hero_name[0] == '\0') {
        snprintf(g_status_message, sizeof(g_status_message), "Please enter a hero name.");
        return;
    }

    CustomHero hero;
    memset(&hero, 0, sizeof(hero));
    snprintf(hero.name, sizeof(hero.name), "%s", g_custom_hero_name);
    hero.target_ratio = g_custom_hero_target_ratio;
    hero.level = g_custom_hero_level;

    if (!save_custom_hero(&hero)) {
        snprintf(g_status_message, sizeof(g_status_message), "Failed to save custom hero.");
        return;
    }

    RefreshCustomHeroes();
    g_custom_hero_name[0] = '\0';
    g_custom_hero_target_ratio = 0.04f;
    g_custom_hero_level = 1;
    MarkDataSaved("add_custom_hero");
    snprintf(g_status_message, sizeof(g_status_message), "Custom hero saved successfully.");
}

static void ComputeRatios() {
    analyze_player_data(&g_player, &g_ratio_leader, &g_ratio_colony, &g_ratio_town_archer, &g_ratio_castle);
    g_colony_gold = colony_stats_calculation(&g_player);
    g_gold_xp = g_colony_gold * 1.20;
    g_gold_whip = g_colony_gold * 1.35;
}

static bool SavePlayerData() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(g_player.last_update, sizeof(g_player.last_update), "%Y-%m-%d", t);
    if (save_player_data(&g_player)) {
        snprintf(g_status_message, sizeof(g_status_message), "Player data saved successfully. Last update: %s", g_player.last_update);
        RefreshProgressHistory();
        g_data_loaded = true;
        return true;
    } else {
        snprintf(g_status_message, sizeof(g_status_message), "Failed to save player data.");
        return false;
    }
}

static void SaveRecommendedRatios() {
    if (g_player.recommended_ratios.leader <= 0.0f || g_player.recommended_ratios.leader > 10.0f ||
        g_player.recommended_ratios.town_archer <= 0.0f || g_player.recommended_ratios.town_archer > 10.0f ||
        g_player.recommended_ratios.castle <= 0.0f || g_player.recommended_ratios.castle > 10.0f) {
        snprintf(g_status_message, sizeof(g_status_message), "Ratios must be greater than 0 and no more than 10.");
        return;
    }

    if (g_player.wave <= 0 || g_player.infinity_castle_level <= 0 || g_player.leader_level <= 0 ||
        g_player.town_archer_level <= 0 || g_player.castle_level <= 0) {
        snprintf(g_status_message, sizeof(g_status_message), "Enter and save valid player data before saving ratios.");
        return;
    }

    if (SavePlayerData()) {
        MarkDataSaved("recommended_ratios");
        snprintf(g_status_message, sizeof(g_status_message), "Recommended ratios saved successfully.");
    }
}

static void SaveCustomHeroesFromGui() {
    for (int i = 0; i < g_custom_hero_count; ++i) {
        if (g_custom_heroes[i].target_ratio <= 0.0f || g_custom_heroes[i].target_ratio > 10.0f || g_custom_heroes[i].level < 1) {
            snprintf(g_status_message, sizeof(g_status_message), "Custom hero ratios must be greater than 0 and no more than 10; levels must be positive.");
            return;
        }
    }

    if (!save_custom_heroes(g_custom_heroes, g_custom_hero_count)) {
        snprintf(g_status_message, sizeof(g_status_message), "Failed to save custom heroes.");
        return;
    }

    MarkDataSaved("custom_heroes");
    snprintf(g_status_message, sizeof(g_status_message), "Custom heroes saved successfully.");
}

static void AddThousandsSeparator(unsigned long long value, char* out, size_t out_size) {
    char temp[64];
    int len = 0;
    if (value == 0) {
        temp[len++] = '0';
    }
    while (value > 0 && len < (int)sizeof(temp) - 1) {
        temp[len++] = (char)('0' + (value % 10));
        value /= 10;
    }
    int dst = 0;
    for (int i = 0; i < len; ++i) {
        if (i > 0 && (i % 3) == 0) {
            if (dst < (int)out_size - 1) {
                out[dst++] = ',';
            }
        }
        if (dst < (int)out_size - 1) {
            out[dst++] = temp[len - 1 - i];
        }
    }
    out[dst] = '\0';
}

static void FormatGoldAmount(unsigned long long amount, char* out, size_t out_size) {
    if (amount >= 1000000000000ULL) {
        double scaled = (double)amount / 1000000000000.0;
        snprintf(out, out_size, "%.2f Trillion", scaled);
    } else if (amount >= 1000000000ULL) {
        double scaled = (double)amount / 1000000000.0;
        snprintf(out, out_size, "%.2f Billion", scaled);
    } else if (amount >= 1000000ULL) {
        double scaled = (double)amount / 1000000.0;
        snprintf(out, out_size, "%.2f Million", scaled);
    } else {
        AddThousandsSeparator(amount, out, out_size);
    }
}

static void DrawPlayerDataTab() {
    ImGui::Text("Enter your player progress details and save them to the local CSV.");
    ImGui::Spacing();
    DrawSectionHeading("INPUT SECTION");
    ImGui::Spacing();

    ImGui::InputInt("Wave", &g_player.wave);
    ImGui::InputInt("Infinity Castle Level", &g_player.infinity_castle_level);
    ImGui::InputInt("Leader Level", &g_player.leader_level);
    ImGui::InputInt("Town Archer Level", &g_player.town_archer_level);
    ImGui::InputInt("Castle Level", &g_player.castle_level);

    if (ImGui::Button("Save Player Data")) {
        if (g_player.wave > 0 && g_player.infinity_castle_level > 0 && g_player.leader_level > 0 && g_player.town_archer_level > 0 && g_player.castle_level > 0) {
            if (SavePlayerData()) {
                MarkDataSaved("player_data");
            }
            ComputeRatios();
        } else {
            snprintf(g_status_message, sizeof(g_status_message), "Please enter positive values for all fields.");
        }
    }
    DrawSaveConfirmation("player_data");

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    DrawSectionHeading("DISPLAY SECTION");
    ImGui::Spacing();
    ImGui::Text("Wave: %d", g_player.wave);
    ImGui::Text("Infinity Castle Level: %d", g_player.infinity_castle_level);
    ImGui::Text("Leader Level: %d", g_player.leader_level);
    ImGui::Text("Town Archer Level: %d", g_player.town_archer_level);
    ImGui::Text("Castle Level: %d", g_player.castle_level);
    ImGui::Text("Last Update: %s", g_player.last_update[0] ? g_player.last_update : "N/A");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    DrawSectionHeading("CUSTOM HEROES");
    ImGui::InputText("Hero Name", g_custom_hero_name, IM_ARRAYSIZE(g_custom_hero_name));
    ImGui::InputFloat("Target Ratio", &g_custom_hero_target_ratio, 0.001f, 0.01f, "%.4f");
    ImGui::InputInt("Current Level", &g_custom_hero_level);
    if (ImGui::Button("Add Custom Hero")) {
        AddCustomHeroFromGui();
    }
    DrawSaveConfirmation("add_custom_hero");
    ImGui::SameLine();
    if (ImGui::Button("Refresh Heroes")) {
        RefreshCustomHeroes();
    }

    if (g_custom_hero_count > 0) {
        ImGui::Spacing();
        ImGui::BeginChild("custom_hero_list", ImVec2(0, 120), true);
        for (int i = 0; i < g_custom_hero_count; ++i) {
            CustomHero& hero = g_custom_heroes[i];
            std::string level_label = std::string(hero.name) + " Level##custom_player_level_" + std::to_string(i);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::InputInt(level_label.c_str(), &hero.level);
        }
        ImGui::EndChild();
        if (ImGui::Button("Save Custom Heroes")) {
            SaveCustomHeroesFromGui();
        }
        DrawSaveConfirmation("custom_heroes");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextWrapped("%s", g_status_message);
}

static void DrawRatioSuggestionTab() {
    ComputeRatios();
    ImGui::Text("Ratio analysis based on current player data.");
    ImGui::Spacing();

    ImGui::Columns(4, "ratio_columns", true);
    ImGui::PushFont(GetUiBoldFont());
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Subject"); ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Your Ratio"); ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Target"); ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Gap"); ImGui::NextColumn();
    ImGui::PopFont();
    ImGui::Separator();

    ImGui::Text("Leader"); ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.6f, 1.0f), "%.4f", g_ratio_leader); ImGui::NextColumn();
    ImGui::InputFloat("##leader_ratio", &g_player.recommended_ratios.leader, 0.001f, 0.01f, "%.4f"); ImGui::NextColumn();
    int leader_gap = (int)(g_player.leader_level - g_player.wave * g_player.recommended_ratios.leader);
    if (leader_gap < 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "-%d", -leader_gap);
    } else {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%d", leader_gap);
    }
    ImGui::NextColumn();

    ImGui::Text("Infinity Castle"); ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.6f, 1.0f), "%.4f", g_ratio_colony); ImGui::NextColumn();
    ImGui::Text("As high as possible"); ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "-"); ImGui::NextColumn();

    ImGui::Text("Town Archer"); ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.6f, 1.0f), "%.4f", g_ratio_town_archer); ImGui::NextColumn();
    ImGui::InputFloat("##town_archer_ratio", &g_player.recommended_ratios.town_archer, 0.001f, 0.01f, "%.4f"); ImGui::NextColumn();
    int archer_gap = (int)(g_player.town_archer_level - g_player.wave * g_player.recommended_ratios.town_archer);
    if (archer_gap < 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "-%d", -archer_gap);
    } else {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%d", archer_gap);
    }
    ImGui::NextColumn();

    ImGui::Text("Castle"); ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.6f, 1.0f), "%.4f", g_ratio_castle); ImGui::NextColumn();
    ImGui::InputFloat("##castle_ratio", &g_player.recommended_ratios.castle, 0.001f, 0.01f, "%.4f"); ImGui::NextColumn();
    int castle_gap = (int)(g_player.castle_level - g_player.wave * g_player.recommended_ratios.castle);
    if (castle_gap < 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "-%d", -castle_gap);
    } else {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%d", castle_gap);
    }
    ImGui::NextColumn();

    ImGui::Columns(1);

    if (ImGui::Button("Save Recommended Ratios")) {
        SaveRecommendedRatios();
    }
    DrawSaveConfirmation("recommended_ratios");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    DrawSectionHeading("CUSTOM HEROES");
    if (g_custom_hero_count > 0) {
        ImGui::Columns(5, "custom_ratio_columns", true);
        ImGui::PushFont(GetUiBoldFont());
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Subject"); ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Custom Hero Name"); ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Your Ratio"); ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Target Ratio"); ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Gap"); ImGui::NextColumn();
        ImGui::PopFont();
        ImGui::Separator();

        for (int i = 0; i < g_custom_hero_count; ++i) {
            CustomHero& hero = g_custom_heroes[i];
            ImGui::Text("Custom Hero"); ImGui::NextColumn();
            ImGui::Text("%s", hero.name); ImGui::NextColumn();
            float current_ratio = (g_player.wave > 0) ? (float)hero.level / g_player.wave : 0.0f;
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.6f, 1.0f), "%.4f", current_ratio); ImGui::NextColumn();
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputFloat(("Target##custom_ratio_" + std::to_string(i)).c_str(), &hero.target_ratio, 0.001f, 0.01f, "%.4f");
            ImGui::NextColumn();
            int custom_gap = (int)(hero.level - g_player.wave * hero.target_ratio);
            if (custom_gap < 0) {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "-%d", -custom_gap);
            } else {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%d", custom_gap);
            }
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
        if (ImGui::Button("Save Custom Heroes")) {
            SaveCustomHeroesFromGui();
        }
        DrawSaveConfirmation("custom_heroes");
    } else {
        ImGui::Text("No custom heroes saved yet.");
    }
}

static void DrawColonyStatsTab() {
    ComputeRatios();
    ImGui::Text("Colony stats and gold production based on Infinity Castle level.");
    ImGui::Separator();
    ImGui::Spacing();
    DrawSectionHeading("CORE METRICS");
    ImGui::Text("Infinity Castle Level: %d", g_player.infinity_castle_level);
    ImGui::Text("Colony Ratio: %.4f", g_ratio_colony);
    ImGui::Spacing();
    DrawSectionHeading("GOLD PRODUCTION");
    ImGui::Text("Base Gold: %.0f", g_colony_gold);
    ImGui::Text("Gold with XP Buff (x1.20): %.0f", g_gold_xp);
    ImGui::Text("Gold with Whip + Skill (x1.35): %.0f", g_gold_whip);
}

static void DrawProgressHistoryTab() {
    RefreshProgressHistory();

    ImGui::Text("Progress history loaded from data/player_data.csv");
    ImGui::Text("Entries: %d", g_progress_count);
    ImGui::Separator();

    if (g_progress_count > 0) {
        const int max_display = 20;
        int display_count = g_progress_count;
        int start_index = 0;
        if (display_count > max_display) {
            start_index = g_progress_count - max_display;
            display_count = max_display;
        }

        double ratios[20];
        double min_ratio = 1e9;
        double max_ratio = -1e9;

        for (int i = 0; i < display_count; ++i) {
            const ProgressData& entry = g_progress[start_index + i];
            ratios[i] = (entry.wave > 0) ? (double)entry.infinity_castle_level / entry.wave : 0.0;
            min_ratio = (ratios[i] < min_ratio) ? ratios[i] : min_ratio;
            max_ratio = (ratios[i] > max_ratio) ? ratios[i] : max_ratio;
        }

        double ratio_span = max_ratio - min_ratio;
        double ratio_padding = ratio_span > 0.0
            ? ratio_span * 0.1
            : ((max_ratio > 0.0 ? max_ratio : 1.0) * 0.1);
        double graph_min_ratio = min_ratio - ratio_padding;
        double graph_max_ratio = max_ratio + ratio_padding;

        ImGui::Text("Showing latest %d entries.", display_count);
        ImGui::Spacing();

        ImVec2 graph_size = ImVec2(-1.0f, 220.0f);
        ImGui::Text("Infinity Castle Level / Wave Ratio");
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 0.8f), "Ratio = Infinity Castle Level divided by Wave   |   ● Entry   ─ Trend");
        ImGui::Spacing();
        ImGui::InvisibleButton("progress_graph", graph_size);
        ImVec2 graph_min = ImGui::GetItemRectMin();
        ImVec2 graph_max = ImGui::GetItemRectMax();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImU32 background_color = ImGui::GetColorU32(ImGuiCol_FrameBg);
        draw_list->AddRectFilled(graph_min, graph_max, background_color);
        draw_list->AddRect(graph_min, graph_max, IM_COL32(110, 110, 130, 255));

        float width = graph_max.x - graph_min.x;
        float height = graph_max.y - graph_min.y;
        float ratio_range = (float)(graph_max_ratio - graph_min_ratio);

        for (int row = 0; row <= 4; ++row) {
            float t = row / 4.0f;
            float y = graph_min.y + height * t;
            draw_list->AddLine(ImVec2(graph_min.x, y), ImVec2(graph_max.x, y), IM_COL32(80, 80, 100, 255));
            double value = graph_max_ratio - (ratio_range * t);
            char label[32];
            snprintf(label, sizeof(label), "%.2f", value);
            draw_list->AddText(ImVec2(graph_min.x + 4, y - 8), IM_COL32(200, 200, 200, 200), label);
        }

        int tick_count = display_count < 5 ? display_count : 5;
        for (int ti = 0; ti < tick_count; ++ti) {
            int idx = tick_count == 1
                ? 0
                : (int)(((double)ti * (display_count - 1)) / (tick_count - 1) + 0.5);
            if (idx < 0) idx = 0;
            if (idx >= display_count) idx = display_count - 1;
            float x = display_count == 1
                ? graph_min.x + width * 0.5f
                : graph_min.x + ((float)idx / (display_count - 1)) * width;
            draw_list->AddLine(ImVec2(x, graph_max.y), ImVec2(x, graph_max.y + 6), IM_COL32(120, 120, 150, 255));
            char label[32];
            snprintf(label, sizeof(label), "%d", g_progress[start_index + idx].wave);
            ImVec2 text_size = ImGui::CalcTextSize(label);
            draw_list->AddText(ImVec2(x - text_size.x * 0.5f, graph_max.y + 8), IM_COL32(220, 220, 220, 200), label);
        }

        ImVec2 previous_point = ImVec2(0, 0);
        for (int i = 0; i < display_count; ++i) {
            float x = display_count == 1
                ? graph_min.x + width * 0.5f
                : graph_min.x + ((float)i / (display_count - 1)) * width;
            float normalized = (float)((ratios[i] - graph_min_ratio) / ratio_range);
            float y = graph_max.y - normalized * height;
            ImVec2 point(x, y);
            if (i > 0) {
                draw_list->AddLine(previous_point, point, IM_COL32(100, 220, 100, 255), 2.0f);
            }
            draw_list->AddCircleFilled(point, 3.5f, IM_COL32(255, 160, 80, 255));
            previous_point = point;
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Dummy(ImVec2(0, 20.0f));

        if (ImGui::BeginTable("history_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))  {
            ImGui::TableSetupColumn("Date");
            ImGui::TableSetupColumn("Wave");
            ImGui::TableSetupColumn("Infinity Castle");
            ImGui::TableSetupColumn("Ratio");
            ImGui::TableHeadersRow();

            for (int i = start_index; i < g_progress_count; ++i) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("%s", g_progress[i].date);
                ImGui::TableNextColumn(); ImGui::Text("%d", g_progress[i].wave);
                ImGui::TableNextColumn(); ImGui::Text("%d", g_progress[i].infinity_castle_level);
                ImGui::TableNextColumn(); ImGui::Text("%.4f", g_progress[i].wave > 0 ? (double)g_progress[i].infinity_castle_level / g_progress[i].wave : 0.0);
            }
            ImGui::EndTable();
        }
    } else {
        ImGui::TextWrapped("No progress history available. Save player data to build up the history.");
    }
}

static void DrawUpgradingCostTab() {
    ImGui::Text("Calculate gold cost for upgrades.");
    ImGui::Separator();

    ImGui::Combo("Upgrade Type", &g_upgrade_type, "Castle\0Town Archers\0Hero/Leader/Tower\0");
    ImGui::InputScalar("From level", ImGuiDataType_S64, &g_upgrade_from);
    ImGui::InputScalar("To level", ImGuiDataType_S64, &g_upgrade_to);

    if (ImGui::Button("Compute Cost")) {
        if (g_upgrade_from > 0 && g_upgrade_to > 0 && g_upgrade_to > g_upgrade_from) {
            unsigned long long cost = 0ULL;
            if (g_upgrade_type == 0) {
                cost = 1250ULL * (unsigned long long)(g_upgrade_to - g_upgrade_from) * (unsigned long long)(g_upgrade_to + g_upgrade_from - 1);
            } else if (g_upgrade_type == 1) {
                cost = 500ULL * (unsigned long long)(g_upgrade_to - g_upgrade_from) * (unsigned long long)(g_upgrade_to + g_upgrade_from);
            } else {
                long long current = g_upgrade_from;
                if (current < 5000) {
                    long long limit = (g_upgrade_to < 5000) ? g_upgrade_to : 5000;
                    unsigned long long limit_sq = (unsigned long long)limit * limit;
                    unsigned long long current_sq = (unsigned long long)current * current;
                    cost += 1500ULL * (limit_sq - current_sq);
                    current = limit;
                }
                if (current < 10000 && current < g_upgrade_to) {
                    long long limit = (g_upgrade_to < 10000) ? g_upgrade_to : 10000;
                    unsigned long long limit_sq = (unsigned long long)limit * limit;
                    unsigned long long current_sq = (unsigned long long)current * current;
                    cost += 2000ULL * (limit_sq - current_sq);
                    current = limit;
                }
                if (current < g_upgrade_to) {
                    unsigned long long to_sq = (unsigned long long)g_upgrade_to * (unsigned long long)g_upgrade_to;
                    unsigned long long current_sq = (unsigned long long)current * current;
                    cost += 2500ULL * (to_sq - current_sq);
                }
            }
            g_upgrade_cost_value = cost;
            g_upgrade_cost = (double)cost;
            g_upgrade_ready = true;
        } else {
            g_upgrade_ready = false;
            g_upgrade_cost_value = 0ULL;
            g_upgrade_cost = 0.0;
        }
    }

    if (g_upgrade_ready) {
        char cost_text[64];
        FormatGoldAmount(g_upgrade_cost_value, cost_text, sizeof(cost_text));
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        DrawSectionHeading("CALCULATION RESULT");
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Estimated Cost: %s Gold", cost_text);
        ImGui::Spacing();
    } else {
        ImGui::Text("Enter valid levels and press Compute Cost.");
    }
}

static void DrawExportImportTab() {
    ImGui::Text("All data is stored locally in the ./data folder.");
    ImGui::Spacing();
    ImGui::Text("Stored files:");
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "./data/player_data.csv");
    ImGui::Text("Player progress and history");
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "./data/custom_heroes.csv");
    ImGui::Text("Custom hero names, ratios, and levels");
    ImGui::Separator();

    ImGui::TextWrapped("Backup: copy both CSV files to a safe location. Restore: copy both files back into the ./data folder with the same names, then restart the app.");
    ImGui::Spacing();
    ImGui::TextWrapped("If a file is missing, that category has no saved data yet.");
}

void ShowApplication() {
    if (!g_data_loaded) {
        RefreshPlayerData();
        RefreshProgressHistory();
        RefreshCustomHeroes();
        ComputeRatios();
    }

    ImGui::Begin("Grow Castle Progress Tracker");
    if (ImGui::BeginTabBar("MainTabs")) {
        if (BeginBoldTabItem("Player Data")) {
            DrawPlayerDataTab();
            ImGui::EndTabItem();
        }
        if (BeginBoldTabItem("Ratio & Suggestion")) {
            DrawRatioSuggestionTab();
            ImGui::EndTabItem();
        }
        if (BeginBoldTabItem("Colony Stats")) {
            DrawColonyStatsTab();
            ImGui::EndTabItem();
        }
        if (BeginBoldTabItem("Progress History")) {
            DrawProgressHistoryTab();
            ImGui::EndTabItem();
        }
        if (BeginBoldTabItem("Upgrading Cost")) {
            DrawUpgradingCostTab();
            ImGui::EndTabItem();
        }
        if (BeginBoldTabItem("Export / Import")) {
            DrawExportImportTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}
