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

//ratio stats variable
float hero_ratio, colony_ratio, leader_ratio, town_archer_ratio, castle_ratio;
float *r_hero = &hero_ratio;
float *r_colony = &colony_ratio;
float *r_leader = &leader_ratio;
float *r_ta = &town_archer_ratio;
float *r_castle = &castle_ratio;

//colony_stats
double gold_from_infinity_town = 0;


int main()
{
    Player player = {0}; 
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
        
        if (!safe_input_int("Select an option: ", &choice, 1, 7)) {
            printf("Invalid input! Please enter a number between 1 and 7.\n");
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
            analyze_player_data(&player,&hero_ratio,&leader_ratio,&colony_ratio,&town_archer_ratio,&castle_ratio);
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

            valid = safe_input_int("Heroes average level: ", &p->heroes_avg_level, 1, INT_MAX);
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
        printf("Heroes average level: %d\n", p->heroes_avg_level);
        printf("Last update: %s\n", p->last_update);
        printf("Town Archer Level:%d\n", p->town_archer_level);
        printf("Castle Level: %d\n", p->castle_level);
    }
}
