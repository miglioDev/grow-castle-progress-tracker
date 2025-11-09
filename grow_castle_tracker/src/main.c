#include <stdio.h>
#include <stdlib.h>
#include "../include/ui.h"
#include "../include/player_data.h"
#include "../include/calculator.h"
#include "../include/file_operations.h"

int main()
{
    int choice;

    while (1)
    {
        show_main_menu();
        printf("Select an option: ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            manage_player_data();
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

        printf("\nPress ENTER to continue...");
        getchar(); // consume leftover newline
        getchar();
    }

    return 0;
}
