#include <stdio.h>
#include "../include/ui.h"
#include "../include/graph.h"
#include "../include/file_operations.h"


// -- MAIN MENU --
void show_main_menu()
{
    printf("\n==============================\n");
    printf(" Grow Castle Progress Tracker \n");
    printf("==============================\n");
    printf("1) Player Data\n");
    printf("2) Ratio & Suggestion\n");
    printf("3) Colony Stats\n");
    printf("4) Progress History\n");
    printf("5) Upgrading Cost\n");
    printf("6) Export/Import Data\n");
    printf("7) Exit\n");
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
    printf("In this section, you'll see ratio stats and recommended levels.\n");
}

void show_colony_stats()
{
    printf("\n== Welcome to colony stats ==\n");
}


void show_progress_history()
{
    // buffer for progress data 
    const int MAX_ENTRIES = 300;
    ProgressData data[MAX_ENTRIES];
    int count = read_progress_history("data/player_data.csv", data, MAX_ENTRIES);

    if (count <= 0) {
        printf("No progress data available yet. Insert player data to build history.\n");
        return;
    }

    // draw graph; pass 0 for auto-detect terminal width
    draw_progress_graph(data, count, 0);
}

//upgrading cost 6
void how_to_use()
{
    printf("In this section you can calculate:\n");
    printf("The gold required to reach your target level for Castle and Town Archers\n");
    printf("Choose an option to get started!\n\n");

    printf("1) Castle \n");
    printf("2) Town archers\n");
    printf("3) Back to Main Menu\n\n");

}

// Option 6 - Data Import / Export instructions
void export_import_data()
{
    printf("\n==============================\n");
    printf("    IMPORT / EXPORT DATA\n");
    printf("==============================\n\n");
    printf("Your progress is automatically saved in:\n");
    printf("  -> ./data/player_data.csv\n\n");

    printf("This file contains all your stored information,\n");
    printf("such as your Wave, Colony Infinity ratio, and Date.\n\n");

    printf(" EXPORT:\n");
    printf(" - To back up your progress, simply copy this file\n");
    printf("   to a safe location (e.g. USB drive, cloud storage).\n");
    printf(" - You can also rename it with a timestamp, e.g.:\n");
    printf("   player_data_backup_2025-11-11.csv\n\n");

    printf(" IMPORT:\n");
    printf(" - To restore your progress, place your backup CSV file\n");
    printf("   back into the './data' folder.\n");
    printf(" - Make sure it is named exactly 'player_data.csv'\n");
    printf("   before restarting the application.\n\n");

    printf("  NOTE:\n");
    printf(" This project uses plain CSV format for simplicity.\n\n");

    printf("==============================\n\n");
}


