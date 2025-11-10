#include <stdio.h>
#include <stdlib.h>
#include <time.h>  // for time(), localtime(), strftime()
#include "../include/ui.h"
#include "../include/player_data.h"
#include "../include/calculator.h"
#include "../include/file_operations.h"

// Forward declaration for submenu function
void player_data_sub_menu(int sub_choice, Player *p);

int main()
{
    Player player = {0}; // create empty player struct
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
            show_efficiency_calculator();
            break;

        case 3:
            show_colony_stats();
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
        scanf("%d", &p->hero_avg_level);

        // Register update date
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        strftime(p->last_update, sizeof(p->last_update), "%Y-%m-%d", t);

        printf("\nData saved successfully! Last update: %s\n", p->last_update);
    }
    else if (sub_choice == 2) // View Player Info
    {
        printf("\n=== Stored Player Data ===\n");
        printf("Wave: %d\n", p->wave);
        printf("Infinity Castle Level: %d\n", p->infinity_castle_level);
        printf("Leader Level: %d\n", p->leader_level);
        printf("Heroes average level: %d\n", p->hero_avg_level);
        printf("Last update: %s\n", p->last_update);
    }
    else
    {
        printf("Error: invalid choice!\n");
    }
}