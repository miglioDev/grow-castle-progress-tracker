#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>  
#include "../include/ui.h"
#include "../include/player_data.h"
#include "../include/calculator.h"
#include "../include/file_operations.h"

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

        // Pause before showing the main menu again
        printf("\nPress ENTER to continue...");
        getchar(); 
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

            valid = (p->wave > 0 && p->infinity_castle_level > 0 && p->leader_level > 0 && p->heroes_avg_level > 0 && p->town_archer_level > 0 && p->castle_level > 0);

            if(!valid) {
                printf("\nError: All input values must be positive!\n"); }
        }
        while(!valid);

        // Register data && update date
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

    // stats print order
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

//upgrading cost   
void upgrading_cost()
{
    how_to_use();
    int sub_menu,valid;
    long long A,Z;
    long double cost = 0.0L;

    const long double K = 1e3L;
    const long double M = 1e6L;
    const long double B = 1e9L;

    do
    {
        printf("Option: ");
        if (scanf("%d",&sub_menu) != 1)
            scanf("%*s");

        valid = (sub_menu == 1 || sub_menu == 2 || sub_menu == 3); 
        if(!valid) {
            printf("Error: invalid choice!\n"); }
    }
    while(!valid);

    switch(sub_menu)
    {
        case 1: 
            do
            {
            printf("Upgrading from level: ");
            scanf("%lld", &A);
            printf("To level: ");
            scanf("%lld", &Z);

            valid = (A < Z && A > 0 && Z > 0);

            if(!valid) {
                printf("\nError: invalid choice!\n"); }
            }
            while(!valid);

            cost = 1250.0L * (long double)(Z-A) * (long double)(Z+A-1);
            
            break;
        
        case 2:
            do 
            {
            printf("Upgrading from level: ");
            scanf("%lld", &A);
            printf("To level: ");
            scanf("%lld", &Z);

            valid = (A < Z && A > 0 && Z > 0);

            if(!valid) {
                printf("\nError: invalid choice!\n"); }
            }
            while(!valid);

            cost = 500.0L * (long double)(Z - A) * (long double)(Z + A);

            break;

            case 3:
                return;
            break;
    }

    if(cost >= 1e9L) {
        printf("\nIt will cost you: %.0fM\n", (double)(cost / 1e6L));
    }
    else if(cost >= 1e6L) {
        printf("\nIt will cost you: %.0LfK\n", (double)(cost / 1e3L));
    }
    else {
        printf("\nIt will cost you: %.0Lf\n", (double)cost);
    }
}