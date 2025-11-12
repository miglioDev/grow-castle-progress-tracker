#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>  // for time(), localtime(), strftime()
#include "../include/ui.h"
#include "../include/player_data.h"
#include "../include/calculator.h"
#include "../include/file_operations.h"

// Forward declaration for submenu function
void player_data_sub_menu(int sub_choice, Player *p);
void analyze_player_data(Player *p,float *r_hero,float *r_leader,float *r_colony,float *r_ta,float *r_castle);

//prototype for colony stats
double colony_stats_calculation(Player *p);
void stats_print_infinite_town(Player *p,float *r_colony,double gold_from_infinity_town);

//ratio stats
float hero_ratio, colony_ratio, leader_ratio, town_archer_ratio, castle_ratio;
float *r_hero = &hero_ratio; 
float *r_colony = &colony_ratio;
float *r_leader = &leader_ratio;
float *r_ta = &town_archer_ratio;
float *r_castle = &castle_ratio;

//colony_stats
double gold_infinity_town;
double gold_from_infinity_town = 0;



int main()
{
    Player player = {0}; // create empty player struct
    if (load_last_player_data(&player)) {
    printf("[Loaded last saved player data: %s, wave=%d]\n", player.last_update, player.wave);
} else {
    printf("[No previous saved player data found]\n");
}

    int choice, sub_choice;

    // Infinite loop for main menu
    while (1)
    {
        show_main_menu(); // prints main menu
        printf("Select an option: ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1: // Player Data Management
            manage_player_data(); // show the submenu UI
            printf("Enter your choice: ");
            scanf("%d", &sub_choice);

            // Call the submenu function passing player pointer
            player_data_sub_menu(sub_choice, &player);
            break;

        case 2:
            ratio_and_suggestion();
            analyze_player_data(&player,&hero_ratio,&leader_ratio,&colony_ratio,&town_archer_ratio,&castle_ratio);
            break;

        case 3:
            show_colony_stats();
            gold_infinity_town = colony_stats_calculation(&player);
            gold_from_infinity_town = colony_stats_calculation(&player);

                // Recalculate colony ratio before printing
                colony_ratio = (float)player.infinity_castle_level / player.wave;

            stats_print_infinite_town(&player,&colony_ratio,gold_from_infinity_town);

            break;

        case 4:
            show_progress_history();
            break;

        case 5:
            export_import_data();
            break;

        case 6:
            printf("Exiting program... Goodbye!\n");
            return 0;

        default:
            printf("Invalid choice! Please try again.\n");
            break;
        }

        // Pause before showing the main menu again
        printf("\nPress ENTER to continue...");
        getchar(); // consume leftover newline
        getchar();
    }

    return 0;
}


// -- Player Data Sub-Menu Section --
void player_data_sub_menu(int sub_choice, Player *p)
{
    printf("\n=== Player Information ===\n");

    if (sub_choice == 1) // Enter Player Info
    {
        printf("\nEnter your stats:\n");

        printf("Wave: ");
        scanf("%d", &p->wave);

        printf("Infinity Castle Level: ");
        scanf("%d", &p->infinity_castle_level);

        printf("Leader Level: ");
        scanf("%d", &p->leader_level);

        printf("Heroes average level: ");
        scanf("%d", &p->heroes_avg_level);

        printf("Town Archer level: ");
        scanf("%d", &p->town_archer_level);

        printf("Castle level: ");
        scanf("%d", &p->castle_level);

        // Register update date
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        strftime(p->last_update, sizeof(p->last_update), "%Y-%m-%d", t);

        printf("\nData saved successfully! Last update: %s\n", p->last_update);

        // NEW: Save data to file
        save_player_data(p);
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
    else if (sub_choice == 3)
    {
        return;
    }
    else
    {
        printf("Error: invalid choice!\n");
    }
}

// -- Ratio & suggestion -- 
void analyze_player_data(Player *p,float *r_hero,float *r_leader,float *r_colony,float *r_ta,float *r_castle)
{
    //all ratio 
    (*r_hero) = (float)p->heroes_avg_level/ p->wave;
    (*r_leader) = (float)p->leader_level/ p->wave;
    (*r_colony) = (float)p->infinity_castle_level/ p->wave;
    (*r_ta) = (float)p->town_archer_level/p->wave;
    (*r_castle) = (float)p->castle_level/p->wave;

    // stats print 
    // your wave, subject,  your ratio, recommanded ratio, corrisponding level to the recommanded ratio?
    printf("Wave:\tsubject:\tyour ratio:\t\trecommanded ratio:\t\n");
    
    printf("%d\t main hero\t%f\t\t ratio: 0.02-0.04\n",p->wave,*r_hero);
    printf("%d\t leader:\t%f\t\t ratio: 0.03\n",p->wave,*r_leader);
    printf("%d\t Infinite C.:\t%f\t\tAs high as possible!\n",p->wave,*r_colony);
    printf("%d\t Town Archer:\t%f\t\t ratio: 0.5\n", p->wave, town_archer_ratio);
    printf("%d\t Castle:\t%f\t\t ratio: 0.25\n", p->wave, castle_ratio);

}

// Infinite town stats
double colony_stats_calculation(Player *p)
{
    double gold;
    if (p->infinity_castle_level <= 86662.0)
        gold = 6600.0 + 5400.0 * (p->infinity_castle_level);
    else {
        double d = p->infinity_castle_level - 86662.0;
        gold = 467981440.0 + 5400.0 * d + 0.00964 * d * d;
    }
    return gold;
}
// Infinite town output
void stats_print_infinite_town(Player *p,float *r_colony,double gold_from_infinity_town)
{
    double gold_with_buff;
    gold_with_buff = gold_from_infinity_town * 1.38;

    printf("Level of Infinite Town: %d\n",(p->infinity_castle_level));
    printf("Ratio with your wawe: %f\n",*r_colony);
    printf("Estimated gold obtained, (just from Infinite Town): %f\n",gold_from_infinity_town);
    printf("Gold with Whip (item) and maxed skill: %f\n",gold_with_buff);

    printf("\nNote: gold is estimated and may contain errors for now\n");
}