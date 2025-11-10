#include <stdio.h>
#include "../include/ui.h"


// -- MAIN MENU --
void show_main_menu()
{
    printf("\n==============================\n");
    printf(" Grow Castle Progress Tracker \n");
    printf("==============================\n");
    printf("1) Player Data\n");
    printf("2) Efficiency Calculator\n");
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


// -- PLACEHOLDER MENUS (future) --
void show_efficiency_calculator()
{
    printf("\n[Efficiency Calculator Placeholder]\n");
}

void show_colony_stats()
{
    printf("\n[Colony Stats Placeholder]\n");
}

void show_progress_history()
{
    printf("\n[Progress History Placeholder]\n");
}

void export_import_data()
{
    printf("\n[Export/Import Placeholder]\n");
}