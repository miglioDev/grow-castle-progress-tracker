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
    printf("7) Add Custom Hero\n");
    printf("8) Exit\n");
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

    // draw graph; pass 0 for auto-detect (terminal width)
    draw_progress_graph(data, count, 0);
}

//upgrading cost how to use on file upgrading.c

// Option 6 - Data Import / Export instructions
void export_import_data()
{
    printf("\n==============================\n");
    printf("    IMPORT / EXPORT DATA\n");
    printf("==============================\n\n");
    printf("Your data is stored locally in the ./data folder:\n");
    printf("  -> player_data.csv   (player progress and history)\n");
    printf("  -> custom_heroes.csv (custom hero names, ratios, and levels)\n\n");

    printf(" EXPORT:\n");
    printf(" - Copy both CSV files to a safe location to back up all data.\n");
    printf(" IMPORT:\n");
    printf(" - Copy both files back into ./data, keep their exact names,\n");
    printf("   then restart the application.\n\n");

    printf("If a file does not exist yet, that category has no saved data.\n\n");

    printf("==============================\n\n");
}


