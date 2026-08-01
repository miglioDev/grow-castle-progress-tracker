#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "../include/ui.h"
#include "../include/player_data.h"
#include "../include/calculator.h"
#include "../include/file_operations.h"
#include "../include/input_safe.h"
#include "../include/player_stats.h"
#include "../include/upgrading.h"

//prototype for submenu function
void player_data_sub_menu(int sub_choice, Player *p);
void add_custom_hero_flow(void);
void show_custom_heroes(void);
void edit_recommended_ratios(Player *p);
void edit_custom_heroes(void);

//ratio stats variable
float colony_ratio, leader_ratio, town_archer_ratio, castle_ratio;
float *r_colony = &colony_ratio;
float *r_leader = &leader_ratio;
float *r_ta = &town_archer_ratio;
float *r_castle = &castle_ratio;

//colony_stats
double gold_from_infinity_town = 0;


int main()
{
    Player player = {0}; 
    player.recommended_ratios = (RecommendedRatios){
        DEFAULT_LEADER_RATIO,
        DEFAULT_TOWN_ARCHER_RATIO,
        DEFAULT_CASTLE_RATIO
    };
    if (load_last_player_data(&player)) {
    printf("[Loaded last saved player data: %s, wave=%d]\n", player.last_update, player.wave);
} else {
    printf("[No previous saved player data found]\n");
}

    int choice, sub_choice;

    // Main menu 
    while (1)
    {
        show_main_menu(); 
        
        if (!safe_input_int("Select an option: ", &choice, 1, 8)) {
            printf("Invalid input! Please enter a number between 1 and 8.\n");
            continue;
        }

        switch (choice)
        {
        case 1: // Player Data Management
            manage_player_data(); // show the submenu UI
            
            if (!safe_input_int("Enter your choice: ", &sub_choice, 1, 3)) {
                printf("Invalid input! Please enter a number between 1 and 3.\n");
                break;
            }

            // submenu fun
            player_data_sub_menu(sub_choice, &player);
            break;

        case 2:
            ratio_and_suggestion();
            analyze_player_data(&player,&leader_ratio,&colony_ratio,&town_archer_ratio,&castle_ratio);
            {
                int edit_choice = 0;
                if (safe_input_int("Edit recommended ratios or custom heroes? 1=Yes, 0=No: ", &edit_choice, 0, 1) && edit_choice == 1) {
                    edit_recommended_ratios(&player);
                    edit_custom_heroes();
                    analyze_player_data(&player,&leader_ratio,&colony_ratio,&town_archer_ratio,&castle_ratio);
                }
            }
            break;

        case 3:
            show_colony_stats();
            gold_from_infinity_town = colony_stats_calculation(&player);

                colony_ratio = (float)player.infinity_castle_level / player.wave;

            stats_print_infinite_town(&player,&colony_ratio,gold_from_infinity_town);

            break;

        case 4:
            show_progress_history();
            break;

        case 5:
            upgrading_cost();
            break;

        case 6:
            export_import_data();
            break;

        case 7:
            add_custom_hero_flow();
            break;

        case 8:
            printf("Exiting program... Goodbye!\n");
            return 0;

        default:
            printf("Invalid choice! Please try again.\n");
            break;
        }

        printf("\nPress ENTER to continue...");
        getchar();
    }

    return 0;
}


// -- Player Data Sub-Menu Section --
void add_custom_hero_flow(void)
{
    CustomHero hero;
    char name[64] = {0};
    float target_ratio = 0.0f;
    int level = 0;

    printf("\n=== Add Custom Hero ===\n");
    if (!safe_input_string("Hero name: ", name, sizeof(name))) {
        printf("Invalid hero name.\n");
        return;
    }

    if (!safe_input_float("Desired target ratio (e.g. 0.04): ", &target_ratio, 0.001f, 10.0f)) {
        printf("Invalid target ratio.\n");
        return;
    }

    if (!safe_input_int("Current level: ", &level, 1, INT_MAX)) {
        printf("Invalid level.\n");
        return;
    }

    memset(&hero, 0, sizeof(hero));
    snprintf(hero.name, sizeof(hero.name), "%s", name);
    hero.target_ratio = target_ratio;
    hero.level = level;

    if (!save_custom_hero(&hero)) {
        printf("Warning: Failed to save custom hero.\n");
    } else {
        printf("Custom hero saved successfully.\n");
    }
}

void edit_recommended_ratios(Player *p)
{
    float leader_ratio_value = p->recommended_ratios.leader;
    float town_archer_ratio_value = p->recommended_ratios.town_archer;
    float castle_ratio_value = p->recommended_ratios.castle;

    printf("\n=== Edit Recommended Ratios ===\n");
    if (!safe_input_float("Leader ratio: ", &leader_ratio_value, 0.0001f, 10.0f) ||
        !safe_input_float("Town Archer ratio: ", &town_archer_ratio_value, 0.0001f, 10.0f) ||
        !safe_input_float("Castle ratio: ", &castle_ratio_value, 0.0001f, 10.0f)) {
        printf("Invalid ratio. No changes were saved.\n");
        return;
    }

    p->recommended_ratios.leader = leader_ratio_value;
    p->recommended_ratios.town_archer = town_archer_ratio_value;
    p->recommended_ratios.castle = castle_ratio_value;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(p->last_update, sizeof(p->last_update), "%Y-%m-%d", t);

    if (save_player_data(p)) {
        printf("Recommended ratios saved successfully.\n");
    } else {
        printf("Warning: Failed to save recommended ratios.\n");
    }
}

void edit_custom_heroes(void)
{
    CustomHero heroes[32];
    int hero_count = load_custom_heroes(heroes, 32);
    if (hero_count <= 0) {
        return;
    }

    printf("\n=== Edit Custom Hero Ratios ===\n");
    for (int i = 0; i < hero_count; ++i) {
        printf("%d) %s | target ratio: %.4f | level: %d\n", i + 1, heroes[i].name, heroes[i].target_ratio, heroes[i].level);
    }

    int hero_choice = 0;
    if (!safe_input_int("Select a hero to edit (0 to skip): ", &hero_choice, 0, hero_count) || hero_choice == 0) {
        return;
    }

    CustomHero *hero = &heroes[hero_choice - 1];
    if (!safe_input_float("Target ratio: ", &hero->target_ratio, 0.0001f, 10.0f) ||
        !safe_input_int("Current level: ", &hero->level, 1, INT_MAX)) {
        printf("Invalid custom hero value. No changes were saved.\n");
        return;
    }

    if (save_custom_heroes(heroes, hero_count)) {
        printf("Custom hero saved successfully.\n");
    } else {
        printf("Warning: Failed to save custom heroes.\n");
    }
}

void show_custom_heroes(void)
{
    CustomHero heroes[32];
    int count = load_custom_heroes(heroes, 32);
    if (count <= 0) {
        printf("No custom heroes saved yet.\n");
        return;
    }

    printf("\n=== Custom Heroes ===\n");
    for (int i = 0; i < count; ++i) {
        printf("%d) %s | target ratio: %.4f | level: %d\n", i + 1, heroes[i].name, heroes[i].target_ratio, heroes[i].level);
    }
}

void player_data_sub_menu(int sub_choice, Player *p)
{
    if (sub_choice == 3)
    {
        printf("Returning to main menu...\n");
        return;
    }
    printf("\n=== Player Information ===\n");

    if (sub_choice == 1) // Enter Player Info
    {
        int valid;

        do 
        {
            printf("\nEnter your stats:\n");

            valid = safe_input_int("Wave: ", &p->wave, 1, INT_MAX);
            if (!valid) {
                printf("Error: invalid input. Please enter a positive number.\n");
                continue;
            }

            valid = safe_input_int("Infinity Castle Level: ", &p->infinity_castle_level, 1, INT_MAX);
            if (!valid) {
                printf("Error: invalid input. Please enter a positive number.\n");
                continue;
            }

            valid = safe_input_int("Leader Level: ", &p->leader_level, 1, INT_MAX);
            if (!valid) {
                printf("Error: invalid input. Please enter a positive number.\n");
                continue;
            }

            valid = safe_input_int("Town Archer level: ", &p->town_archer_level, 1, INT_MAX);
            if (!valid) {
                printf("Error: invalid input. Please enter a positive number.\n");
                continue;
            }

            valid = safe_input_int("Castle level: ", &p->castle_level, 1, INT_MAX);
            if (!valid) {
                printf("Error: invalid input. Please enter a positive number.\n");
                continue;
            }

            break;
        }
        while(1);

        // Register data && update date
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        strftime(p->last_update, sizeof(p->last_update), "%Y-%m-%d", t);

        printf("\nData saved successfully! Last update: %s\n", p->last_update);

        // NEW: Save data to file
        if (!save_player_data(p)) {
            printf("Warning: Failed to save player data to file.\n");
        }
    }
    else if (sub_choice == 2) // View Player Info
    {
        printf("\n=== Stored Player Data ===\n");
        printf("Wave: %d\n", p->wave);
        printf("Infinity Castle Level: %d\n", p->infinity_castle_level);
        printf("Leader Level: %d\n", p->leader_level);
        printf("Last update: %s\n", p->last_update);
        printf("Town Archer Level:%d\n", p->town_archer_level);
        printf("Castle Level: %d\n", p->castle_level);
        show_custom_heroes();
    }
}
