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

// Safe integer input with validation
static int safe_input_int(const char *prompt, int *result, int min_val, int max_val) {
    char buffer[64];
    char *endptr;
    long val;
    
    printf("%s", prompt);
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return 0;
    }
    
    errno = 0;
    val = strtol(buffer, &endptr, 10);
    
    if (errno != 0 || endptr == buffer || val < min_val || val > max_val) {
        return 0;
    }
    
    *result = (int)val;
    return 1;
}

//prototype for submenu function
void player_data_sub_menu(int sub_choice, Player *p);
void analyze_player_data(Player *p,float *r_hero,float *r_leader,float *r_colony,float *r_ta,float *r_castle);

//prototype for colony stats
double colony_stats_calculation(Player *p);
void stats_print_infinite_town(Player *p,float *r_colony,double gold_from_infinity_town);

//ratio stats variable
float hero_ratio, colony_ratio, leader_ratio, town_archer_ratio, castle_ratio;
float *r_hero = &hero_ratio; 
float *r_colony = &colony_ratio;
float *r_leader = &leader_ratio;
float *r_ta = &town_archer_ratio;
float *r_castle = &castle_ratio;

//colony_stats
double gold_from_infinity_town = 0;

//upgrading section
void upgrading_cost();


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

            // Call the submenu function passing player pointer
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

        // Pause for menu again
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
    else
    {
        printf("Error: invalid choice!\n");
    }
}

// -- Ratio & suggestion -- 
void analyze_player_data(Player *p,float *r_hero,float *r_leader,float *r_colony,float *r_ta,float *r_castle)
{
    long int target_ratio1,target_ratio2;
    //all ratio 
    (*r_hero) = (float)p->heroes_avg_level/ p->wave;
    (*r_leader) = (float)p->leader_level/ p->wave;
    (*r_colony) = (float)p->infinity_castle_level/ p->wave;
    (*r_ta) = (float)p->town_archer_level/p->wave;
    (*r_castle) = (float)p->castle_level/p->wave;

    // stats print order:
    printf("Wave:\t\tsubject:\tyour ratio:\t\trecommanded ratio:\tcorresponding level for target ratio:\n");
    
    target_ratio1 = p->wave*0.2;
    target_ratio2 = p->wave*0.4;
    printf("%d\t\t main hero\t%f\t\t ratio: 0.02-0.04\t%ld-%ld\n",p->wave,*r_hero,target_ratio1,target_ratio2);

    target_ratio1 = p->wave*0.3;
    printf("%d\t\t leader:\t%f\t\t ratio: 0.03\t\t%ld\n",p->wave,*r_leader,target_ratio1);

    printf("%d\t\t Infinite C.:\t%f\t\tAs high as possible!\t--\n",p->wave,*r_colony);

    target_ratio1 = p->wave*0.5;
    printf("%d\t\t Town Archer:\t%f\t\t ratio: 0.5\t\t%ld\n", p->wave, town_archer_ratio,target_ratio1);

    target_ratio1 = p->wave*0.25;
    printf("%d\t\t Castle:\t%f\t\t ratio: 0.25\t\t%ld\n", p->wave, castle_ratio,target_ratio1);

}

// Infinite town stats
double colony_stats_calculation(Player *p)
{
    double gold;

    gold = 4500 * p->infinity_castle_level + 10000;

    return gold;
}

// Infinite town output
void stats_print_infinite_town(Player *p,float *r_colony,double gold_from_infinity_town)
{
    double gold_with_xpbuff,gold_with_whip_and_xp;
    gold_with_xpbuff = gold_from_infinity_town * 1.20;
    gold_with_whip_and_xp = gold_from_infinity_town * 1.35;

    printf("Level of Infinite Town: %d\n",(p->infinity_castle_level));
    printf("Ratio with your wawe: %f\n",*r_colony);
    printf("Gold obtained, (Base Infinite Town): %.0f\n",gold_from_infinity_town);
    printf("Gold with lv 20 colony gold skill: %.0f\n",gold_with_xpbuff);
    printf("Gold with Whip + Skill: %.0f\n", gold_with_whip_and_xp);
}

// safe long long input with validation
static int safe_input_long_long(const char *prompt, long long *result, long long min_val, long long max_val) {
    char buffer[64];
    char *endptr;
    long long val;
    
    printf("%s", prompt);
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return 0;
    }
    
    errno = 0;
    val = strtoll(buffer, &endptr, 10);
    
    if (errno != 0 || endptr == buffer || val < min_val || val > max_val) {
        return 0;
    }
    
    *result = val;
    return 1;
}

//upgrading cost   
void upgrading_cost()
{
    how_to_use();
    int sub_menu, valid = 0;
    long long A, Z;
    unsigned long long cost = 0ULL;
    double display_cost;
    const char* unit;

    do
    {
        char buffer[64];
        printf("Option: ");
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            printf("Error reading input.\n");
            continue;
        }
        
        char *endptr;
        errno = 0;
        long val = strtol(buffer, &endptr, 10);
        
        if (errno != 0 || endptr == buffer || val < 1 || val > 4) {
            printf("Error: invalid choice! Please enter 1, 2, 3 or 4.\n");
            continue;
        }
        
        sub_menu = (int)val;
        valid = 1;
    }
    while(!valid);

    switch(sub_menu)
    {
        case 1: 
            do
            {
                if (!safe_input_long_long("Upgrading from level: ", &A, 1, LLONG_MAX)) {
                    printf("Error: invalid input.\n");
                    continue;
                }
                if (!safe_input_long_long("To level: ", &Z, 1, LLONG_MAX)) {
                    printf("Error: invalid input.\n");
                    continue;
                }

                valid = (A < Z && A > 0 && Z > 0);

                if(!valid) {
                    printf("\nError: 'From level' must be less than 'To level' and both must be positive!\n");
                }
            }
            while(!valid);

            cost = 1250ULL * (unsigned long long)(Z - A) * (unsigned long long)(Z + A - 1);
            
            break;
        
        case 2:
            do 
            {
                if (!safe_input_long_long("Upgrading from level: ", &A, 1, LLONG_MAX)) {
                    printf("Error: invalid input.\n");
                    continue;
                }
                if (!safe_input_long_long("To level: ", &Z, 1, LLONG_MAX)) {
                    printf("Error: invalid input.\n");
                    continue;
                }

                valid = (A < Z && A > 0 && Z > 0);

                if(!valid) {
                    printf("\nError: 'From level' must be less than 'To level' and both must be positive!\n");
                }
            }
            while(!valid);

            cost = 500ULL * (unsigned long long)(Z - A) * (unsigned long long)(Z + A);

            break;

        case 3: 
            do
            {
                if (!safe_input_long_long("Upgrading from level: ", &A, 1, LLONG_MAX)) {
                    printf("Error: invalid input.\n");
                    continue;
                }
                if (!safe_input_long_long("To level: ", &Z, 1, LLONG_MAX)) {
                    printf("Error: invalid input.\n");
                    continue;
                }

                valid = (A < Z && A > 0 && Z > 0);

                if(!valid) {
                    printf("\nError: 'From level' must be less than 'To level' and both must be positive!\n");
                }
            }
            while(!valid);

            long long current = A;

            if (current < 5000) {
            long long limit = (Z < 5000) ? Z : 5000;
           
            unsigned long long limit_sq = (unsigned long long)limit * limit;
            unsigned long long current_sq = (unsigned long long)current * current;
            cost += 1500ULL * (limit_sq - current_sq);
            current = limit;
            }
        
            if (current < 10000 && current < Z) {
                long long limit = (Z < 10000) ? Z : 10000;
                unsigned long long limit_sq = (unsigned long long)limit * limit;
                unsigned long long current_sq = (unsigned long long)current * current;
                cost += 2000ULL * (limit_sq - current_sq);
                current = limit;
            }
            
            if (current < Z) {
                unsigned long long Z_sq = (unsigned long long)Z * Z;
                unsigned long long current_sq = (unsigned long long)current * current;
                cost += 2500ULL * (Z_sq - current_sq);
            }
            
            break;

            case 4:
                return;
            break;
    }

    if (cost >= 1000000000000ULL) { //(10^12)
        display_cost = (double)cost / 1000000000000.0;
        unit = "trillion";
    } else if (cost >= 1000000000ULL) { //(10^9)
        display_cost = (double)cost / 1000000000.0;
        unit = "billion";
    } else if (cost >= 1000000ULL) { 
        display_cost = (double)cost / 1000000.0;
        unit = "million";
    } else if (cost >= 1000ULL) {
        display_cost = (double)cost / 1000.0;
        unit = "thousand";
    } else {
        display_cost = (double)cost;
        unit = ""; }

    printf("\nIt will cost you: %.2f %s gold\n",display_cost, unit);
}
