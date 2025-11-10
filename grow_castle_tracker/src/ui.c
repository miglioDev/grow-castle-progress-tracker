#include <stdio.h>
#include "../include/ui.h"


// -- MAIN MENU --
void show_main_menu()
{
    printf("\n==============================\n");
    printf(" Grow Castle Progress Tracker \n");
    printf("==============================\n");
    printf("1) Player Data\n");
    printf("2) Ratio & suggestion\n");
    printf("3) Colony Stats\n");
    printf("4) Progress History\n");
    printf("5) Export/Import Data\n");
    printf("6) Exit\n");
    printf("==============================\n");
}


// -- PLAYER DATA SUBMENU --
void manage_player_data()
{
    printf("\n=== Player Data Menu ===\n");
    printf("1) Enter Player Info\n");
    printf("2) View Player Info\n");
    printf("3) Back to Main Menu\n");
    printf("========================\n");
}

void ratio_and_suggestion()
{
    printf("\n== Welcome to ratio and suggestion ==\n");
    printf("In this section, we will show your ratio stats and recommended levels.\n");
}

void show_colony_stats()
{
    printf("\n== Welcome to colony stats ==\n");
}

// -- PLACEHOLDER MENUS (future) --

void show_progress_history()
{
    printf("\n[Progress History Placeholder]\n");
}

void export_import_data()
{
    printf("\n[Export/Import Placeholder]\n");
}