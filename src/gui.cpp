#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <cmath>
#include <cctype>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "imgui.h"
#include "file_operations.h"
#include "player_data.h"
#include "player_stats.h"
#include "upgrading.h"
#include "investment.h"
#include "graph.h"
#include "pace_analysis.h"
#include "gpw.h"

static Player g_player = {0};
static ProgressData g_progress[300] = {0};
static int g_progress_count = 0;
static int g_progress_invalid_row_count = 0;
static bool g_data_loaded = false;
static char g_status_message[256] = "";

static int g_upgrade_type = 0;
static long long g_upgrade_from = 1;
static long long g_upgrade_to = 2;
static double g_upgrade_cost = 0.0;
static bool g_upgrade_ready = false;
static long long g_gpw_samples[16] = {0};
static int g_gpw_sample_count = 1;
static const double kGpwCostPerWave = 456.0;
static GpwAnalysis g_gpw_analysis = {};
static GpwCalculationError g_gpw_error = GPW_CALCULATION_NO_SAMPLES;
static bool g_gpw_ready = false;

static float g_ratio_leader = 0.0f;
static float g_ratio_colony = 0.0f;
static float g_ratio_town_archer = 0.0f;
static float g_ratio_castle = 0.0f;
static double g_colony_gold = 0.0;
static double g_gold_xp = 0.0;
static double g_gold_whip = 0.0;
static int g_projection_days = 5;
static int g_investment_pace_source = 1;
static int g_gold_power_pace_source = 1;
static double g_gold_power_season_income = 1.0;
static bool g_gold_power_has_saved_gold = false;
static double g_gold_power_saved_gold = 0.0;
static bool g_gold_power_has_target_gold = false;
static double g_gold_power_target_gold = 0.0;
static char g_custom_hero_name[64] = "";
static float g_custom_hero_target_ratio = 0.04f;
static long long g_custom_hero_level = 1;
static CustomHero g_custom_heroes[32] = {0};
static int g_custom_hero_count = 0;
static const int MAX_CUSTOM_HEROES = 32;
// InputScalar so 64-bit level/wave fields keep +/- buttons.
static const long long kInt64Step = 1;
static const long long kInt64StepFast = 100;
static PaceInputs g_pace_inputs = {0};
static PaceStats g_pace_stats = {0};
static int g_pace_history_period = 2;
static double g_save_confirmation_until = 0.0;
static char g_save_confirmation_target[64] = "";
static Player g_pending_player_deletion = {0};
static int g_selected_custom_hero_deletion = 0;
static Player g_player_saved_snapshot = {0};
static CustomHero g_custom_heroes_saved_snapshot[32] = {0};
static int g_custom_heroes_saved_snapshot_count = 0;

// My cool palette of colors for all the tabs
namespace UiColors {
    static const ImVec4 Success = ImVec4(0.30f, 0.85f, 0.45f, 1.0f);
    static const ImVec4 Danger = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
    static const ImVec4 Warning = ImVec4(1.0f, 0.75f, 0.35f, 1.0f);
    static const ImVec4 Info = ImVec4(0.40f, 0.80f, 1.0f, 1.0f);
    static const ImVec4 Muted = ImVec4(0.62f, 0.66f, 0.72f, 1.0f);
    static const ImVec4 Heading = ImVec4(0.46f, 0.78f, 1.0f, 1.0f);
    static const ImVec4 Subheading = ImVec4(0.62f, 0.78f, 0.92f, 1.0f);
    static const ImVec4 Accent = ImVec4(1.0f, 0.84f, 0.30f, 1.0f);
    static const ImVec4 InfoAuthor = ImVec4(1.0f, 0.68f, 0.34f, 1.0f);
    static const ImVec4 InfoContributor = ImVec4(0.18f, 0.68f, 1.0f, 1.0f);
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

// help marker
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

static void DrawSectionHeading(const char* text, const char* help_text = nullptr)
{
    ImGui::PushStyleColor(ImGuiCol_Text, UiColors::Heading);
    ImGui::PushFont(GetUiHeaderFont());
    ImGui::TextUnformatted(text);
    ImGui::PopFont();
    ImGui::PopStyleColor();
    if (help_text) {
        DrawHelpMarker(help_text);
    }
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(UiColors::Heading.x, UiColors::Heading.y, UiColors::Heading.z, 0.45f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

static void BeginPanel(const char* id, const ImVec2& size = ImVec2(0.0f, 0.0f))
{
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.39f, 0.56f, 0.60f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, UiColors::PanelBg);
    ImGui::BeginChild(id, size, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
}

static void EndPanel()
{
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

static void BeginHoverRow(const char* row_id, float row_height)
{
    ImGui::TableNextRow(ImGuiTableRowFlags_None, row_height);
    ImGui::TableSetColumnIndex(0);
    const ImVec2 row_cursor_pos = ImGui::GetCursorScreenPos();
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1.0f, 1.0f, 1.0f, 0.06f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.0f, 1.0f, 1.0f, 0.06f));
    ImGui::Selectable(row_id, false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, ImVec2(0.0f, row_height));
    ImGui::PopStyleColor(2);
    ImGui::SetCursorScreenPos(row_cursor_pos);
}

static void DrawHeroValue(const ImVec4& color, const char* text)
{
    ImGui::PushFont(GetUiHeroFont());
    ImGui::TextColored(color, "%s", text);
    ImGui::PopFont();
}

static void DrawSubsectionHeading(const char* text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, UiColors::Subheading);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

static void DrawHeroMetric(const char* label, const ImVec4& color, const char* value)
{
    const float line_y = ImGui::GetCursorPosY();
    const float label_height = ImGui::GetTextLineHeight();
    ImGui::PushFont(GetUiHeroFont());
    const float value_height = ImGui::GetTextLineHeight();
    ImGui::PopFont();

    ImGui::SetCursorPosY(line_y + (value_height - label_height) * 0.5f);
    ImGui::TextColored(UiColors::Muted, "%s", label);
    ImGui::SameLine(0.0f, 12.0f);
    ImGui::SetCursorPosY(line_y);
    DrawHeroValue(color, value);
}

// Standard spacer for major sections
static void DrawSectionBreak()
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

static void DrawStatusMessage()
{
    if (g_status_message[0] == '\0') {
        return;
    }
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, UiColors::PanelBg);
    const float fixed_height = ImGui::GetTextLineHeightWithSpacing() * 3.0f + ImGui::GetStyle().FramePadding.y * 2.0f;
    ImGui::BeginChild("status_message", ImVec2(0.0f, fixed_height), ImGuiChildFlags_Borders);
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

static void SnapshotPlayerSavedState() {
    g_player_saved_snapshot = g_player;
}

static void SnapshotCustomHeroesSavedState() {
    memcpy(g_custom_heroes_saved_snapshot, g_custom_heroes, sizeof(g_custom_heroes));
    g_custom_heroes_saved_snapshot_count = g_custom_hero_count;
}

static bool HasUnsavedCustomHeroChanges() {
    if (g_custom_hero_count != g_custom_heroes_saved_snapshot_count) {
        return true;
    }
    for (int i = 0; i < g_custom_hero_count; ++i) {
        if (g_custom_heroes[i].level != g_custom_heroes_saved_snapshot[i].level
            || g_custom_heroes[i].target_ratio != g_custom_heroes_saved_snapshot[i].target_ratio) {
            return true;
        }
    }
    return false;
}

static bool HasUnsavedPlayerDataChanges() {
    return g_player.wave != g_player_saved_snapshot.wave
        || g_player.infinity_castle_level != g_player_saved_snapshot.infinity_castle_level
        || g_player.leader_level != g_player_saved_snapshot.leader_level
        || g_player.town_archer_level != g_player_saved_snapshot.town_archer_level
        || g_player.castle_level != g_player_saved_snapshot.castle_level;
}

static bool HasUnsavedRatioChanges() {
    return g_player.recommended_ratios.leader != g_player_saved_snapshot.recommended_ratios.leader
        || g_player.recommended_ratios.town_archer != g_player_saved_snapshot.recommended_ratios.town_archer
        || g_player.recommended_ratios.castle != g_player_saved_snapshot.recommended_ratios.castle
        || HasUnsavedCustomHeroChanges();
}

// dot for unsaved data update 
static void DrawUnsavedChangesDot() {
    ImGui::SameLine();
    ImGui::TextColored(UiColors::Warning, "\xE2\x97\x8F");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
        ImGui::SetTooltip("Unsaved changes");
    }
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
    SnapshotPlayerSavedState();
}

static void RefreshProgressHistory() {
    g_progress_count = read_progress_history("data/player_data.csv", g_progress, 300);
    if (g_progress_count <= 0) {
        g_progress_count = 0;
    }
    g_progress_invalid_row_count = get_progress_history_invalid_row_count();
}

static void RefreshCustomHeroes() {
    g_custom_hero_count = load_custom_heroes(g_custom_heroes, 32);
    SnapshotCustomHeroesSavedState();
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

    if (!std::isfinite(g_custom_hero_target_ratio) || g_custom_hero_target_ratio <= 0.0f || g_custom_hero_target_ratio > 10.0f
        || g_custom_hero_level < 1) {
        snprintf(g_status_message, sizeof(g_status_message), "Custom hero ratio must be finite and between 0 and 10; level must be positive.");
        return;
    }

    if (strchr(g_custom_hero_name, ',') != NULL) {
        snprintf(g_status_message, sizeof(g_status_message), "Hero name cannot contain a comma.");
        return;
    }

    const char* trim_start = g_custom_hero_name;
    while (*trim_start != '\0' && std::isspace((unsigned char)*trim_start)) {
        trim_start++;
    }
    const char* trim_end = trim_start + strlen(trim_start);
    while (trim_end > trim_start && std::isspace((unsigned char)*(trim_end - 1))) {
        trim_end--;
    }
    const size_t trimmed_len = (size_t)(trim_end - trim_start);
    if (trimmed_len == 0) {
        snprintf(g_status_message, sizeof(g_status_message), "Please enter a non-empty hero name.");
        return;
    }
    memmove(g_custom_hero_name, trim_start, trimmed_len);
    g_custom_hero_name[trimmed_len] = '\0';

    for (int i = 0; i < g_custom_hero_count; i++) {
        const char* existing_name = g_custom_heroes[i].name;
        if (strlen(existing_name) != trimmed_len) {
            continue;
        }
        bool same_name = true;
        for (size_t j = 0; j < trimmed_len; j++) {
            if (std::tolower((unsigned char)existing_name[j]) != std::tolower((unsigned char)trim_start[j])) {
                same_name = false;
                break;
            }
        }
        if (same_name) {
            snprintf(g_status_message, sizeof(g_status_message), "A custom hero named '%s' already exists.", existing_name);
            return;
        }
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
    int date_length = 0;
    if (!text || !timestamp || !has_time
        || sscanf(text, "%d-%d-%d%n", &year, &month, &day, &date_length) != 3
        || (text[date_length] != '\0' && text[date_length] != ' ')) {
        return false;
    }

    struct tm parsed_time = {};
    parsed_time.tm_year = year - 1900;
    parsed_time.tm_mon = month - 1;
    parsed_time.tm_mday = day;
    parsed_time.tm_hour = 12;
    parsed_time.tm_isdst = -1;

    *timestamp = mktime(&parsed_time);
    if (*timestamp == (time_t)-1 || parsed_time.tm_year != year - 1900
        || parsed_time.tm_mon != month - 1 || parsed_time.tm_mday != day) {
        return false;
    }

    *has_time = false;
    if (text[date_length] == '\0') {
        return true;
    }
    char trailing = '\0';
    if (sscanf(text + date_length + 1, "%d:%d:%d%c", &hour, &minute, &second, &trailing) != 3
        || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        return false;
    }
    parsed_time.tm_hour = hour;
    parsed_time.tm_min = minute;
    parsed_time.tm_sec = second;
    *timestamp = mktime(&parsed_time);
    if (*timestamp == (time_t)-1 || parsed_time.tm_year != year - 1900
        || parsed_time.tm_mon != month - 1 || parsed_time.tm_mday != day
        || parsed_time.tm_hour != hour || parsed_time.tm_min != minute || parsed_time.tm_sec != second) {
        return false;
    }
    *has_time = true;
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
    if (!std::isfinite(g_player.recommended_ratios.leader) || g_player.recommended_ratios.leader <= 0.0f || g_player.recommended_ratios.leader > 10.0f
        || !std::isfinite(g_player.recommended_ratios.town_archer) || g_player.recommended_ratios.town_archer <= 0.0f || g_player.recommended_ratios.town_archer > 10.0f
        || !std::isfinite(g_player.recommended_ratios.castle) || g_player.recommended_ratios.castle <= 0.0f || g_player.recommended_ratios.castle > 10.0f) {
        snprintf(g_status_message, sizeof(g_status_message), "Ratios must be finite, greater than 0 and no more than 10.");
        return false;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (!t || strftime(g_player.last_update, sizeof(g_player.last_update), "%Y-%m-%d %H:%M:%S", t) == 0) {
        snprintf(g_status_message, sizeof(g_status_message), "Failed to generate the current timestamp.");
        return false;
    }
    if (save_player_data(&g_player)) {
        snprintf(g_status_message, sizeof(g_status_message), "Player data saved successfully. Last update: %s", g_player.last_update);
        RefreshProgressHistory();
        g_data_loaded = true;
        SnapshotPlayerSavedState();
        return true;
    } else {
        snprintf(g_status_message, sizeof(g_status_message), "Failed to save player data.");
        return false;
    }
}

static void SaveRecommendedRatios() {
    if (!std::isfinite(g_player.recommended_ratios.leader) || g_player.recommended_ratios.leader <= 0.0f || g_player.recommended_ratios.leader > 10.0f ||
        !std::isfinite(g_player.recommended_ratios.town_archer) || g_player.recommended_ratios.town_archer <= 0.0f || g_player.recommended_ratios.town_archer > 10.0f ||
        !std::isfinite(g_player.recommended_ratios.castle) || g_player.recommended_ratios.castle <= 0.0f || g_player.recommended_ratios.castle > 10.0f) {
        snprintf(g_status_message, sizeof(g_status_message), "Ratios must be finite, greater than 0 and no more than 10.");
        return;
    }

    if (g_player.wave <= 0 || g_player.infinity_castle_level <= 0 || g_player.leader_level <= 0 ||
        g_player.town_archer_level <= 0 || g_player.castle_level <= 0) {
        snprintf(g_status_message, sizeof(g_status_message), "Enter and save valid player data before saving ratios.");
        return;
    }

    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);
    if (!current_time || strftime(g_player.last_update, sizeof(g_player.last_update), "%Y-%m-%d %H:%M:%S", current_time) == 0) {
        snprintf(g_status_message, sizeof(g_status_message), "Failed to generate the current timestamp.");
        return;
    }
    if (replace_last_player_data(&g_player)) {
        RefreshProgressHistory();
        MarkDataSaved("recommended_ratios");
        SnapshotPlayerSavedState();
        snprintf(g_status_message, sizeof(g_status_message), "Recommended ratios saved successfully.");
    } else {
        snprintf(g_status_message, sizeof(g_status_message), "Failed to save recommended ratios. Existing player data was preserved.");
    }
}

static void SaveCustomHeroesFromGui() {
    for (int i = 0; i < g_custom_hero_count; ++i) {
        if (!std::isfinite(g_custom_heroes[i].target_ratio) || g_custom_heroes[i].target_ratio <= 0.0f
            || g_custom_heroes[i].target_ratio > 10.0f || g_custom_heroes[i].level < 1) {
            snprintf(g_status_message, sizeof(g_status_message), "Custom hero ratios must be greater than 0 and no more than 10; levels must be positive.");
            return;
        }
    }

    if (!save_custom_heroes(g_custom_heroes, g_custom_hero_count)) {
        snprintf(g_status_message, sizeof(g_status_message), "Failed to save custom heroes.");
        return;
    }

    SnapshotCustomHeroesSavedState();
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

static void FormatLevelValue(long long value, char* out, size_t out_size) {
    const std::string digits = std::to_string(value);
    const size_t first_digit = digits[0] == '-' ? 1 : 0;
    std::string formatted;
    formatted.reserve(digits.length() + (digits.length() - first_digit) / 3);
    if (first_digit != 0) {
        formatted.push_back('-');
    }
    for (size_t index = first_digit; index < digits.length(); ++index) {
        if (index > first_digit && (digits.length() - index) % 3 == 0) {
            formatted.push_back(',');
        }
        formatted.push_back(digits[index]);
    }
    snprintf(out, out_size, "%s", formatted.c_str());
}

static void FormatGpwResultValue(double amount, char* out, size_t out_size) {
    if (amount >= 1000000.0) {
        FormatGoldValue(amount, out, out_size);
    } else {
        snprintf(out, out_size, "%.2f", amount);
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

static void FormatCompactGoldValue(double amount, char* out, size_t out_size)
{
    const double absolute_amount = amount < 0.0 ? -amount : amount;
    const char* sign = amount < 0.0 ? "-" : "";
    if (absolute_amount >= 1000000000000.0) {
        snprintf(out, out_size, "%s%.2fT", sign, absolute_amount / 1000000000000.0);
    } else if (absolute_amount >= 1000000000.0) {
        snprintf(out, out_size, "%s%.2fB", sign, absolute_amount / 1000000000.0);
    } else if (absolute_amount >= 1000000.0) {
        snprintf(out, out_size, "%s%.2fM", sign, absolute_amount / 1000000.0);
    } else if (absolute_amount >= 1000.0) {
        snprintf(out, out_size, "%s%.2fK", sign, absolute_amount / 1000.0);
    } else {
        snprintf(out, out_size, "%.0f", amount);
    }
}

static const char* GetGoldPowerStatusMessage(GoldPowerStatus status)
{
    switch (status) {
    case GOLD_POWER_INVALID_WAVE: return "Wave must be a finite value greater than zero.";
    case GOLD_POWER_INVALID_PACE: return "The selected WPH source must be a finite value greater than zero.";
    case GOLD_POWER_INVALID_TOTAL_GOLD: return "Total invested gold must be a finite value greater than zero.";
    case GOLD_POWER_INVALID_SEASON_INCOME: return "Gold income per season must be a finite value greater than zero.";
    case GOLD_POWER_INVALID_SAVED_GOLD: return "Saved gold equivalent must be a finite value greater than or equal to zero.";
    case GOLD_POWER_INVALID_TARGET_GOLD: return "Desired gold target must be a finite value greater than or equal to zero.";
    case GOLD_POWER_CALCULATION_OVERFLOW: return "Gold Power values are too large to calculate safely.";
    default: return "Unable to calculate Gold Power.";
    }
}

static void DrawGoldPowerSection(double total_investment, const HistoricalPaceStats& historical_stats)
{
    const bool historical_pace_available = historical_stats.isValid != 0;
    const bool pace_results_available = g_pace_stats.isValid != 0;
    const bool selected_pace_available = g_gold_power_pace_source == 0
        ? historical_pace_available
        : pace_results_available;
    const double selected_wph = g_gold_power_pace_source == 0
        ? historical_stats.wavesPerHour
        : g_pace_stats.wph;
    const double selected_season_pace = selected_wph * 120.0;
    char historical_pace_label[64];
    char pace_results_label[64];
    snprintf(historical_pace_label, sizeof(historical_pace_label), "Historical pace  %.2f WPH", historical_stats.wavesPerHour);
    snprintf(pace_results_label, sizeof(pace_results_label), "Pace results  %.2f WPH", g_pace_stats.wph);

    ImGui::Spacing();
    BeginPanel("gold_power_panel");
    DrawSectionHeading("GOLD POWER", "GP measures how much gold you've invested in your build relative to your wave. It's a community-created metric.\nEntering your total seasonal gold income will show you additional insights.");
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("WPH source");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(300.0f);
    const char* selected_pace_label = g_gold_power_pace_source == 0 ? historical_pace_label : pace_results_label;
    if (ImGui::BeginCombo("##gold_power_wph_source", selected_pace_label)) {
        ImGui::BeginDisabled(!historical_pace_available);
        if (ImGui::Selectable(historical_pace_label, g_gold_power_pace_source == 0)) {
            g_gold_power_pace_source = 0;
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!pace_results_available);
        if (ImGui::Selectable(pace_results_label, g_gold_power_pace_source == 1)) {
            g_gold_power_pace_source = 1;
        }
        ImGui::EndDisabled();
        ImGui::EndCombo();
    }
    ImGui::SetNextItemWidth(360.0f);
    ImGui::InputDouble("Gold income per season", &g_gold_power_season_income, 0.0, 0.0, "%.0f");
    ImGui::Checkbox("Include saved gold equivalent", &g_gold_power_has_saved_gold);
    if (g_gold_power_has_saved_gold) {
        ImGui::SetNextItemWidth(360.0f);
        ImGui::InputDouble("Saved gold equivalent", &g_gold_power_saved_gold, 0.0, 0.0, "%.0f");
    }
    ImGui::Checkbox("Include desired gold target", &g_gold_power_has_target_gold);
    if (g_gold_power_has_target_gold) {
        ImGui::SetNextItemWidth(360.0f);
        ImGui::InputDouble("Desired gold target", &g_gold_power_target_gold, 0.0, 0.0, "%.0f");
    }

    GoldPowerInput input = {(double)g_player.wave, selected_season_pace, total_investment, g_gold_power_season_income,
        g_gold_power_has_saved_gold ? 1 : 0, g_gold_power_saved_gold,
        g_gold_power_has_target_gold ? 1 : 0, g_gold_power_target_gold};
    GoldPowerMetrics metrics = {};
    GoldPowerStatus status = selected_pace_available
        ? calculate_gold_power_metrics(&input, &metrics)
        : GOLD_POWER_INVALID_PACE;
    if (status != GOLD_POWER_OK) {
        ImGui::TextColored(UiColors::Warning, "%s", GetGoldPowerStatusMessage(status));
    }

    ImGui::Spacing();
    ImGui::BeginDisabled(status != GOLD_POWER_OK);
    if (ImGui::BeginTable("gold_power_metrics", 5,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("GP");
        ImGui::TableSetupColumn("GP + Saved");
        ImGui::TableSetupColumn("GP Loss/Season");
        ImGui::TableSetupColumn("GP Gain");
        ImGui::TableSetupColumn("GP Desired");
        ImGui::TableHeadersRow();
        BeginHoverRow("##gp_row_current", ImGui::GetTextLineHeight());
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
            ImGui::GetColorU32(ImVec4(UiColors::Accent.x, UiColors::Accent.y, UiColors::Accent.z, 0.18f)));
        ImGui::PushFont(GetUiBoldFont());
        ImGui::TextColored(UiColors::Accent, "%.2f", metrics.current_power);
        ImGui::PopFont();
        ImGui::TableNextColumn();
        if (metrics.has_power_with_saved_gold) ImGui::Text("%.2f", metrics.power_with_saved_gold);
        else ImGui::TextDisabled("-");
        ImGui::TableNextColumn();
        ImGui::Text("%.2f", metrics.power_loss_per_season);
        ImGui::TableNextColumn(); ImGui::Text("%.2f", metrics.power_gain);
        ImGui::TableNextColumn();
        if (metrics.has_target_power) {
            ImGui::Text("%.2f", metrics.target_power);
        } else ImGui::TextDisabled("-");

        BeginHoverRow("##gp_row_gap", ImGui::GetTextLineHeight());
        ImGui::TextDisabled("-");
        char gold_gap_text[64];
        ImGui::TableNextColumn();
        if (metrics.has_saved_gold_gap) {
            FormatCompactGoldValue(metrics.saved_gold_gap, gold_gap_text, sizeof(gold_gap_text));
            ImGui::Text("%s Gold", gold_gap_text);
        } else ImGui::TextDisabled("-");
        ImGui::TableNextColumn();
        ImGui::Text("%.2f%%", metrics.power_loss_percent * 100.0);
        ImGui::TableNextColumn(); ImGui::Text("%.2f%%", metrics.power_gain_percent * 100.0);
        ImGui::TableNextColumn();
        if (metrics.has_target_gold_gap) {
            FormatCompactGoldValue(metrics.target_gold_gap, gold_gap_text, sizeof(gold_gap_text));
            if (metrics.target_gold_gap < 0.0) {
                ImGui::TextColored(UiColors::Success, "Target already exceeded");
            } else {
                ImGui::Text("%s Gold", gold_gap_text);
            }
        } else ImGui::TextDisabled("-");
        ImGui::EndTable();
    }
    ImGui::EndDisabled();
    EndPanel();
}

static void DrawInvestmentAndCostSection() {
    InvestmentRow rows[36] = {};
    int row_count = 0;
    HistoricalPaceStats historical_stats = CalculateHistoricalPaceStats(0);
    const bool historical_pace_available = historical_stats.isValid != 0;
    const bool pace_results_available = g_pace_stats.isValid != 0;
    const bool pace_available = g_investment_pace_source == 0
        ? historical_pace_available
        : pace_results_available;
    const double pace_wph = !pace_available ? 0.0
        : (g_investment_pace_source == 0 ? historical_stats.wavesPerHour : g_pace_stats.wph);
    DrawSectionBreak();
    BeginPanel("investment_cost_panel");
    DrawSectionHeading("INVESTMENT & COST", "Investment (Gold): total gold already spent on this unit.\nInvestment %: this unit's share of total gold spent.\nCost to Target (now): gold needed to reach your target ratio right now.\nCost to Target (next period): gold needed to stay on target after the projection window, based on the selected WPH source.");
    ImGui::Spacing();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("WPH source");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(300.0f);
    char historical_pace_label[64];
    char pace_results_label[64];
    snprintf(historical_pace_label, sizeof(historical_pace_label), "Historical pace  %.2f WPH", historical_stats.wavesPerHour);
    snprintf(pace_results_label, sizeof(pace_results_label), "Pace results  %.2f WPH", g_pace_stats.wph);
    const char* selected_pace_label = g_investment_pace_source == 0 ? historical_pace_label : pace_results_label;
    if (ImGui::BeginCombo("##investment_wph_source", selected_pace_label)) {
        ImGui::BeginDisabled(!historical_pace_available);
        if (ImGui::Selectable(historical_pace_label, g_investment_pace_source == 0)) {
            g_investment_pace_source = 0;
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!pace_results_available);
        if (ImGui::Selectable(pace_results_label, g_investment_pace_source == 1)) {
            g_investment_pace_source = 1;
        }
        ImGui::EndDisabled();
        ImGui::EndCombo();
    }
    DrawHelpMarker("Historical pace uses the actual WPH derived from saved Player Data history. Pace results uses the WPH configured in the Pace & Season Analysis tab. The selected value is used only for Cost to Target (next period).");
    ImGui::SetNextItemWidth(220.0f);
    ImGui::InputInt("Projection Days", &g_projection_days, 1, 1);
    if (g_projection_days < 1) {
        g_projection_days = 1;
    }
    const double projection_hours = g_projection_days * 24.0;

    rows[row_count++] = {"Leader", calculate_investment_metrics(UNIT_TYPE_LEADER, g_player.leader_level,
        g_player.recommended_ratios.leader, g_player.wave, pace_wph, projection_hours)};
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
            "The selected WPH source is unavailable. Configure Pace Results or save enough Player Data history.");
    } else {
        ImGui::Text("Selected pace: %.2f WPH", pace_wph);
    }

    ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.10f, 0.145f, 0.21f, 0.62f));
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
            BeginHoverRow(("##investment_row_" + std::to_string(index)).c_str(), ImGui::GetTextLineHeight());
            ImGui::Text("%s", rows[index].name);
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
    ImGui::PopStyleColor();
    EndPanel();

    ImGui::Spacing();
    BeginPanel("gold_distribution_panel");
    DrawSectionHeading("GOLD DISTRIBUTION");
    const ImU32 colors[] = {IM_COL32(74, 167, 220, 255), IM_COL32(228, 151, 66, 255), IM_COL32(112, 190, 116, 255), IM_COL32(214, 94, 94, 255), IM_COL32(174, 128, 220, 255)};
    {
        if (ImGui::BeginTable("gold_distribution_bars", 3,
            ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Investment", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Share", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            bool rendered[36] = {};
            for (int chart_row = 0; chart_row < row_count; ++chart_row) {
                int index = -1;
                for (int candidate = 0; candidate < row_count; ++candidate) {
                    if (!rendered[candidate] && (index < 0
                        || rows[candidate].metrics.investment_percent > rows[index].metrics.investment_percent)) {
                        index = candidate;
                    }
                }
                rendered[index] = true;
                const double distribution_percent = rows[index].metrics.investment_percent;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(rows[index].name);
                ImGui::TableNextColumn();
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImGui::ColorConvertU32ToFloat4(colors[index % IM_ARRAYSIZE(colors)]));
                ImGui::ProgressBar((float)distribution_percent, ImVec2(-1.0f, 20.0f), "");
                ImGui::PopStyleColor();
                ImGui::TableNextColumn();
                ImGui::Text("%.1f%%", distribution_percent * 100.0);
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
        ImGui::Text("Total invested: %s Gold", investment_total);
        ImGui::Text("Total cost to target now: %s Gold", now_total);
        if (pace_available) {
            ImGui::Text("Total cost to target next period: %s Gold", next_total);
        } else {
            ImGui::TextDisabled("Total cost to target next period: N/A");
        }
    }
    EndPanel();

    DrawGoldPowerSection(total_investment, historical_stats);
}

static void DrawPlayerDataTab() {
    ImGui::Text("Enter your player progress details and save them to the local CSV.");
    ImGui::Spacing();
    if (ImGui::BeginTable("player_data_layout", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch, 0.55f);
        ImGui::TableSetupColumn("Display", ImGuiTableColumnFlags_WidthStretch, 0.45f);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
    BeginPanel("player_input_panel");
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
        if (HasUnsavedPlayerDataChanges()) {
            DrawUnsavedChangesDot();
        }
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
            char wave_text[32];
            char infinity_castle_text[32];
            char leader_text[32];
            char town_archer_text[32];
            char castle_text[32];
            FormatLevelValue(g_pending_player_deletion.wave, wave_text, sizeof(wave_text));
            FormatLevelValue(g_pending_player_deletion.infinity_castle_level, infinity_castle_text, sizeof(infinity_castle_text));
            FormatLevelValue(g_pending_player_deletion.leader_level, leader_text, sizeof(leader_text));
            FormatLevelValue(g_pending_player_deletion.town_archer_level, town_archer_text, sizeof(town_archer_text));
            FormatLevelValue(g_pending_player_deletion.castle_level, castle_text, sizeof(castle_text));
            ImGui::Text("Wave: %s", wave_text);
            ImGui::Text("Infinity Castle: %s", infinity_castle_text);
            ImGui::Text("Leader: %s", leader_text);
            ImGui::Text("Town Archers: %s", town_archer_text);
            ImGui::Text("Castle: %s", castle_text);
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
        EndPanel();

        ImGui::TableSetColumnIndex(1);
        BeginPanel("player_display_panel"); 
              DrawSectionHeading("DISPLAY SECTION");
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.10f, 0.145f, 0.21f, 0.62f));
                if (ImGui::BeginTable("player_display_table", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
                    ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthStretch, 0.55f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.45f);
                    const char* labels[] = {"Wave", "Infinity Castle", "Leader", "Town Archer", "Castle"};
                    char values[5][32];
                    FormatLevelValue(g_player.wave, values[0], sizeof(values[0]));
                    FormatLevelValue(g_player.infinity_castle_level, values[1], sizeof(values[1]));
                    FormatLevelValue(g_player.leader_level, values[2], sizeof(values[2]));
                    FormatLevelValue(g_player.town_archer_level, values[3], sizeof(values[3]));
                    FormatLevelValue(g_player.castle_level, values[4], sizeof(values[4]));
                    for (int index = 0; index < 5; ++index) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::TextUnformatted(labels[index]);
                        ImGui::TableNextColumn();
                        if (index == 0) {
                            ImGui::TextUnformatted(values[index]);
                        } else {
                            ImGui::Text("Lv. %s", values[index]);
                        }
                    }
                    for (int index = 0; index < g_custom_hero_count; ++index) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::TextUnformatted(g_custom_heroes[index].name);
                        char level_text[32];
                        FormatLevelValue(g_custom_heroes[index].level, level_text, sizeof(level_text));
                        ImGui::TableNextColumn(); ImGui::Text("Lv. %s", level_text);
                    }
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::TextUnformatted("Last Update");
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(g_player.last_update[0] ? g_player.last_update : "N/A");
                    ImGui::EndTable();
                }
                ImGui::PopStyleColor();
        EndPanel();
        ImGui::EndTable();
    }

    DrawSectionBreak();
    if (ImGui::BeginTable("custom_hero_layout", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Add Custom", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Your Custom Unit", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
    BeginPanel("add_custom_panel");
    DrawSectionHeading("Add Custom");
    ImGui::SetNextItemWidth(420.0f);
    ImGui::InputText("Hero / Tower Name", g_custom_hero_name, IM_ARRAYSIZE(g_custom_hero_name));
    ImGui::SetNextItemWidth(420.0f);
    ImGui::InputFloat("Target Ratio", &g_custom_hero_target_ratio, 0.001f, 0.01f, "%.4f");
    if (g_custom_hero_target_ratio < 0.0f) g_custom_hero_target_ratio = 0.0f;
    DrawHelpMarker("All ratios, including this one, are also editable later in the 'Ratios & Economy' tab.");
    ImGui::SetNextItemWidth(420.0f);
    ImGui::InputScalar("Current Level", ImGuiDataType_S64, &g_custom_hero_level, &kInt64Step, &kInt64StepFast);
    PushPrimaryButtonStyle();
    if (ImGui::Button("Save New Custom Hero")) {
        AddCustomHeroFromGui();
    }
    ImGui::PopStyleColor(3);
    DrawSaveConfirmation("add_custom_hero");
    EndPanel();

        ImGui::TableSetColumnIndex(1);
    if (g_custom_hero_count > 0) {
        ImGui::Spacing();
        DrawSubsectionHeading("Your Custom Unit");
        ImGui::BeginChild("custom_hero_list", ImVec2(0, 120), true);
        for (int i = 0; i < g_custom_hero_count; ++i) {
            CustomHero& hero = g_custom_heroes[i];
            std::string level_label = std::string(hero.name) + " Level##custom_player_level_" + std::to_string(i);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::InputScalar(level_label.c_str(), ImGuiDataType_S64, &hero.level, &kInt64Step, &kInt64StepFast);
        }
        ImGui::EndChild();
        PushPrimaryButtonStyle();
        if (ImGui::Button("Save Level##Save Custom Heroes", ImVec2(190.0f, 0.0f))) {
            SaveCustomHeroesFromGui();
        }
        ImGui::PopStyleColor(3);
        DrawSaveConfirmation("custom_heroes");
        if (HasUnsavedCustomHeroChanges()) {
            DrawUnsavedChangesDot();
        }
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
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    DrawStatusMessage();
}

static const ImGuiTableFlags kRatioTableFlags =
    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;

static void DrawCurrentRatioValue(float ratio, const ImVec4& color, const char* indicator)
{
    const ImVec2 cell_start = ImGui::GetCursorScreenPos();
    ImGui::TextColored(color, "%.4f", ratio);
    const ImVec2 indicator_size = ImGui::CalcTextSize(indicator);
    const ImVec2 indicator_position(
        cell_start.x + ImGui::GetColumnWidth() - indicator_size.x - ImGui::GetStyle().CellPadding.x,
        ImGui::GetItemRectMin().y);
    ImGui::GetWindowDrawList()->AddText(indicator_position, ImGui::GetColorU32(color), indicator);
}

static void DrawRatioSuggestionTab() {
    ComputeRatios();
    char wave_text[32];
    FormatLevelValue(g_player.wave, wave_text, sizeof(wave_text));
    ImGui::Text("Ratio analysis based on current player data. Scroll down to see Investment & Cost, Gold distribution and Gold Power");
    ImGui::SameLine(0.0f, 12.0f);
    ImGui::PushFont(GetUiBoldFont());
    const float wave_line_width = ImGui::CalcTextSize("Current Wave  ").x + ImGui::CalcTextSize(wave_text).x;
    ImGui::PopFont();
    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - wave_line_width);
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(UiColors::Muted, "Current Wave");
    ImGui::SameLine(0.0f, 6.0f);
    ImGui::PushFont(GetUiBoldFont());
    ImGui::TextColored(UiColors::Info, "%s", wave_text);
    ImGui::PopFont();
    ImGui::Spacing();

    BeginPanel("ratio_overview_panel");
    DrawSectionHeading("RATIOS");
    std::string ratio_clipboard_text;
    char ratio_clipboard_line[96];
    ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.10f, 0.145f, 0.21f, 0.62f));
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

        BeginHoverRow("##ratio_row_leader", ImGui::GetFrameHeight());
        ImGui::Text("Leader");
    char level_text[32];
    FormatLevelValue(g_player.leader_level, level_text, sizeof(level_text));
    ImGui::TableNextColumn(); ImGui::TextUnformatted(level_text);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputFloat("##leader_ratio", &g_player.recommended_ratios.leader, 0.001f, 0.01f, "%.4f");
        if (g_player.recommended_ratios.leader < 0.0f) g_player.recommended_ratios.leader = 0.0f;
        ImGui::TableNextColumn();
        float leader_current_ratio = (g_player.wave > 0) ? (float)((double)g_player.leader_level / (double)g_player.wave) : 0.0f;
        const ImVec4* leader_ratio_color;
        const char* leader_ratio_indicator;
        if (leader_current_ratio < g_player.recommended_ratios.leader) {
            leader_ratio_color = &UiColors::Danger;
            leader_ratio_indicator = "▼";
        } else {
            leader_ratio_color = &UiColors::Success;
            leader_ratio_indicator = "▲";
        }
        DrawCurrentRatioValue(leader_current_ratio, *leader_ratio_color, leader_ratio_indicator);
        snprintf(ratio_clipboard_line, sizeof(ratio_clipboard_line), "Leader: %.4f\n", leader_current_ratio);
        ratio_clipboard_text += ratio_clipboard_line;
        ImGui::TableNextColumn();
        long long leader_gap = (long long)((double)g_player.leader_level - (double)g_player.wave * (double)g_player.recommended_ratios.leader);
        char gap_text[32];
        FormatLevelValue(leader_gap < 0 ? -leader_gap : leader_gap, gap_text, sizeof(gap_text));
        if (leader_gap < 0) {
            ImGui::TextColored(UiColors::Danger, "-%s", gap_text);
        } else {
            ImGui::TextColored(UiColors::Success, "%s", gap_text);
        }

        BeginHoverRow("##ratio_row_ic", ImGui::GetTextLineHeight());
        ImGui::Text("Infinity Castle");
        FormatLevelValue(g_player.infinity_castle_level, level_text, sizeof(level_text));
        ImGui::TableNextColumn(); ImGui::TextUnformatted(level_text);
        ImGui::TableNextColumn(); ImGui::Text("As high as possible");
        ImGui::TableNextColumn();
        float colony_current_ratio = (g_player.wave > 0) ? (float)((double)g_player.infinity_castle_level / (double)g_player.wave) : 0.0f;
        ImGui::TextColored(UiColors::Info, "%.4f", colony_current_ratio);
        snprintf(ratio_clipboard_line, sizeof(ratio_clipboard_line), "IC: %.4f\n", colony_current_ratio);
        ratio_clipboard_text += ratio_clipboard_line;
        ImGui::TableNextColumn();
        ImGui::TextColored(UiColors::Muted, "-");

        BeginHoverRow("##ratio_row_ta", ImGui::GetFrameHeight());
        ImGui::Text("Town Archer");
        FormatLevelValue(g_player.town_archer_level, level_text, sizeof(level_text));
        ImGui::TableNextColumn(); ImGui::TextUnformatted(level_text);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputFloat("##town_archer_ratio", &g_player.recommended_ratios.town_archer, 0.001f, 0.01f, "%.4f");
        if (g_player.recommended_ratios.town_archer < 0.0f) g_player.recommended_ratios.town_archer = 0.0f;
        ImGui::TableNextColumn();
        float town_archer_current_ratio = (g_player.wave > 0) ? (float)((double)g_player.town_archer_level / (double)g_player.wave) : 0.0f;
        const ImVec4* town_archer_ratio_color;
        const char* town_archer_ratio_indicator;
        if (town_archer_current_ratio < g_player.recommended_ratios.town_archer) {
            town_archer_ratio_color = &UiColors::Danger;
            town_archer_ratio_indicator = "▼";
        } else {
            town_archer_ratio_color = &UiColors::Success;
            town_archer_ratio_indicator = "▲";
        }
        DrawCurrentRatioValue(town_archer_current_ratio, *town_archer_ratio_color, town_archer_ratio_indicator);
        snprintf(ratio_clipboard_line, sizeof(ratio_clipboard_line), "TA: %.4f\n", town_archer_current_ratio);
        ratio_clipboard_text += ratio_clipboard_line;
        ImGui::TableNextColumn();
        long long archer_gap = (long long)((double)g_player.town_archer_level - (double)g_player.wave * (double)g_player.recommended_ratios.town_archer);
        FormatLevelValue(archer_gap < 0 ? -archer_gap : archer_gap, gap_text, sizeof(gap_text));
        if (archer_gap < 0) {
            ImGui::TextColored(UiColors::Danger, "-%s", gap_text);
        } else {
            ImGui::TextColored(UiColors::Success, "%s", gap_text);
        }

        BeginHoverRow("##ratio_row_castle", ImGui::GetFrameHeight());
        ImGui::Text("Castle");
        FormatLevelValue(g_player.castle_level, level_text, sizeof(level_text));
        ImGui::TableNextColumn(); ImGui::TextUnformatted(level_text);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputFloat("##castle_ratio", &g_player.recommended_ratios.castle, 0.001f, 0.01f, "%.4f");
        if (g_player.recommended_ratios.castle < 0.0f) g_player.recommended_ratios.castle = 0.0f;
        ImGui::TableNextColumn();
        float castle_current_ratio = (g_player.wave > 0) ? (float)((double)g_player.castle_level / (double)g_player.wave) : 0.0f;
        const ImVec4* castle_ratio_color;
        const char* castle_ratio_indicator;
        if (castle_current_ratio < g_player.recommended_ratios.castle) {
            castle_ratio_color = &UiColors::Danger;
            castle_ratio_indicator = "▼";
        } else {
            castle_ratio_color = &UiColors::Success;
            castle_ratio_indicator = "▲";
        }
        DrawCurrentRatioValue(castle_current_ratio, *castle_ratio_color, castle_ratio_indicator);
        snprintf(ratio_clipboard_line, sizeof(ratio_clipboard_line), "Castle: %.4f\n", castle_current_ratio);
        ratio_clipboard_text += ratio_clipboard_line;
        ImGui::TableNextColumn();
        long long castle_gap = (long long)((double)g_player.castle_level - (double)g_player.wave * (double)g_player.recommended_ratios.castle);
        FormatLevelValue(castle_gap < 0 ? -castle_gap : castle_gap, gap_text, sizeof(gap_text));
        if (castle_gap < 0) {
            ImGui::TextColored(UiColors::Danger, "-%s", gap_text);
        } else {
            ImGui::TextColored(UiColors::Success, "%s", gap_text);
        }

        if (g_custom_hero_count > 0) {
            // Separator row for custom units
            ImGui::TableNextRow();
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1,
                ImGui::GetColorU32(ImVec4(UiColors::Heading.x, UiColors::Heading.y, UiColors::Heading.z, 0.18f)));
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(UiColors::Subheading, "CUSTOM UNITS");

            for (int i = 0; i < g_custom_hero_count; ++i) {
                CustomHero& hero = g_custom_heroes[i];
                BeginHoverRow(("##ratio_row_custom_" + std::to_string(i)).c_str(), ImGui::GetFrameHeight());
                ImGui::Text("%s", hero.name);
                char hero_level_text[32];
                FormatLevelValue(hero.level, hero_level_text, sizeof(hero_level_text));
                ImGui::TableNextColumn(); ImGui::TextUnformatted(hero_level_text);
                ImGui::TableNextColumn();
                float current_ratio = (g_player.wave > 0) ? (float)((double)hero.level / (double)g_player.wave) : 0.0f;
                ImGui::SetNextItemWidth(-1.0f);
                ImGui::InputFloat(("##custom_ratio_" + std::to_string(i)).c_str(), &hero.target_ratio, 0.001f, 0.01f, "%.4f");
                if (hero.target_ratio < 0.0f) hero.target_ratio = 0.0f;
                ImGui::TableNextColumn();
                const ImVec4* custom_ratio_color;
                const char* custom_ratio_indicator;
                if (current_ratio < hero.target_ratio) {
                    custom_ratio_color = &UiColors::Danger;
                    custom_ratio_indicator = "▼";
                } else {
                    custom_ratio_color = &UiColors::Success;
                    custom_ratio_indicator = "▲";
                }
                DrawCurrentRatioValue(current_ratio, *custom_ratio_color, custom_ratio_indicator);
                snprintf(ratio_clipboard_line, sizeof(ratio_clipboard_line), "%s: %.4f\n", hero.name, current_ratio);
                ratio_clipboard_text += ratio_clipboard_line;
                ImGui::TableNextColumn();
                long long custom_gap = (long long)((double)hero.level - (double)g_player.wave * (double)hero.target_ratio);
                char custom_gap_text[32];
                FormatLevelValue(custom_gap < 0 ? -custom_gap : custom_gap, custom_gap_text, sizeof(custom_gap_text));
                if (custom_gap < 0) {
                    ImGui::TextColored(UiColors::Danger, "-%s", custom_gap_text);
                } else {
                    ImGui::TextColored(UiColors::Success, "%s", custom_gap_text);
                }
            }
        }

        ImGui::EndTable();
    }
    ImGui::PopStyleColor();

    if (g_custom_hero_count == 0) {
        ImGui::TextDisabled("No custom heroes saved yet.");
    }

    PushPrimaryButtonStyle();
    if (ImGui::Button("Save Ratios", ImVec2(220.0f, 0.0f))) {
        SaveRecommendedRatios();
        SaveCustomHeroesFromGui();
    }
    ImGui::PopStyleColor(3);
    if (HasUnsavedRatioChanges()) {
        DrawUnsavedChangesDot();
    }
    if (g_save_confirmation_until > ImGui::GetTime()
        && (strcmp(g_save_confirmation_target, "recommended_ratios") == 0
            || strcmp(g_save_confirmation_target, "custom_heroes") == 0)) {
        ImGui::SameLine();
        ImGui::TextColored(UiColors::Success, "Ratios saved");
    }

    const ImVec2 copy_ratios_button_size(170.0f, 0.0f);
    const float copy_ratios_help_marker_width = ImGui::CalcTextSize("(?)").x;
    const float copy_ratios_row_width = copy_ratios_button_size.x + ImGui::GetStyle().ItemSpacing.x + copy_ratios_help_marker_width;
    ImGui::SameLine();
    const bool show_copied_confirmation = g_save_confirmation_until > ImGui::GetTime()
        && strcmp(g_save_confirmation_target, "ratio_clipboard") == 0;
    if (show_copied_confirmation) {
        const float copied_text_width = ImGui::CalcTextSize("Copied!").x;
        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - copy_ratios_row_width - ImGui::GetStyle().ItemSpacing.x - copied_text_width);
        ImGui::TextColored(UiColors::Success, "Copied!");
        ImGui::SameLine();
    }
    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - copy_ratios_row_width);
    if (ImGui::Button("Copy to Clipboard", copy_ratios_button_size)) {
        ImGui::SetClipboardText(ratio_clipboard_text.c_str());
        MarkDataSaved("ratio_clipboard");
    }
    DrawHelpMarker(
        "Copies the Subject and Current Ratio of every row in the table\n"
        "(fixed units and custom units), in the order shown, one per line:\n"
        "Subject: CurrentRatio");
    EndPanel();

    DrawInvestmentAndCostSection();
}

static void DrawColonyStatsTab() {
    BeginPanel("colony_stats_panel");
    DrawSectionHeading("CORE METRICS");
    if (ImGui::BeginTable("colony_stats_layout", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Core", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Gold", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableNextColumn();
        char infinity_castle_text[32];
        FormatLevelValue(
            g_player.infinity_castle_level,
            infinity_castle_text,
            sizeof(infinity_castle_text)
        );
        ImGui::Text(
            "Infinity Castle Level: %s",
            infinity_castle_text
        );
        ImGui::Text(
            "Colony Ratio: %.4f",
            g_ratio_colony
        );
        double formatted_ratio = 0.0;
        if (g_player.wave > 0) {
            formatted_ratio = (double)(g_player.infinity_castle_level*1000 + 1000) / (double)g_player.wave;
        }
        char ratio_text[16];
        snprintf(ratio_text, sizeof(ratio_text), "%.1fX", formatted_ratio);
        DrawHeroMetric("Ratio (defense):", UiColors::Info, ratio_text);
        ImGui::TableNextColumn();
        char base_gold_text[64];
        char xp_gold_text[64];
        char whip_gold_text[64];
       FormatProfitValue(
            g_colony_gold,
            base_gold_text,
            sizeof(base_gold_text)
        );
        FormatProfitValue(
            g_gold_xp,
            xp_gold_text,
            sizeof(xp_gold_text)
        );
        FormatProfitValue(
            g_gold_whip,
            whip_gold_text,
            sizeof(whip_gold_text)
        );
        ImGui::Text(
            "Base Gold: "
        );
        ImGui::SameLine(0.0f, 12.0f);
        ImGui::TextUnformatted(base_gold_text);
        ImGui::Text(
            "Gold with Whip + Skill: %s",
            whip_gold_text
        );
        DrawHeroMetric("Gold with XP Skill Buff:", UiColors::Success, xp_gold_text);
        ImGui::EndTable();
    }
    EndPanel();
}

static void DrawProgressHistoryTab() {
    static int entry_limit_index = 2;
    const int entry_limits[] = {5, 10, 20, 50};

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

        char latest_wave_text[32];
        FormatLevelValue(g_progress[start_index + display_count - 1].wave, latest_wave_text, sizeof(latest_wave_text));
        DrawHeroMetric("LATEST WAVE", UiColors::Info, latest_wave_text);
        ImGui::Spacing();

        ImGui::Text("Progress history loaded from data/player_data.csv");
        ImGui::Text("Entries: %d", g_progress_count);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        ImGui::Combo("Entries to show", &entry_limit_index, "5\0" "10\0" "20\0" "50\0\0");
        ImGui::Separator();
        if (g_progress_invalid_row_count > 0) {
            ImGui::TextColored(UiColors::Danger, "%d corrupt history row(s) were ignored.", g_progress_invalid_row_count);
        }
        ImGui::Text("Showing latest %d entries.", display_count);
        ImGui::Spacing();

        ImVec2 graph_size = ImVec2(-1.0f, 220.0f);
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
            snprintf(label, sizeof(label), "%.4f", value);
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
            FormatLevelValue(g_progress[start_index + idx].wave, label, sizeof(label));
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

        if (ImGui::IsMouseHoveringRect(graph_min, graph_max)) {
            const float mouse_x = ImGui::GetMousePos().x;
            int hovered_index = display_count == 1
                ? 0
                : (int)(((mouse_x - graph_min.x) / width) * (display_count - 1) + 0.5f);
            if (hovered_index < 0) hovered_index = 0;
            if (hovered_index >= display_count) hovered_index = display_count - 1;

            const float hover_x = display_count == 1
                ? graph_min.x + width * 0.5f
                : graph_min.x + ((float)hovered_index / (display_count - 1)) * width;
            const float hover_normalized = (float)((ratios[hovered_index] - graph_min_ratio) / ratio_range);
            const float hover_y = graph_max.y - hover_normalized * height;
            draw_list->AddCircleFilled(ImVec2(hover_x, hover_y), 5.5f, IM_COL32(255, 255, 255, 255));
            draw_list->AddCircle(ImVec2(hover_x, hover_y), 7.5f, IM_COL32(255, 200, 100, 255), 0, 2.0f);

            const ProgressData& hovered_entry = g_progress[start_index + hovered_index];
            char hovered_wave_text[32];
            char hovered_level_text[32];
            FormatLevelValue(hovered_entry.wave, hovered_wave_text, sizeof(hovered_wave_text));
            FormatLevelValue(hovered_entry.infinity_castle_level, hovered_level_text, sizeof(hovered_level_text));
            ImGui::BeginTooltip();
            ImGui::Text("Date: %s", hovered_entry.date);
            ImGui::Text("Wave: %s", hovered_wave_text);
            ImGui::Text("Infinity Castle: %s", hovered_level_text);
            ImGui::Text("Ratio: %.4f", ratios[hovered_index]);
            ImGui::EndTooltip();
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
                char wave_text[32];
                char level_text[32];
                FormatLevelValue(g_progress[i].wave, wave_text, sizeof(wave_text));
                FormatLevelValue(g_progress[i].infinity_castle_level, level_text, sizeof(level_text));
                ImGui::TableNextColumn(); ImGui::TextUnformatted(wave_text);
                ImGui::TableNextColumn(); ImGui::TextUnformatted(level_text);
                ImGui::TableNextColumn(); ImGui::Text("%.4f", g_progress[i].wave > 0 ? (double)g_progress[i].infinity_castle_level / g_progress[i].wave : 0.0);
            }
            ImGui::EndTable();
        }
    } else {
        ImGui::TextDisabled("No progress history available. Save player data to build up the history.");
    }
}

static void DrawGpwSection()
{
    BeginPanel("gpw_input_panel");
    DrawSectionHeading("Gold Per Wave", "After entering your data, your average gain per wave will be calculated.\nConsider 456 as your breakeven point, If the number you see is higher than that, you're in profit; otherwise, you're at a loss.");
    ImGui::Text("Check whether GAB (Gold Auto Battle) is profitable at your current saved wave");
    char wave_text[32];
    FormatLevelValue(g_player.wave, wave_text, sizeof(wave_text));
    ImGui::Text("Wave reached: %s", wave_text);
    ImGui::SetNextItemWidth(180.0f);
    ImGui::InputInt("Gold samples", &g_gpw_sample_count);
    DrawHelpMarker("Enter the gold earned per GAB run. More samples make the calculation more reliable. GAB samples are not saved by the app.");
    if (g_gpw_sample_count < 0) g_gpw_sample_count = 0;
    if (g_gpw_sample_count > IM_ARRAYSIZE(g_gpw_samples)) g_gpw_sample_count = IM_ARRAYSIZE(g_gpw_samples);
    for (int index = 0; index < g_gpw_sample_count; ++index) {
        ImGui::SetNextItemWidth(260.0f);
        ImGui::InputScalar(("Gold obtained##gpw_sample_" + std::to_string(index)).c_str(),
            ImGuiDataType_S64, &g_gpw_samples[index], &kInt64Step, &kInt64StepFast);
    }

    PushPrimaryButtonStyle();
    if (ImGui::Button("Calculate GPW")) {
        double gold_samples[IM_ARRAYSIZE(g_gpw_samples)];
        for (int index = 0; index < g_gpw_sample_count; ++index) {
            gold_samples[index] = (double)g_gpw_samples[index];
        }
        g_gpw_error = calculate_gpw_analysis(gold_samples, g_gpw_sample_count,
            (double)g_player.wave, kGpwCostPerWave, &g_gpw_analysis);
        g_gpw_ready = g_gpw_error == GPW_CALCULATION_OK;
    }
    ImGui::PopStyleColor(3);
    EndPanel();

    if (!g_gpw_ready) {
        if (g_gpw_error == GPW_CALCULATION_NO_SAMPLES) {
            ImGui::TextColored(UiColors::Danger, "Unable to calculate: enter at least one gold sample.");
        } else if (g_gpw_error == GPW_CALCULATION_NEGATIVE_SAMPLE) {
            ImGui::TextColored(UiColors::Danger, "Unable to calculate: gold samples cannot be negative.");
        } else if (g_gpw_error == GPW_CALCULATION_INVALID_WAVE) {
            ImGui::TextColored(UiColors::Danger, "Unable to calculate: save Player Data with a valid wave first.");
        }
        return;
    }

    ImGui::Spacing();
    char gpw_average_text[64];
    char gpw_maximum_text[64];
    char gpw_minimum_text[64];
    char profit_average_text[64];
    char profit_maximum_text[64];
    char profit_minimum_text[64];
    FormatGpwResultValue(g_gpw_analysis.gpw.avg, gpw_average_text, sizeof(gpw_average_text));
    FormatGpwResultValue(g_gpw_analysis.gpw.max, gpw_maximum_text, sizeof(gpw_maximum_text));
    FormatGpwResultValue(g_gpw_analysis.gpw.min, gpw_minimum_text, sizeof(gpw_minimum_text));
    FormatGpwResultValue(g_gpw_analysis.profit.avg < 0.0 ? -g_gpw_analysis.profit.avg : g_gpw_analysis.profit.avg,
        profit_average_text, sizeof(profit_average_text));
    FormatGpwResultValue(g_gpw_analysis.profit.max < 0.0 ? -g_gpw_analysis.profit.max : g_gpw_analysis.profit.max,
        profit_maximum_text, sizeof(profit_maximum_text));
    FormatGpwResultValue(g_gpw_analysis.profit.min < 0.0 ? -g_gpw_analysis.profit.min : g_gpw_analysis.profit.min,
        profit_minimum_text, sizeof(profit_minimum_text));
    BeginPanel("gpw_result_panel");
    DrawSubsectionHeading("RESULT");
    ImGui::Text("Samples: %d", g_gpw_analysis.sampleCount);
    ImGui::TextColored(UiColors::Muted, "GPW AVERAGE");
    DrawHeroValue(UiColors::Info, gpw_average_text);
    ImGui::Text("GPW max: %s Gold/wave | min: %s", gpw_maximum_text, gpw_minimum_text);
    ImGui::Text("Profit per wave average: %s%s Gold/wave | max: %s%s | min: %s%s",
        g_gpw_analysis.profit.avg < 0.0 ? "-" : "", profit_average_text,
        g_gpw_analysis.profit.max < 0.0 ? "-" : "", profit_maximum_text,
        g_gpw_analysis.profit.min < 0.0 ? "-" : "", profit_minimum_text);
    ImGui::Spacing();
    if (g_gpw_analysis.isProfitable) {
        ImGui::TextColored(UiColors::Success, "Gold Auto Battle is profitable.");
    } else {
        ImGui::TextColored(UiColors::Danger, "Gold Auto Battle is running at a loss.");
    }
    if (g_gpw_analysis.reliabilityWarning) {
        ImGui::TextColored(UiColors::Warning, "Fewer than five samples: this result may be unreliable.");
    }
    EndPanel();
}

static void DrawUpgradingCostTab() {
    ImGui::Text("Calculate gold cost for upgrades.");
    ImGui::Separator();

    BeginPanel("upgrade_inputs_panel");
    ImGui::SetNextItemWidth(320.0f);
    ImGui::Combo("Upgrade Type", &g_upgrade_type, "Castle\0Town Archers\0Hero/Leader/Tower\0");
    ImGui::SetNextItemWidth(320.0f);
    ImGui::InputScalar("From level", ImGuiDataType_S64, &g_upgrade_from);
    ImGui::SetNextItemWidth(320.0f);
    ImGui::InputScalar("To level", ImGuiDataType_S64, &g_upgrade_to);

    PushPrimaryButtonStyle();
    if (ImGui::Button("Compute Cost")) {
        if (g_upgrade_from >= 0 && g_upgrade_to > g_upgrade_from) {
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
    EndPanel();

    if (g_upgrade_ready) {
        char cost_text[64];
        FormatGoldValue(g_upgrade_cost, cost_text, sizeof(cost_text));
        DrawSectionBreak();
        DrawSectionHeading("CALCULATION RESULT");
        ImGui::Spacing();
        BeginPanel("upgrade_result_panel");
        ImGui::TextColored(UiColors::Muted, "ESTIMATED COST");
        DrawHeroValue(UiColors::Success, cost_text);
        ImGui::TextColored(UiColors::Success, "Gold");
        EndPanel();
        ImGui::Spacing();
    } else {
        ImGui::Text("Enter valid levels and press Compute Cost.");
    }

    DrawSectionBreak();
    DrawGpwSection();
}

static void DrawPaceAnalysisTab() {
    calculatePaceStats(&g_pace_inputs, &g_pace_stats);
    ImGui::Text("Calculate waves per hour/season and compare them with your actual pace based on the data entered in Player Data.");
    ImGui::Spacing();

    const float avail_width  = ImGui::GetContentRegionAvail().x;
    const float item_spacing = ImGui::GetStyle().ItemSpacing.x;
    const float left_width   = avail_width * 0.38f; 
    const float right_width  = avail_width - left_width - item_spacing;

    ImGui::BeginChild("pace_settings_column", ImVec2(left_width, 0.0f), false);
    {
        BeginPanel("pace_settings_panel");
        DrawSectionHeading("PACE SETTINGS");
        bool changed = false;

        ImGui::Text("Devil Horn");
        ImGui::SetNextItemWidth(-FLT_MIN);
        changed |= ImGui::Combo("##dh_level", &g_pace_inputs.dhLevel, "0\0" "1\0" "2\0" "3\0" "4\0" "5\0\0");

        ImGui::Spacing();
        ImGui::Text("Game Speed");
        int game_speed_index = g_pace_inputs.gameSpeed - 2;
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Combo("##game_speed", &game_speed_index, "2x\0" "3x\0\0")) {
            g_pace_inputs.gameSpeed = game_speed_index + 2;
            changed = true;
        }

        ImGui::Spacing();
        ImGui::Text("Chrono");
        int chrono_index = (int)g_pace_inputs.chrono;
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Combo("##chrono", &chrono_index, "None\0Passive\0Yellow\0Blue\0")) {
            g_pace_inputs.chrono = (PaceChrono)chrono_index;
            changed = true;
        }

        ImGui::Spacing();
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
        EndPanel();
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("pace_results_column", ImVec2(right_width, 0.0f), false);
    {
        if (!g_pace_stats.isValid) {
            ImGui::TextColored(UiColors::Danger, "%s", g_pace_stats.validationMessage[0] ? g_pace_stats.validationMessage : "Invalid pace input.");
        } else {
            BeginPanel("pace_results_panel");
            DrawSubsectionHeading("PACE RESULTS");
            ImGui::TextColored(UiColors::Success, "RWPH: %d", g_pace_stats.rwph);
            char wph_text[32];
            snprintf(wph_text, sizeof(wph_text), "%.2f WPH", g_pace_stats.wph);
            DrawHeroValue(UiColors::Success, wph_text);
            ImGui::Spacing();
            DrawSubsectionHeading("TOTAL WAVES");
            ImGui::Text("Waves / Day: %.2f", g_pace_stats.wavesPerDay);
            ImGui::Text("Waves / Season (5 days): %.2f", g_pace_stats.wavesPerSeason);

            DrawSectionBreak();
            DrawSubsectionHeading("ACTUAL PACE & DOWNTIME");
            ImGui::TextWrapped("Actual Pace uses the first and last Player Data records in the selected period. Estimated downtime is the elapsed time minus the time required to complete the same waves at the expected pace.");

            ImGui::Text("History Period");
            ImGui::SetNextItemWidth(260.0f);
            ImGui::Combo("##history_period", &g_pace_history_period, "All Time\0Last Month\0Last 5 Days\0Last 24 Hours\0\0");

            HistoricalPaceStats historical_stats = CalculateHistoricalPaceStats(g_pace_history_period);
            if (!historical_stats.isValid) {
                ImGui::TextColored(UiColors::Muted, "%s", historical_stats.message);
            } else {
                ImGui::Text("Records used: %d", historical_stats.entryCount);
                ImGui::Spacing();
                const float inner_avail    = ImGui::GetContentRegionAvail().x;
                const float inner_spacing   = ImGui::GetStyle().ItemSpacing.x;
                const float col_width       = (inner_avail - inner_spacing) * 0.5f;
                const float pace_col_height = 110.0f;

                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);

                ImGui::BeginChild("actual_pace_col", ImVec2(col_width, pace_col_height), true);
                ImGui::TextColored(UiColors::Info, "Actual Pace");
                ImGui::Text("Waves / Hour: %.2f", historical_stats.wavesPerHour);
                ImGui::Text("Waves / Day: %.2f", historical_stats.wavesPerDay);
                ImGui::Text("Waves / Season (5 days): %.2f", historical_stats.wavesPerSeason);
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("expected_pace_col", ImVec2(col_width, pace_col_height), true);
                ImGui::TextColored(UiColors::Success, "Expected Pace");
                ImGui::Text("Waves / Hour: %.2f", g_pace_stats.wph);
                ImGui::Text("Waves / Day: %.2f", g_pace_stats.wavesPerDay);
                ImGui::Text("Waves / Season (5 days): %.2f", g_pace_stats.wavesPerSeason);
                ImGui::EndChild();

                ImGui::PopStyleVar(2);

                ImGui::Spacing();
                DrawSectionBreak();

                const float downtime_panel_height = 90.0f;
                ImGui::BeginChild("downtime_scroll_panel", ImVec2(0.0f, downtime_panel_height), true);
                ImGui::TextColored(UiColors::Warning, "Estimated Downtime: %.2f hours", historical_stats.downtimeHours);
                ImGui::Text("Downtime %% of selected elapsed time: %.2f %%", historical_stats.downtimePercentage);
                if (historical_stats.usesDateOnlyEntries) {
                    ImGui::TextColored(UiColors::Warning, "Some older records have no time of day and are treated as midnight.");
                }
                ImGui::EndChild();
            }
            EndPanel();
        }
    }
    ImGui::EndChild();
}

static void DrawClickableLinkText(const char* label, const ImVec4& color, const char* url)
{
    ImGui::TextColored(color, "%s", label);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Click to open:\n%s", url);
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
#ifdef _WIN32
        ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
#else
        (void)url;
#endif
    }
}

static void DrawInfoTab() {

    BeginPanel("info_header");

    ImGui::Indent(8.0f);
    ImGui::PushFont(GetUiHeaderFont());
    ImGui::TextUnformatted("Grow Castle Progress Tracker");
    ImGui::PopFont();
    DrawHeroValue(UiColors::Info, "v5.0.0");

    ImGui::Spacing();

    ImGui::TextUnformatted("Built and Maintained by ");
    ImGui::SameLine(0.0f, 0.0f);
    DrawClickableLinkText("@miglioDev", UiColors::InfoAuthor, "https://youtube.com/@migliodev");

    ImGui::Text("Thanks to ");
    ImGui::SameLine(0.0f, 0.0f);
    DrawClickableLinkText("@Laku", ImVec4(0.30f, 0.90f, 1.0f, 1.0f), "https://www.youtube.com/@lakujk");
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::TextUnformatted(" for supporting and sharing the project.");

    ImGui::Spacing();

    ImGui::TextLinkOpenURL(
        "GitHub Repository",
        "https://github.com/miglioDev/grow-castle-progress-tracker"
    );
    ImGui::Unindent(8.0f);

    EndPanel();
    ImGui::Spacing();
    ImGui::Spacing();

    BeginPanel("info_about_panel");
    DrawSubsectionHeading("ABOUT THE PROJECT");
    ImGui::Spacing();

    ImGui::TextWrapped(
        "For further information about the project, how it works, "
        "and the formulas used, please read the DOCUMENTATION.md "
        "file on GitHub."
    );

    ImGui::Spacing();

    ImGui::TextWrapped(
        "Have feedback, suggestions, or found a bug? Open an issue "
        "on GitHub, or reach out, you can found me on the official Grow Castle Discord "
        "server under @miglioDev  I'll get back to you when I have "
        "the chance."
    );

    ImGui::Spacing();

    ImGui::TextLinkOpenURL(
        "Report an issue on GitHub",
        "https://github.com/miglioDev/grow-castle-progress-tracker/issues"
    );

    ImGui::Spacing();

    ImGui::TextDisabled("Licensed under the MIT License.");
    EndPanel();

    ImGui::Spacing();

    BeginPanel("info_support_panel");
    DrawSubsectionHeading("SUPPORT THE PROJECT");
    ImGui::Spacing();

    ImGui::TextWrapped(
        "Enjoying the tool? A star on GitHub takes one click and "
        "costs nothing, but it helps other players discover the "
        "project and keeps me motivated to keep building it. "
        "Sharing it with others player goes a long way too."
    );

    ImGui::Spacing();

    ImGui::TextLinkOpenURL(
        "Star on GitHub",
        "https://github.com/miglioDev/grow-castle-progress-tracker"
    );
    EndPanel();

    ImGui::Spacing();

    BeginPanel("info_data_panel");
    DrawSubsectionHeading("DATA & BACKUP");
    ImGui::Spacing();

    ImGui::TextWrapped(
        "All data is stored locally in the ./data folder. "
        "You can import or export your progress by copying the "
        "CSV files to another location."
    );

    DrawSectionBreak();
    DrawSubsectionHeading("STORED FILES");
    ImGui::Spacing();

    struct FileEntry {
        const char* path;
        const char* description;
    };

    static constexpr FileEntry kStoredFiles[] = {
        { "./data/player_data.csv",   "All the standard player data" },
        { "./data/custom_heroes.csv", "Custom hero names, ratios, and levels" },
        { "./data/pace_data.csv",     "Saved RWPH/WPH calculator options" },
    };

    if (ImGui::BeginTable("stored_files_table", 2,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch, 0.9f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch, 1.4f);
        ImGui::PushStyleColor(ImGuiCol_Text, UiColors::Heading);
        ImGui::PushFont(GetUiBoldFont());
        ImGui::TableHeadersRow();
        ImGui::PopFont();
        ImGui::PopStyleColor();

        for (const auto& entry : kStoredFiles) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled(entry.path);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextWrapped(entry.description);
        }

        ImGui::EndTable();
    }

    DrawSectionBreak();
    DrawSubsectionHeading("BACKUP & RESTORE");
    ImGui::Spacing();

    ImGui::BulletText("Backup: copy all CSV files to a safe location.");
    ImGui::BulletText(
        "Restore: copy them back into the ./data folder with the "
        "same names, then restart the app."
    );
    ImGui::BulletText(
        "If a file is missing, that category has no saved data yet."
    );
    EndPanel();
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

    const ImVec2 title_cursor = ImGui::GetCursorPos();
    ImGui::PushFont(GetUiHeaderFont());
    const float title_height = ImGui::GetTextLineHeight();
    ImGui::TextUnformatted("Grow Castle Progress Tracker");
    ImGui::PopFont();

    constexpr float kMiglioDevScale = 1.15f;
    const ImVec2 by_size = ImGui::CalcTextSize("by");
    const float by_height = ImGui::GetTextLineHeight();
    ImGui::PushFont(GetUiBoldFont());
    ImGui::SetWindowFontScale(kMiglioDevScale);
    const ImVec2 migliodev_size = ImGui::CalcTextSize("miglioDev");
    const float migliodev_height = ImGui::GetTextLineHeight();
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();

    const float branding_width = by_size.x + 4.0f + migliodev_size.x;
    const bool branding_fits = ImGui::GetContentRegionAvail().x >= branding_width + 8.0f;
    if (branding_fits) {
        ImGui::SameLine(0.0f, 8.0f);
    }
    const ImVec2 row_cursor = ImGui::GetCursorPos();
    const float row_height = branding_fits ? title_height : by_height;

    ImGui::SetCursorPosY(row_cursor.y + (row_height - by_height) * 0.5f);
    ImGui::TextColored(UiColors::Muted, "by");
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::SetCursorPosY(row_cursor.y + (row_height - migliodev_height) * 0.5f);
    ImGui::PushFont(GetUiBoldFont());
    ImGui::SetWindowFontScale(kMiglioDevScale);
    ImGui::TextColored(UiColors::Heading, "miglioDev");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();

    const float footer_height = ImGui::GetTextLineHeight() + 10.0f;
    ImGui::Spacing();
    ImGui::BeginChild("MainContent", ImVec2(0.0f, -footer_height));
    if (ImGui::BeginTabBar("MainTabs")) {
        if (BeginBoldTabItem("Player Data")) {
            DrawPlayerDataTab();
            ImGui::EndTabItem();
        }
        if (BeginBoldTabItem("Ratios & Economy")) {
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
        if (BeginBoldTabItem("Upgrading Cost & GAB Profit")) {
            DrawUpgradingCostTab();
            ImGui::EndTabItem();
        }
        if (BeginBoldTabItem("Info:")) {
            DrawInfoTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::EndChild();

    const ImVec2 footer_cursor = ImGui::GetCursorPos();
    const ImVec2 footer_screen = ImGui::GetCursorScreenPos();
    const float footer_width = ImGui::GetContentRegionAvail().x;
    const float footer_text_y = footer_cursor.y + (footer_height - ImGui::GetTextLineHeight()) * 0.5f;
    ImGui::GetWindowDrawList()->AddRectFilled(
        footer_screen,
        ImVec2(footer_screen.x + footer_width, footer_screen.y + footer_height),
        ImGui::GetColorU32(UiColors::PanelBg)
    );

    ImGui::SetCursorPos(ImVec2(footer_cursor.x + 8.0f, footer_text_y));
    ImGui::TextColored(UiColors::Muted, "v5.0.0");

    const float footer_branding_width = ImGui::CalcTextSize("miglioDev").x;
    ImGui::SetCursorPosY(footer_text_y);
    ImGui::SetCursorPosX(footer_cursor.x + footer_width - footer_branding_width - 8.0f);
    ImGui::TextColored(UiColors::Heading, "miglioDev");
    ImGui::SetCursorPos(footer_cursor);
    ImGui::Dummy(ImVec2(footer_width, footer_height));
    ImGui::End();
}
