#include "file_operations.h"

#include <cassert>
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

int main() {
    testDeleteLastPlayerRecord();
    testDeleteOneCustomHero();
    std::printf("file_operations_tests: all checks passed\n");
    return 0;
}