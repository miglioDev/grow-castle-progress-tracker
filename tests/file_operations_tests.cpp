#include "file_operations.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>

static void resetDataDirectory() {
    std::filesystem::remove_all("data");
    std::filesystem::create_directory("data");
}

static Player makePlayer(const char* date, int wave) {
    Player player = {};
    std::snprintf(player.last_update, sizeof(player.last_update), "%s", date);
    player.wave = wave;
    player.infinity_castle_level = wave + 1;
    player.leader_level = wave + 2;
    player.town_archer_level = wave + 3;
    player.castle_level = wave + 4;
    player.recommended_ratios = {0.1f, 0.2f, 0.3f};
    return player;
}

static void testDeleteLastPlayerRecord() {
    resetDataDirectory();
    Player first = makePlayer("2026-08-01 10:00:00", 100);
    Player second = makePlayer("2026-08-02 10:00:00", 200);
    assert(save_player_data(&first) == 1);
    assert(save_player_data(&second) == 1);
    assert(delete_last_player_record() == 1);

    Player loaded = {};
    assert(load_last_player_data(&loaded) == 1);
    assert(loaded.wave == 100);
    assert(std::strcmp(loaded.last_update, "2026-08-01 10:00:00") == 0);

    assert(delete_last_player_record() == 1);
    assert(load_last_player_data(&loaded) == 0);
    assert(delete_last_player_record() == 0);
}

static void testReplaceLastPlayerRecord() {
    resetDataDirectory();
    Player first = makePlayer("2026-08-01 10:00:00", 100);
    Player second = makePlayer("2026-08-02 10:00:00", 200);
    Player original_second = makePlayer("2026-08-02 10:00:00", 200);
    second.recommended_ratios = {0.4f, 0.5f, 0.6f};
    assert(save_player_data(&first) == 1);
    assert(save_player_data(&original_second) == 1);
    assert(replace_last_player_data(&second) == 1);

    Player loaded = {};
    assert(load_last_player_data(&loaded) == 1);
    assert(loaded.wave == 200);
    assert(fabs(loaded.recommended_ratios.leader - 0.4f) < 1e-6f);
    assert(fabs(loaded.recommended_ratios.town_archer - 0.5f) < 1e-6f);
    assert(fabs(loaded.recommended_ratios.castle - 0.6f) < 1e-6f);
}

static void testDeleteOneCustomHero() {
    resetDataDirectory();
    CustomHero heroes[3] = {};
    std::snprintf(heroes[0].name, sizeof(heroes[0].name), "%s", "Alpha");
    std::snprintf(heroes[1].name, sizeof(heroes[1].name), "%s", "Beta");
    std::snprintf(heroes[2].name, sizeof(heroes[2].name), "%s", "Gamma");
    heroes[0].target_ratio = heroes[1].target_ratio = heroes[2].target_ratio = 0.1f;
    heroes[0].level = 10;
    heroes[1].level = 20;
    heroes[2].level = 30;
    assert(save_custom_heroes(heroes, 3) == 1);
    assert(delete_custom_hero(1) == 1);

    CustomHero loaded[3] = {};
    assert(load_custom_heroes(loaded, 3) == 2);
    assert(std::strcmp(loaded[0].name, "Alpha") == 0);
    assert(std::strcmp(loaded[1].name, "Gamma") == 0);
    assert(loaded[1].level == 30);
}

static void testCustomHeroNameValidation() {
    resetDataDirectory();
    CustomHero numeric = {};
    std::snprintf(numeric.name, sizeof(numeric.name), "%s", "007");
    numeric.target_ratio = 0.1f;
    numeric.level = 10;
    assert(save_custom_hero(&numeric) == 1);

    CustomHero padded = {};
    std::snprintf(padded.name, sizeof(padded.name), "%s", "  Alpha  ");
    padded.target_ratio = 0.2f;
    padded.level = 20;
    assert(save_custom_hero(&padded) == 1);

    CustomHero loaded[3] = {};
    assert(load_custom_heroes(loaded, 3) == 2);
    assert(std::strcmp(loaded[0].name, "007") == 0);
    assert(std::strcmp(loaded[1].name, "Alpha") == 0);

    CustomHero invalid = {};
    std::snprintf(invalid.name, sizeof(invalid.name), "%s", "   ");
    invalid.target_ratio = 0.1f;
    invalid.level = 10;
    assert(save_custom_hero(&invalid) == 0);
    std::snprintf(invalid.name, sizeof(invalid.name), "%s", "Alpha,Beta");
    assert(save_custom_hero(&invalid) == 0);
}

static void testCorruptProgressRowsAreIgnored() {
    resetDataDirectory();
    FILE* file = std::fopen("data/history.csv", "w");
    assert(file != NULL);
    std::fprintf(file, "2026-08-01 10:00:00,100,200,300\n");
    std::fprintf(file, "2026-13-01 10:00:00,110,210,310\n");
    std::fprintf(file, "2026-08-02 99:99:99,120,220,320\n");
    std::fprintf(file, "2026-08-03 10:00:00,-1,230,330\n");
    assert(std::fclose(file) == 0);

    ProgressData history[8] = {};
    assert(read_progress_history("data/history.csv", history, 8) == 1);
    assert(get_progress_history_invalid_row_count() == 3);
    assert(history[0].wave == 100);
}

int main() {
    testDeleteLastPlayerRecord();
    testReplaceLastPlayerRecord();
    testDeleteOneCustomHero();
    testCustomHeroNameValidation();
    testCorruptProgressRowsAreIgnored();
    std::printf("file_operations_tests: all checks passed\n");
    return 0;
}