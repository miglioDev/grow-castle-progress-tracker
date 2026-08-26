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
#include "investment.h"
#include "graph.h"
#include "pace_analysis.h"
#include "profit.h"

static Player g_player = {0};
static ProgressData g_progress[300] = {0};
static int g_progress_count = 0;
static bool g_data_loaded = false;
static char g_status_message[256] = "";

static int g_upgrade_type = 0;
static long long g_upgrade_from = 1;
static long long g_upgrade_to = 2;
static double g_upgrade_cost = 0.0;
static bool g_upgrade_ready = false;

static float g_ratio_leader = 0.0f;
static float g_ratio_colony = 0.0f;
static float g_ratio_town_archer = 0.0f;
static float g_ratio_castle = 0.0f;
static double g_colony_gold = 0.0;
static double g_gold_xp = 0.0;
static double g_gold_whip = 0.0;
static int g_projection_days = 5;
static char g_custom_hero_name[64] = "";
static float g_custom_hero_target_ratio = 0.04f;
static long long g_custom_hero_level = 1;
static CustomHero g_custom_heroes[32] = {0};
static int g_custom_hero_count = 0;
static const int MAX_CUSTOM_HEROES = 32;
// Step values for InputScalar so 64-bit level/wave fields keep +/- buttons.
static const long long kInt64Step = 1;
static const long long kInt64StepFast = 100;
static PaceInputs g_pace_inputs = {0};
static PaceStats g_pace_stats = {0};
static int g_pace_history_period = 0;
static double g_save_confirmation_until = 0.0;
static char g_save_confirmation_target[64] = "";
static Player g_pending_player_deletion = {0};
static int g_selected_custom_hero_deletion = 0;

// Centralized palette so status/severity colors stay consistent across tabs.
namespace UiColors {
    static const ImVec4 Success = ImVec4(0.30f, 0.85f, 0.45f, 1.0f);
    static const ImVec4 Danger = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
    static const ImVec4 Warning = ImVec4(1.0f, 0.75f, 0.35f, 1.0f);
    static const ImVec4 Info = ImVec4(0.40f, 0.80f, 1.0f, 1.0f);
    static const ImVec4 Muted = ImVec4(0.62f, 0.66f, 0.72f, 1.0f);
    static const ImVec4 Heading = ImVec4(0.46f, 0.78f, 1.0f, 1.0f);
    static const ImVec4 Subheading = ImVec4(0.62f, 0.78f, 0.92f, 1.0f);
    static const ImVec4 PanelBg = ImVec4(0.075f, 0.12f, 0.19f, 0.72f);
}

typedef struct {
    double wavesPerHour;
    double wavesPerDay;
    double wavesPerSeason;
    double downtimeHours;
    double downtimePercentage;
    double elapsedHours;
    long long referenceWave;
    int entryCount;
    int usesDateOnlyEntries;
    int isValid;
    char message[256];
} HistoricalPaceStats;

typedef struct {
    const char* name;
    InvestmentMetrics metrics;
} InvestmentRow;

static void MarkDataSaved(const char* target)
{
    snprintf(g_save_confirmation_target, sizeof(g_save_confirmation_target), "%s", target);
    g_save_confirmation_until = ImGui::GetTime() + 2.5;
}

static void DrawSaveConfirmation(const char* target)
{
    if (g_save_confirmation_until > ImGui::GetTime() && strcmp(g_save_confirmation_target, target) == 0) {
        ImGui::SameLine();
        ImGui::TextColored(UiColors::Success, "Data Saved");
    }
}

static void DrawSectionHeading(const char* text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, UiColors::Heading);
    ImGui::PushFont(GetUiBoldFont());
    ImGui::TextUnformatted(text);
    ImGui::PopFont();
    ImGui::PopStyleColor();
}

static void DrawSubsectionHeading(const char* text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, UiColors::Subheading);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

// Standard spacer between major sections.
static void DrawSectionBreak()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

// "(?)" marker with a hover tooltip.
static void DrawHelpMarker(const char* text)
{
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static void DrawStatusMessage()
{
    if (g_status_message[0] == '\0') {
        return;
    }
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, UiColors::PanelBg);
    ImGui::BeginChild("status_message", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.68f, 0.84f, 1.0f, 1.0f));
    ImGui::TextWrapped("%s", g_status_message);
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

static void PushDangerButtonStyle()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.20f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f, 0.27f, 0.27f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.42f, 0.14f, 0.14f, 1.0f));
}

static void PushPrimaryButtonStyle()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.32f, 0.56f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.16f, 0.44f, 0.72f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.08f, 0.24f, 0.42f, 1.0f));
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
        snprintf(g_status_message, sizeof(g_status_message), "Loaded last saved player data: %s, wave=%lld", g_player.last_update, g_player.wave);
    } else {
        g_data_loaded = true;
        g_player = {0};
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

static void RefreshPaceData() {
    resetPaceInputs(&g_pace_inputs);
    load_pace_data(&g_pace_inputs);
    calculatePaceStats(&g_pace_inputs, &g_pace_stats);
}

static void SavePaceDataFromGui() {
    calculatePaceStats(&g_pace_inputs, &g_pace_stats);
    if (!g_pace_stats.isValid) {
        snprintf(g_status_message, sizeof(g_status_message), "%s", g_pace_stats.validationMessage);
        return;
    }

    if (!save_pace_data(&g_pace_inputs)) {
        snprintf(g_status_message, sizeof(g_status_message), "Failed to save pace data.");
        return;
    }

    MarkDataSaved("pace_data");
    snprintf(g_status_message, sizeof(g_status_message), "Pace data saved successfully.");
}

static void AddCustomHeroFromGui() {
    if (g_custom_hero_name[0] == '\0') {
        snprintf(g_status_message, sizeof(g_status_message), "Please enter a hero name.");
        return;
    }

    if (strchr(g_custom_hero_name, ',') != NULL) {
        snprintf(g_status_message, sizeof(g_status_message), "Hero name cannot contain a comma.");
        return;
    }

    if (g_custom_hero_count >= MAX_CUSTOM_HEROES) {
        snprintf(g_status_message, sizeof(g_status_message), "Maximum of %d custom heroes reached.", MAX_CUSTOM_HEROES);
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

static bool ParseProgressTimestamp(const char* text, time_t* timestamp, bool* has_time) {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int fields = sscanf(text, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    if (fields != 3 && fields != 6) {
        return false;
    }

    struct tm parsed_time = {};
    parsed_time.tm_year = year - 1900;
    parsed_time.tm_mon = month - 1;
    parsed_time.tm_mday = day;
    parsed_time.tm_hour = hour;
    parsed_time.tm_min = minute;
    parsed_time.tm_sec = second;
    parsed_time.tm_isdst = -1;

    *timestamp = mktime(&parsed_time);
    if (*timestamp == (time_t)-1) {
        return false;
    }

    *has_time = fields == 6;
    return true;
}

static HistoricalPaceStats CalculateHistoricalPaceStats(int period_index) {
    HistoricalPaceStats stats = {};
    const double expected_wph = g_pace_stats.wph;
    const double period_hours[] = {0.0, 30.0 * 24.0, 5.0 * 24.0, 24.0};
    const time_t now = time(NULL);
    const time_t cutoff = period_index == 0
        ? (time_t)0
        : now - (time_t)(period_hours[period_index] * 3600.0);
    int first_index = -1;
    int last_index = -1;
    time_t first_timestamp = 0;
    time_t last_timestamp = 0;

    RefreshProgressHistory();
    for (int index = 0; index < g_progress_count; ++index) {
        time_t timestamp = 0;
        bool has_time = false;
        if (!ParseProgressTimestamp(g_progress[index].date, &timestamp, &has_time) || timestamp < cutoff) {
            continue;
        }

        stats.entryCount++;
        stats.usesDateOnlyEntries |= !has_time;
        if (first_index < 0 || timestamp < first_timestamp) {
            first_index = index;
            first_timestamp = timestamp;
        }
        if (last_index < 0 || timestamp > last_timestamp) {
            last_index = index;
            last_timestamp = timestamp;
        }
    }

    if (first_index < 0 || last_index < 0 || first_index == last_index) {
        snprintf(stats.message, sizeof(stats.message), "At least two Player Data entries are required for the selected period.");
        return stats;
    }

    const double elapsed_hours = difftime(last_timestamp, first_timestamp) / 3600.0;
    const long long completed_waves = g_progress[last_index].wave - g_progress[first_index].wave;
    if (elapsed_hours <= 0.0 || completed_waves < 0 || expected_wph <= 0.0) {
        snprintf(stats.message, sizeof(stats.message), "The selected Player Data entries cannot produce a valid pace comparison.");
        return stats;
    }

    stats.wavesPerHour = (double)completed_waves / elapsed_hours;
    stats.elapsedHours = elapsed_hours;
    stats.referenceWave = g_progress[first_index].wave;
    stats.wavesPerDay = stats.wavesPerHour * 24.0;
    stats.wavesPerSeason = stats.wavesPerHour * 120.0;
    const double expected_hours = (double)completed_waves / expected_wph;
    stats.downtimeHours = elapsed_hours > expected_hours ? elapsed_hours - expected_hours : 0.0;
    stats.downtimePercentage = (stats.downtimeHours / elapsed_hours) * 100.0;
    stats.isValid = 1;
    return stats;
}

static bool SavePlayerData() {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(g_player.last_update, sizeof(g_player.last_update), "%Y-%m-%d %H:%M:%S", t);
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

    // Replace the previous entry instead of appending a ratio-only duplicate row.
    delete_last_player_record();
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

static void FormatGoldValue(double amount, char* out, size_t out_size) {
    if (amount >= 1000000000000.0) {
        snprintf(out, out_size, "%.2f Trillion", amount / 1000000000000.0);
    } else if (amount >= 1000000000.0) {
        snprintf(out, out_size, "%.2f Billion", amount / 1000000000.0);
    } else if (amount >= 1000000.0) {
        snprintf(out, out_size, "%.2f Million", amount / 1000000.0);
    } else {
        snprintf(out, out_size, "%.0f", amount);
    }
}

static void FormatProfitValue(double amount, char* out, size_t out_size) {
    if (amount >= 1000000000000.0) {
        snprintf(out, out_size, "%.2fT", amount / 1000000000000.0);
    } else if (amount >= 1000000000.0) {
        snprintf(out, out_size, "%.2fB", amount / 1000000000.0);
    } else if (amount >= 1000000.0) {
        snprintf(out, out_size, "%.2fM", amount / 1000000.0);
    } else if (amount >= 1000.0) {
        snprintf(out, out_size, "%.2fK", amount / 1000.0);
    } else {
        snprintf(out, out_size, "%.0f", amount);
    }
}

static void DrawInvestmentAndCostSection() {
    InvestmentRow rows[36] = {};
    int row_count = 0;
    HistoricalPaceStats historical_stats = CalculateHistoricalPaceStats(0);
    const bool pace_available = historical_stats.isValid != 0;
    const double pace_wph = pace_available ? historical_stats.wavesPerHour : 0.0;
    DrawSectionBreak();
    DrawSectionHeading("INVESTMENT & COST");
    DrawHelpMarker("Investment (Gold): total gold already spent on this unit.\nInvestment %: this unit's share of total gold spent.\nCost to Target (now): gold needed to reach your target ratio right now.\nCost to Target (next period): gold needed to stay on target after the projection window, based on your actual historical pace.");
    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputInt("Projection Days", &g_projection_days, 1, 1);
    if (g_projection_days < 1) {
        g_projection_days = 1;
    }
    const double projection_hours = g_projection_days * 24.0;

    rows[row_count++] = {"Leader", calculate_investment_metrics(UNIT_TYPE_LEADER, g_player.leader_level,
        g_player.recommended_ratios.leader, g_player.wave, pace_wph, projection_hours)};
    rows[row_count++] = {"Infinity Castle", calculate_investment_metrics(UNIT_TYPE_INFINITY_CASTLE,
        g_player.infinity_castle_level, g_ratio_colony, g_player.wave, pace_wph, projection_hours)};
    rows[row_count++] = {"Town Archer", calculate_investment_metrics(UNIT_TYPE_TOWN_ARCHERS,
        g_player.town_archer_level, g_player.recommended_ratios.town_archer, g_player.wave, pace_wph, projection_hours)};
    rows[row_count++] = {"Castle", calculate_investment_metrics(UNIT_TYPE_CASTLE, g_player.castle_level,
        g_player.recommended_ratios.castle, g_player.wave, pace_wph, projection_hours)};
    for (int index = 0; index < g_custom_hero_count; ++index) {
        rows[row_count].name = g_custom_heroes[index].name;
        rows[row_count].metrics = calculate_investment_metrics(UNIT_TYPE_CUSTOM_HERO, g_custom_heroes[index].level,
            g_custom_heroes[index].target_ratio, g_player.wave, pace_wph, projection_hours);
        row_count++;
    }

    InvestmentMetrics metrics[36] = {};
    for (int index = 0; index < row_count; ++index) {
        metrics[index] = rows[index].metrics;
    }
    calculate_investment_percentages(metrics, row_count);
    for (int index = 0; index < row_count; ++index) {
        rows[index].metrics.investment_percent = metrics[index].investment_percent;
    }

    double total_investment = 0.0;
    double total_now = 0.0;
    double total_next_period = 0.0;
    for (int index = 0; index < row_count; ++index) {
        total_investment += rows[index].metrics.investment_gold;
        total_now += rows[index].metrics.cost_to_target_now;
        total_next_period += rows[index].metrics.cost_to_target_next_period;
    }

    if (!pace_available) {
        ImGui::TextColored(UiColors::Muted,
            "Cost to Target (next period) requires at least two valid Player Data snapshots to calculate actual WPH.");
    } else {
        ImGui::Text("Actual historical pace: %.2f WPH", pace_wph);
    }

    if (ImGui::BeginTable("investment_cost_table", 5,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Unit");
        ImGui::TableSetupColumn("Investment (Gold)", ImGuiTableColumnFlags_WidthStretch, 1.15f);
        ImGui::TableSetupColumn("Investment %", ImGuiTableColumnFlags_WidthStretch, 0.75f);
        ImGui::TableSetupColumn("Cost to Target (now)", ImGuiTableColumnFlags_WidthStretch, 1.15f);
        ImGui::TableSetupColumn("Cost to Target (next period)", ImGuiTableColumnFlags_WidthStretch, 1.35f);
        ImGui::TableHeadersRow();
        for (int index = 0; index < row_count; ++index) {
            char investment[64];
            char cost_now[64];
            char cost_next[64];
            FormatGoldValue(rows[index].metrics.investment_gold, investment, sizeof(investment));
            FormatGoldValue(rows[index].metrics.cost_to_target_now, cost_now, sizeof(cost_now));
            FormatGoldValue(rows[index].metrics.cost_to_target_next_period, cost_next, sizeof(cost_next));
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%s", rows[index].name);
            ImGui::TableNextColumn(); ImGui::Text("%s", investment);
            ImGui::TableNextColumn(); ImGui::Text("%.2f %%", rows[index].metrics.investment_percent * 100.0);
            ImGui::TableNextColumn(); ImGui::Text("%s", cost_now);
            ImGui::TableNextColumn();
            if (pace_available) {
                ImGui::Text("%s", cost_next);
            } else {
                ImGui::TextDisabled("N/A");
            }
        }
        ImGui::EndTable();
    }

    char investment_total[64];
    char now_total[64];
    char next_total[64];
    FormatGoldValue(total_investment, investment_total, sizeof(investment_total));
    FormatGoldValue(total_now, now_total, sizeof(now_total));
    FormatGoldValue(total_next_period, next_total, sizeof(next_total));
    ImGui::Spacing();
    ImGui::Text("Total Investment: %s Gold", investment_total);
    ImGui::Text("Total Cost to Target (now): %s Gold", now_total);
    if (pace_available) {
        ImGui::Text("Total Cost to Target (next period): %s Gold", next_total);
    }

    ImGui::Spacing();
    DrawSectionHeading("GOLD DISTRIBUTION");
    const ImU32 colors[] = {IM_COL32(74, 167, 220, 255), IM_COL32(228, 151, 66, 255), IM_COL32(112, 190, 116, 255), IM_COL32(214, 94, 94, 255), IM_COL32(174, 128, 220, 255)};
    double distribution_total = 0.0;
    for (int index = 0; index < row_count; ++index) {
        if (strcmp(rows[index].name, "Infinity Castle") != 0) {
            distribution_total += rows[index].metrics.investment_gold;
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Legend:");
    for (int index = 0; index < row_count; ++index) {
        if (strcmp(rows[index].name, "Infinity Castle") == 0) {
            continue;
        }
        double distribution_percent = distribution_total > 0.0
            ? rows[index].metrics.investment_gold / distribution_total
            : 0.0;
        ImGui::SameLine();
        ImGui::ColorButton(("##distribution_color_" + std::to_string(index)).c_str(),
            ImGui::ColorConvertU32ToFloat4(colors[index % IM_ARRAYSIZE(colors)]), ImGuiColorEditFlags_NoTooltip, ImVec2(12.0f, 12.0f));
        ImGui::SameLine(0.0f, 3.0f);
        ImGui::Text("%s %.1f%%", rows[index].name, distribution_percent * 100.0);
    }

    ImVec2 bar_start = ImGui::GetCursorScreenPos();
    const float bar_width = ImGui::GetContentRegionAvail().x;
    const float bar_height = 22.0f;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float offset = 0.0f;
    for (int index = 0; index < row_count; ++index) {
        if (strcmp(rows[index].name, "Infinity Castle") == 0) {
            continue;
        }
        float segment_width = distribution_total > 0.0f
            ? bar_width * (float)(rows[index].metrics.investment_gold / distribution_total)
            : 0.0f;
        ImVec2 segment_min(bar_start.x + offset, bar_start.y);
        ImVec2 segment_max(bar_start.x + offset + segment_width, bar_start.y + bar_height);
        draw_list->AddRectFilled(segment_min, segment_max, colors[index % IM_ARRAYSIZE(colors)]);
        if (segment_width > 0.0f && ImGui::IsMouseHoveringRect(segment_min, segment_max)) {
            char segment_gold[64];
            FormatGoldValue(rows[index].metrics.investment_gold, segment_gold, sizeof(segment_gold));
            double segment_percent = distribution_total > 0.0
                ? rows[index].metrics.investment_gold / distribution_total
                : 0.0;
            ImGui::SetTooltip("%s\n%s Gold (%.1f%%)", rows[index].name, segment_gold, segment_percent * 100.0);
        }
        offset += segment_width;
    }
    draw_list->AddRect(bar_start, ImVec2(bar_start.x + bar_width, bar_start.y + bar_height), IM_COL32(180, 180, 190, 255));
    ImGui::Dummy(ImVec2(bar_width, bar_height));
}

static void DrawPlayerDataTab() {
    ImGui::Text("Enter your player progress details and save them to the local CSV.");
    ImGui::Spacing();
    DrawSectionHeading("INPUT SECTION");
    ImGui::Spacing();

    ImGui::SetNextItemWidth(320.0f);
    ImGui::InputScalar("Wave", ImGuiDataType_S64, &g_player.wave, &kInt64Step, &kInt64StepFast);
    if (g_player.wave < 1) g_player.wave = 1;
    ImGui::SetNextItemWidth(320.0f);
    ImGui::InputScalar("Infinity Castle Level", ImGuiDataType_S64, &g_player.infinity_castle_level, &kInt64Step, &kInt64StepFast);
    if (g_player.infinity_castle_level < 0) g_player.infinity_castle_level = 0;
    ImGui::SetNextItemWidth(320.0f);
    ImGui::InputScalar("Leader Level", ImGuiDataType_S64, &g_player.leader_level, &kInt64Step, &kInt64StepFast);
    if (g_player.leader_level < 0) g_player.leader_level = 0;
    ImGui::SetNextItemWidth(320.0f);
    ImGui::InputScalar("Town Archer Level", ImGuiDataType_S64, &g_player.town_archer_level, &kInt64Step, &kInt64StepFast);
    if (g_player.town_archer_level < 0) g_player.town_archer_level = 0;
    ImGui::SetNextItemWidth(320.0f);
    ImGui::InputScalar("Castle Level", ImGuiDataType_S64, &g_player.castle_level, &kInt64Step, &kInt64StepFast);
    if (g_player.castle_level < 0) g_player.castle_level = 0;

    PushPrimaryButtonStyle();
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
    ImGui::PopStyleColor(3);
    DrawSaveConfirmation("player_data");
    ImGui::SameLine();
    PushDangerButtonStyle();
    if (ImGui::Button("Delete Last Saved Data", ImVec2(220.0f, 0.0f))) {
        if (load_last_player_data(&g_pending_player_deletion)) {
            ImGui::OpenPopup("Delete Last Saved Player Data");
        } else {
            snprintf(g_status_message, sizeof(g_status_message), "No saved player data is available to delete.");
        }
    }
    ImGui::PopStyleColor(3);

    if (ImGui::BeginPopupModal("Delete Last Saved Player Data", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("The following saved data will be permanently deleted:");
        ImGui::Separator();
        ImGui::Text("Date / Time: %s", g_pending_player_deletion.last_update);
        ImGui::Text("Wave: %lld", g_pending_player_deletion.wave);
        ImGui::Text("Infinity Castle: %lld", g_pending_player_deletion.infinity_castle_level);
        ImGui::Text("Leader: %lld", g_pending_player_deletion.leader_level);
        ImGui::Text("Town Archers: %lld", g_pending_player_deletion.town_archer_level);
        ImGui::Text("Castle: %lld", g_pending_player_deletion.castle_level);
        ImGui::Spacing();
        ImGui::Text("Do you want to delete this saved data?");
        PushDangerButtonStyle();
        if (ImGui::Button("Delete", ImVec2(120.0f, 0.0f))) {
            if (delete_last_player_record()) {
                RefreshPlayerData();
                RefreshProgressHistory();
                ComputeRatios();
                snprintf(g_status_message, sizeof(g_status_message), "Last saved player data deleted successfully.");
            } else {
                snprintf(g_status_message, sizeof(g_status_message), "Failed to delete the last saved player data.");
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    DrawSectionBreak();
    DrawSectionHeading("DISPLAY SECTION");
    ImGui::Spacing();
    if (ImGui::BeginTable("player_display_table", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthStretch, 0.55f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.45f);
        const char* labels[] = {"Wave", "Infinity Castle Level", "Leader Level", "Town Archer Level", "Castle Level", "Last Update"};
        const char* last_update = g_player.last_update[0] ? g_player.last_update : "N/A";
        char values[6][32];
        snprintf(values[0], sizeof(values[0]), "%lld", g_player.wave);
        snprintf(values[1], sizeof(values[1]), "%lld", g_player.infinity_castle_level);
        snprintf(values[2], sizeof(values[2]), "%lld", g_player.leader_level);
        snprintf(values[3], sizeof(values[3]), "%lld", g_player.town_archer_level);
        snprintf(values[4], sizeof(values[4]), "%lld", g_player.castle_level);
        snprintf(values[5], sizeof(values[5]), "%s", last_update);
        for (int index = 0; index < 6; ++index) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(labels[index]);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(values[index]);
        }
        ImGui::EndTable();
    }

    DrawSectionBreak();
    DrawSectionHeading("Custom");
    ImGui::SetNextItemWidth(420.0f);
    ImGui::InputText("Hero / Tower Name", g_custom_hero_name, IM_ARRAYSIZE(g_custom_hero_name));
    ImGui::SetNextItemWidth(420.0f);
    ImGui::InputFloat("Target Ratio", &g_custom_hero_target_ratio, 0.001f, 0.01f, "%.4f");
    if (g_custom_hero_target_ratio < 0.0f) g_custom_hero_target_ratio = 0.0f;
    DrawHelpMarker("All ratios, including this one, are also editable later in the 'Ratio, Levels & Economy' tab.");
    ImGui::SetNextItemWidth(420.0f);
    ImGui::InputScalar("Current Level", ImGuiDataType_S64, &g_custom_hero_level, &kInt64Step, &kInt64StepFast);
    PushPrimaryButtonStyle();
    if (ImGui::Button("Add Custom Hero")) {
        AddCustomHeroFromGui();
    }
    ImGui::PopStyleColor(3);
    DrawSaveConfirmation("add_custom_hero");

    if (g_custom_hero_count > 0) {
        ImGui::Spacing();
        ImGui::BeginChild("custom_hero_list", ImVec2(0, 120), true);
        for (int i = 0; i < g_custom_hero_count; ++i) {
            CustomHero& hero = g_custom_heroes[i];
            std::string level_label = std::string(hero.name) + " Level##custom_player_level_" + std::to_string(i);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::InputScalar(level_label.c_str(), ImGuiDataType_S64, &hero.level, &kInt64Step, &kInt64StepFast);
        }
        ImGui::EndChild();
        PushPrimaryButtonStyle();
        if (ImGui::Button("Save Custom Heroes", ImVec2(190.0f, 0.0f))) {
            SaveCustomHeroesFromGui();
        }
        ImGui::PopStyleColor(3);
        DrawSaveConfirmation("custom_heroes");
        if (g_selected_custom_hero_deletion >= g_custom_hero_count) {
            g_selected_custom_hero_deletion = g_custom_hero_count - 1;
        }
        const float delete_button_width = 200.0f;
        ImGui::Spacing();
        PushDangerButtonStyle();
        if (ImGui::Button("Delete Custom Hero", ImVec2(delete_button_width, 0.0f))) {
            ImGui::OpenPopup("Delete Custom Hero");
        }
        ImGui::PopStyleColor(3);

        if (ImGui::BeginPopupModal("Delete Custom Hero", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Select the custom hero to delete:");
            ImGui::SetNextItemWidth(240.0f);
            if (ImGui::BeginCombo("Custom Hero", g_custom_heroes[g_selected_custom_hero_deletion].name)) {
                for (int index = 0; index < g_custom_hero_count; ++index) {
                    bool selected = g_selected_custom_hero_deletion == index;
                    if (ImGui::Selectable(g_custom_heroes[index].name, selected)) {
                        g_selected_custom_hero_deletion = index;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Spacing();
            ImGui::Text("Hero: %s", g_custom_heroes[g_selected_custom_hero_deletion].name);
            ImGui::TextWrapped("All saved data associated with this hero will be permanently lost. Do you want to continue?");
            ImGui::Spacing();
            PushDangerButtonStyle();
            if (ImGui::Button("Confirm Delete", ImVec2(150.0f, 0.0f))) {
                if (delete_custom_hero(g_selected_custom_hero_deletion)) {
                    RefreshCustomHeroes();
                    g_selected_custom_hero_deletion = 0;
                    snprintf(g_status_message, sizeof(g_status_message), "Custom hero deleted successfully.");
                } else {
                    snprintf(g_status_message, sizeof(g_status_message), "Failed to delete custom hero.");
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(150.0f, 0.0f))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

    }

    ImGui::Spacing();
    ImGui::Separator();
    DrawStatusMessage();
}

static const ImGuiTableFlags kRatioTableFlags =
    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;

static void DrawRatioSuggestionTab() {
    ComputeRatios();
    ImGui::Text("Ratio analysis based on current player data.");
    ImGui::Spacing();

    if (ImGui::BeginTable("ratio_table", 5, kRatioTableFlags)) {
        ImGui::TableSetupColumn("Subject");
        ImGui::TableSetupColumn("Level");
        ImGui::TableSetupColumn("Target Ratio");
        ImGui::TableSetupColumn("Current ratio");
        ImGui::TableSetupColumn("Lvl difference");
        ImGui::PushStyleColor(ImGuiCol_Text, UiColors::Heading);
        ImGui::PushFont(GetUiBoldFont());
        ImGui::TableHeadersRow();
        ImGui::PopFont();
        ImGui::PopStyleColor();

        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Leader");
        ImGui::TableNextColumn(); ImGui::Text("%lld", g_player.leader_level);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputFloat("##leader_ratio", &g_player.recommended_ratios.leader, 0.001f, 0.01f, "%.4f");
        if (g_player.recommended_ratios.leader < 0.0f) g_player.recommended_ratios.leader = 0.0f;
        ImGui::TableNextColumn();
        float leader_current_ratio = (g_player.wave > 0) ? (float)((double)g_player.leader_level / (double)g_player.wave) : 0.0f;
        if (leader_current_ratio < g_player.recommended_ratios.leader) {
            ImGui::TextColored(UiColors::Danger, "%.4f", leader_current_ratio);
        } else {
            ImGui::TextColored(UiColors::Success, "%.4f", leader_current_ratio);
        }
        ImGui::TableNextColumn();
        long long leader_gap = (long long)((double)g_player.leader_level - (double)g_player.wave * (double)g_player.recommended_ratios.leader);
        if (leader_gap < 0) {
            ImGui::TextColored(UiColors::Danger, "-%lld", -leader_gap);
        } else {
            ImGui::TextColored(UiColors::Success, "%lld", leader_gap);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Infinity Castle");
        ImGui::TableNextColumn(); ImGui::Text("%lld", g_player.infinity_castle_level);
        ImGui::TableNextColumn(); ImGui::Text("As high as possible");
        ImGui::TableNextColumn();
        float colony_current_ratio = (g_player.wave > 0) ? (float)((double)g_player.infinity_castle_level / (double)g_player.wave) : 0.0f;
        ImGui::TextColored(UiColors::Info, "%.4f", colony_current_ratio);
        ImGui::TableNextColumn();
        ImGui::TextColored(UiColors::Muted, "-");

        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Town Archer");
        ImGui::TableNextColumn(); ImGui::Text("%lld", g_player.town_archer_level);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputFloat("##town_archer_ratio", &g_player.recommended_ratios.town_archer, 0.001f, 0.01f, "%.4f");
        if (g_player.recommended_ratios.town_archer < 0.0f) g_player.recommended_ratios.town_archer = 0.0f;
        ImGui::TableNextColumn();
        float town_archer_current_ratio = (g_player.wave > 0) ? (float)((double)g_player.town_archer_level / (double)g_player.wave) : 0.0f;
        if (town_archer_current_ratio < g_player.recommended_ratios.town_archer) {
            ImGui::TextColored(UiColors::Danger, "%.4f", town_archer_current_ratio);
        } else {
            ImGui::TextColored(UiColors::Success, "%.4f", town_archer_current_ratio);
        }
        ImGui::TableNextColumn();
        long long archer_gap = (long long)((double)g_player.town_archer_level - (double)g_player.wave * (double)g_player.recommended_ratios.town_archer);
        if (archer_gap < 0) {
            ImGui::TextColored(UiColors::Danger, "-%lld", -archer_gap);
        } else {
            ImGui::TextColored(UiColors::Success, "%lld", archer_gap);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Castle");
        ImGui::TableNextColumn(); ImGui::Text("%lld", g_player.castle_level);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputFloat("##castle_ratio", &g_player.recommended_ratios.castle, 0.001f, 0.01f, "%.4f");
        if (g_player.recommended_ratios.castle < 0.0f) g_player.recommended_ratios.castle = 0.0f;
        ImGui::TableNextColumn();
        float castle_current_ratio = (g_player.wave > 0) ? (float)((double)g_player.castle_level / (double)g_player.wave) : 0.0f;
        if (castle_current_ratio < g_player.recommended_ratios.castle) {
            ImGui::TextColored(UiColors::Danger, "%.4f", castle_current_ratio);
        } else {
            ImGui::TextColored(UiColors::Success, "%.4f", castle_current_ratio);
        }
        ImGui::TableNextColumn();
        long long castle_gap = (long long)((double)g_player.castle_level - (double)g_player.wave * (double)g_player.recommended_ratios.castle);
        if (castle_gap < 0) {
            ImGui::TextColored(UiColors::Danger, "-%lld", -castle_gap);
        } else {
            ImGui::TextColored(UiColors::Success, "%lld", castle_gap);
        }

        ImGui::EndTable();
    }

    PushPrimaryButtonStyle();
    if (ImGui::Button("Save ratio", ImVec2(220.0f, 0.0f))) {
        SaveRecommendedRatios();
    }
    ImGui::PopStyleColor(3);
    if (g_save_confirmation_until > ImGui::GetTime() && strcmp(g_save_confirmation_target, "recommended_ratios") == 0) {
        ImGui::SameLine();
        ImGui::TextColored(UiColors::Success, "New ratio saved");
    }

    DrawSectionBreak();
    DrawSectionHeading("Custom");
    if (g_custom_hero_count > 0) {
        if (ImGui::BeginTable("custom_ratio_table", 5, kRatioTableFlags)) {
            ImGui::TableSetupColumn("Subject");
            ImGui::TableSetupColumn("Level");
            ImGui::TableSetupColumn("Target Ratio");
            ImGui::TableSetupColumn("Current ratio");
            ImGui::TableSetupColumn("Lvl difference");
            ImGui::PushStyleColor(ImGuiCol_Text, UiColors::Heading);
            ImGui::PushFont(GetUiBoldFont());
            ImGui::TableHeadersRow();
            ImGui::PopFont();
            ImGui::PopStyleColor();

            for (int i = 0; i < g_custom_hero_count; ++i) {
                CustomHero& hero = g_custom_heroes[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("%s", hero.name);
                ImGui::TableNextColumn(); ImGui::Text("%lld", hero.level);
                ImGui::TableNextColumn();
                float current_ratio = (g_player.wave > 0) ? (float)((double)hero.level / (double)g_player.wave) : 0.0f;
                ImGui::SetNextItemWidth(-1.0f);
                ImGui::InputFloat(("Target##custom_ratio_" + std::to_string(i)).c_str(), &hero.target_ratio, 0.001f, 0.01f, "%.4f");
                if (hero.target_ratio < 0.0f) hero.target_ratio = 0.0f;
                ImGui::TableNextColumn();
                if (current_ratio < hero.target_ratio) {
                    ImGui::TextColored(UiColors::Danger, "%.4f", current_ratio);
                } else {
                    ImGui::TextColored(UiColors::Success, "%.4f", current_ratio);
                }
                ImGui::TableNextColumn();
                long long custom_gap = (long long)((double)hero.level - (double)g_player.wave * (double)hero.target_ratio);
                if (custom_gap < 0) {
                    ImGui::TextColored(UiColors::Danger, "-%lld", -custom_gap);
                } else {
                    ImGui::TextColored(UiColors::Success, "%lld", custom_gap);
                }
            }
            ImGui::EndTable();
        }
        if (ImGui::Button("Save custom ratio")) {
            SaveCustomHeroesFromGui();
        }
        if (g_save_confirmation_until > ImGui::GetTime() && strcmp(g_save_confirmation_target, "custom_heroes") == 0) {
            ImGui::SameLine();
            ImGui::TextColored(UiColors::Success, "Ratio has been updated");
        }
    } else {
        ImGui::TextDisabled("No custom heroes saved yet.");
    }

    DrawInvestmentAndCostSection();
}

static void DrawColonyStatsTab() {
    ComputeRatios();
    ImGui::Text("Colony stats and gold production based on Infinity Castle level.");
    ImGui::Separator();
    ImGui::Spacing();
    DrawSubsectionHeading("CORE METRICS");
    ImGui::Text("Infinity Castle Level: %lld", g_player.infinity_castle_level);
    ImGui::Text("Colony Ratio: %.4f", g_ratio_colony);
    ImGui::Spacing();
    DrawSubsectionHeading("GOLD PRODUCTION");
    ImGui::Text("Base Gold: %.0f", g_colony_gold);
    ImGui::Text("Gold with XP Skill Buff (x1.20): %.0f", g_gold_xp);
    ImGui::Text("Gold with Whip + Skill (x1.35): %.0f", g_gold_whip);
}

static void DrawProgressHistoryTab() {
    static int entry_limit_index = 2;
    const int entry_limits[] = {5, 10, 20, 50};
    RefreshProgressHistory();

    ImGui::Text("Progress history loaded from data/player_data.csv");
    ImGui::Text("Entries: %d", g_progress_count);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::Combo("Entries to show", &entry_limit_index, "5\0" "10\0" "20\0" "50\0\0");
    ImGui::Separator();

    if (g_progress_count > 0) {
        const int max_display = entry_limits[entry_limit_index];
        int display_count = g_progress_count;
        int start_index = 0;
        if (display_count > max_display) {
            start_index = g_progress_count - max_display;
            display_count = max_display;
        }

        double ratios[50];
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
        ImGui::TextColored(ImVec4(UiColors::Muted.x, UiColors::Muted.y, UiColors::Muted.z, 0.8f), "Ratio = Infinity Castle / Wave");
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
            snprintf(label, sizeof(label), "%lld", g_progress[start_index + idx].wave);
            ImVec2 text_size = ImGui::CalcTextSize(label);
            draw_list->AddText(ImVec2(x - text_size.x * 0.5f, graph_max.y + 8), IM_COL32(220, 220, 220, 200), label);
        }
        ImVec2 axis_label_size = ImGui::CalcTextSize("Wave");
        draw_list->AddText(ImVec2(graph_min.x + width * 0.5f - axis_label_size.x * 0.5f, graph_max.y + 22), IM_COL32(180, 180, 200, 200), "Wave");

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

        ImGui::Dummy(ImVec2(0, 24.0f));
        DrawSectionBreak();
        ImGui::Dummy(ImVec2(0, 12.0f));

        if (ImGui::BeginTable("history_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))  {
            ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthStretch, 1.35f);
            ImGui::TableSetupColumn("Wave", ImGuiTableColumnFlags_WidthStretch, 0.65f);
            ImGui::TableSetupColumn("Infinity Castle", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("Ratio", ImGuiTableColumnFlags_WidthStretch, 0.75f);
            ImGui::TableHeadersRow();

            for (int i = start_index; i < start_index + display_count; ++i) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("%s", g_progress[i].date);
                ImGui::TableNextColumn(); ImGui::Text("%lld", g_progress[i].wave);
                ImGui::TableNextColumn(); ImGui::Text("%lld", g_progress[i].infinity_castle_level);
                ImGui::TableNextColumn(); ImGui::Text("%.4f", g_progress[i].wave > 0 ? (double)g_progress[i].infinity_castle_level / g_progress[i].wave : 0.0);
            }
            ImGui::EndTable();
        }
    } else {
        ImGui::TextDisabled("No progress history available. Save player data to build up the history.");
    }
}

static void DrawProfitEstimateSection()
{
    HistoricalPaceStats historical_stats = CalculateHistoricalPaceStats(g_pace_history_period);
    DrawSectionHeading("PROFIT ESTIMATE");
    if (!historical_stats.isValid) {
        ImGui::TextColored(UiColors::Muted, "%s", historical_stats.message);
        return;
    }

    const double tab_profit = calculate_tab_profit(
        (double)g_pace_stats.rwph,
        (double)historical_stats.referenceWave,
        historical_stats.elapsedHours,
        historical_stats.downtimeHours);
    char tab_profit_text[64];
    FormatProfitValue(tab_profit, tab_profit_text, sizeof(tab_profit_text));
    ImGui::TextWrapped("Estimate of gold earned through TAB during the last tracked period, calculated from your real gameplay data - no additional input required.");
    ImGui::Text("Period: %.2f days | Reference wave: %lld", historical_stats.elapsedHours / 24.0, historical_stats.referenceWave);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, UiColors::PanelBg);
    ImGui::BeginChild("profit_estimate_panel", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
    ImGui::Text("TAB Profit: %s Gold", tab_profit_text);

    const double projected_tab_profit = calculate_tab_profit(
        (double)g_pace_stats.rwph,
        (double)g_player.wave,
        (double)g_projection_days * 24.0,
        0.0);
    char projected_tab_profit_text[64];
    FormatProfitValue(projected_tab_profit, projected_tab_profit_text, sizeof(projected_tab_profit_text));
    ImGui::Spacing();
    ImGui::Text("Next %d days (projected, no downtime):", g_projection_days);
    ImGui::Text("TAB Profit: %s Gold", projected_tab_profit_text);
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

static void DrawUpgradingCostTab() {
    ImGui::Text("Calculate gold cost for upgrades.");
    ImGui::Separator();

    ImGui::SetNextItemWidth(320.0f);
    ImGui::Combo("Upgrade Type", &g_upgrade_type, "Castle\0Town Archers\0Hero/Leader/Tower\0");
    ImGui::SetNextItemWidth(320.0f);
    ImGui::InputScalar("From level", ImGuiDataType_S64, &g_upgrade_from);
    ImGui::SetNextItemWidth(320.0f);
    ImGui::InputScalar("To level", ImGuiDataType_S64, &g_upgrade_to);

    PushPrimaryButtonStyle();
    if (ImGui::Button("Compute Cost")) {
        if (g_upgrade_from > 0 && g_upgrade_to > 0 && g_upgrade_to > g_upgrade_from) {
            UnitType unit_type = g_upgrade_type == 0 ? UNIT_TYPE_CASTLE
                : (g_upgrade_type == 1 ? UNIT_TYPE_TOWN_ARCHERS : UNIT_TYPE_LEADER);
            g_upgrade_cost = cost_function(unit_type, (double)g_upgrade_to)
                - cost_function(unit_type, (double)g_upgrade_from);
            g_upgrade_ready = true;
        } else {
            g_upgrade_ready = false;
            g_upgrade_cost = 0.0;
        }
    }
    ImGui::PopStyleColor(3);

    if (g_upgrade_ready) {
        char cost_text[64];
        FormatGoldValue(g_upgrade_cost, cost_text, sizeof(cost_text));
        DrawSectionBreak();
        DrawSectionHeading("CALCULATION RESULT");
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, UiColors::PanelBg);
        ImGui::BeginChild("upgrade_result_panel", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
        ImGui::TextColored(UiColors::Success, "Estimated Cost: %s Gold", cost_text);
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    } else {
        ImGui::Text("Enter valid levels and press Compute Cost.");
    }

    DrawSectionBreak();
    DrawProfitEstimateSection();
}

static void DrawPaceAnalysisTab() {
    calculatePaceStats(&g_pace_inputs, &g_pace_stats);

    ImGui::Text("Calculate waves per hour/season and compare them with your actual pace based on the data entered in Player Data.");
    ImGui::Spacing();

    DrawSectionHeading("PACE SETTINGS");
    bool changed = false;
    ImGui::SetNextItemWidth(260.0f);
    changed |= ImGui::Combo("Devil Horn", &g_pace_inputs.dhLevel, "0\0" "1\0" "2\0" "3\0" "4\0" "5\0\0");
    int game_speed_index = g_pace_inputs.gameSpeed - 2;
    ImGui::SetNextItemWidth(260.0f);
    if (ImGui::Combo("Game Speed", &game_speed_index, "2x\0" "3x\0\0")) {
        g_pace_inputs.gameSpeed = game_speed_index + 2;
        changed = true;
    }
    int chrono_index = (int)g_pace_inputs.chrono;
    ImGui::SetNextItemWidth(260.0f);
    if (ImGui::Combo("Chrono", &chrono_index, "None\0Passive\0Yellow\0Blue\0")) {
        g_pace_inputs.chrono = (PaceChrono)chrono_index;
        changed = true;
    }

    bool golden_horn = g_pace_inputs.goldenHorn != 0;
    bool horn = g_pace_inputs.horn != 0;
    if (ImGui::Checkbox("Golden Horn", &golden_horn)) {
        g_pace_inputs.goldenHorn = golden_horn ? 1 : 0;
        changed = true;
    }
    if (ImGui::Checkbox("Horn", &horn)) {
        g_pace_inputs.horn = horn ? 1 : 0;
        changed = true;
    }

    ImGui::Spacing();
    DrawSectionHeading("TAB HEROES");
    bool ob = g_pace_inputs.ob != 0;
    bool mbf = g_pace_inputs.mbf != 0;
    if (ImGui::Checkbox("OB", &ob)) {
        g_pace_inputs.ob = ob ? 1 : 0;
        changed = true;
    }
    if (ImGui::Checkbox("MBF", &mbf)) {
        g_pace_inputs.mbf = mbf ? 1 : 0;
        changed = true;
    }

    if (changed) {
        SavePaceDataFromGui();
    }
    DrawSaveConfirmation("pace_data");

    DrawSectionBreak();

    if (!g_pace_stats.isValid) {
        ImGui::TextColored(UiColors::Danger, "%s", g_pace_stats.validationMessage[0] ? g_pace_stats.validationMessage : "Invalid pace input.");
    } else {
        DrawSubsectionHeading("PACE RESULTS");
        ImGui::TextColored(UiColors::Success, "RWPH: %d", g_pace_stats.rwph);
        ImGui::TextColored(UiColors::Success, "WPH: %.2f", g_pace_stats.wph);
        ImGui::Spacing();
        DrawSubsectionHeading("TOTAL WAVES");
        ImGui::Text("Waves / Day: %.2f", g_pace_stats.wavesPerDay);
        ImGui::Text("Waves / Season (5 days): %.2f", g_pace_stats.wavesPerSeason);

        DrawSectionBreak();
        DrawSubsectionHeading("ACTUAL PACE & DOWNTIME");
        ImGui::TextWrapped("Actual Pace uses the first and last Player Data records in the selected period. Estimated downtime is the elapsed time minus the time required to complete the same waves at the expected pace.");
        ImGui::SetNextItemWidth(260.0f);
        ImGui::Combo("History Period", &g_pace_history_period, "All Time\0Last Month\0Last 5 Days\0Last 24 Hours\0\0");

        HistoricalPaceStats historical_stats = CalculateHistoricalPaceStats(g_pace_history_period);
        if (!historical_stats.isValid) {
            ImGui::TextColored(UiColors::Muted, "%s", historical_stats.message);
        } else {
            ImGui::Text("Records used: %d", historical_stats.entryCount);
            ImGui::Spacing();
            ImGui::TextColored(UiColors::Info, "Actual Pace");
            ImGui::Text("Waves / Hour: %.2f", historical_stats.wavesPerHour);
            ImGui::Text("Waves / Day: %.2f", historical_stats.wavesPerDay);
            ImGui::Text("Waves / Season (5 days): %.2f", historical_stats.wavesPerSeason);
            ImGui::Spacing();
            ImGui::TextColored(UiColors::Success, "Expected Pace");
            ImGui::Text("Waves / Hour: %.2f", g_pace_stats.wph);
            ImGui::Text("Waves / Day: %.2f", g_pace_stats.wavesPerDay);
            ImGui::Text("Waves / Season (5 days): %.2f", g_pace_stats.wavesPerSeason);
            ImGui::Spacing();
            ImGui::TextColored(UiColors::Warning, "Estimated Downtime: %.2f hours", historical_stats.downtimeHours);
            ImGui::Text("Downtime %% of selected elapsed time: %.2f %%", historical_stats.downtimePercentage);
            if (historical_stats.usesDateOnlyEntries) {
                ImGui::TextColored(UiColors::Warning, "Some older records have no time of day and are treated as midnight.");
            }

        }
    }
}

static void DrawInfoTab() {
    ImGui::Text("Grow Castle Progress Tracker v4.0.0");
    ImGui::Text("Built and maintained by miglioDev");

    ImGui::TextLinkOpenURL(
        "github.com/miglioDev/grow-castle-progress-tracker",
        "https://github.com/miglioDev/grow-castle-progress-tracker"
    );

    ImGui::Spacing();

    ImGui::TextWrapped(
        "For further information on the project, how it works and formulas used please read the DOCUMENTATION.md file on the website\n"
        "Have feedback, suggestions, or found a bug?\n"
        "Feel free to reach out! You can find me on the official "
        "Grow Castle Discord server under @miglioDev I’ll get back to you when I have the chance."
    );

    ImGui::Separator();
    ImGui::Spacing();

    DrawSubsectionHeading("IMPORT / EXPORT DATA");
    ImGui::Spacing();

    ImGui::TextWrapped(
        "All data is stored locally in the ./data folder. "
        "You can import or export your progress by copying the CSV files "
        "to another location."
    );

    ImGui::Spacing();

    DrawSubsectionHeading("STORED FILES");

    ImGui::TextColored(
        UiColors::Success,
        "./data/player_data.csv"
    );
    ImGui::Text("Player progress and history");

    ImGui::TextColored(
        UiColors::Success,
        "./data/custom_heroes.csv"
    );
    ImGui::Text("Custom hero names, ratios, and levels");

    ImGui::TextColored(
        UiColors::Success,
        "./data/pace_data.csv"
    );
    ImGui::Text("Saved RWPH/WPH calculator options");

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "These files can also be shared with friends. "
        "For example, sharing your data files allows you to compare "
        "specific builds, player progress, hero levels, ratios, and "
        "other saved information."
    );

    ImGui::Spacing();

    ImGui::TextWrapped(
        "Backup: copy all CSV files to a safe location. "
        "Restore: copy them back into the ./data folder with the same "
        "names, then restart the app."
    );

    ImGui::Spacing();

    ImGui::TextWrapped(
        "If a file is missing, that category has no saved data yet."
    );
}

void ShowApplication() {
    if (!g_data_loaded) {
        RefreshPlayerData();
        RefreshProgressHistory();
        RefreshCustomHeroes();
        RefreshPaceData();
        ComputeRatios();
    }

    ImGui::Begin("Grow Castle Progress Tracker");
    ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize("Built by miglioDev").x - 20.0f);
    ImGui::TextColored(ImVec4(0.75f, 0.82f, 0.9f, 0.8f), "Built by miglioDev");
    if (ImGui::BeginTabBar("MainTabs")) {
        if (BeginBoldTabItem("Player Data")) {
            DrawPlayerDataTab();
            ImGui::EndTabItem();
        }
        if (BeginBoldTabItem("Ratio, Levels & Economy")) {
            DrawRatioSuggestionTab();
            ImGui::EndTabItem();
        }
        if (BeginBoldTabItem("Pace & Season Analysis")) {
            DrawPaceAnalysisTab();
            ImGui::EndTabItem();
        }
        if (BeginBoldTabItem("IC Stats & History")) {
            DrawColonyStatsTab();
            DrawSectionBreak();
            DrawProgressHistoryTab();
            ImGui::EndTabItem();
        }
        if (BeginBoldTabItem("Upgrading Cost & Profit")) {
            DrawUpgradingCostTab();
            ImGui::EndTabItem();
        }
        if (BeginBoldTabItem("Info:")) {
            DrawInfoTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}
