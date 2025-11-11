// src/file_operations.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/file_operations.h"
#include "../include/graph.h" // ensures ProgressData type

// File path - recommended: create a 'data' folder in the project root and keep file there.
// If you don't want folders, change to "player_data.csv".
#define PLAYER_DATA_FILE "data/player_data.csv"

// Save one record (append). Format: YYYY-MM-DD,wave,infinity_castle_level,leader_level,heroes_avg_level
int save_player_data(const Player *p)
{
    FILE *f = fopen(PLAYER_DATA_FILE, "a");
    if (!f) {
        // try fallback to current directory
        f = fopen("player_data.csv", "a");
        if (!f) return 0;
    }

    // Write CSV line
    fprintf(f, "%s,%d,%d,%d,%d\n",
            p->last_update,
            p->wave,
            p->infinity_castle_level,
            p->leader_level,
            p->heroes_avg_level);

    fclose(f);
    return 1;
}

// Load last record by reading file line-by-line and parsing the last valid line
int load_last_player_data(Player *p)
{
    FILE *f = fopen(PLAYER_DATA_FILE, "r");
    if (!f) {
        // fallback to current dir
        f = fopen("player_data.csv", "r");
        if (!f) return 0;
    }

    char line[512];
    char last_line[512];
    last_line[0] = '\0';

    while (fgets(line, sizeof(line), f) != NULL) {
        // store the line; at the end last_line will contain the last read line
        strncpy(last_line, line, sizeof(last_line) - 1);
        last_line[sizeof(last_line)-1] = '\0';
    }

    fclose(f);

    if (last_line[0] == '\0') return 0; // empty file

    // parse last_line: date,wave,inf,leader,heroes
    // last_update max 19 chars (YYYY-MM-DD)
    char datebuf[32];
    int wave, inf, leader, heroes;
    int scanned = sscanf(last_line, "%31[^,],%d,%d,%d,%d",
                         datebuf, &wave, &inf, &leader, &heroes);
    if (scanned == 5) {
        // copy into struct
        strncpy(p->last_update, datebuf, sizeof(p->last_update)-1);
        p->last_update[sizeof(p->last_update)-1] = '\0';
        p->wave = wave;
        p->infinity_castle_level = inf;
        p->leader_level = leader;
        p->heroes_avg_level = heroes;
        return 1;
    }
}

// read_progress_history: read CSV file and fill out[] with date,wave,infinity_castle_level
int read_progress_history(const char *filename, ProgressData *out, int max_entries)
{
    if (!out || max_entries <= 0) return 0;

    FILE *f = fopen(filename, "r");
    if (!f) {
        // try fallback in current directory
        f = fopen("player_data.csv", "r");
        if (!f) return 0;
    }

    char line[512];
    int count = 0;

    // read line by line, parse CSV
    while (fgets(line, sizeof(line), f) && count < max_entries) {
        // Trim line endings
        size_t L = strlen(line);
        while (L > 0 && (line[L-1] == '\n' || line[L-1] == '\r')) { line[--L] = '\0'; }

        // skip empty
        if (L == 0) continue;

        // parse: date,wave,inf,leader,heroes
        char datebuf[64];
        int wave = 0, inf = 0, leader = 0, heroes = 0;
        int scanned = sscanf(line, "%63[^,],%d,%d,%d,%d", datebuf, &wave, &inf, &leader, &heroes);
        if (scanned >= 3) {
            // keep date,wave,inf ; ignore other fields if missing
            strncpy(out[count].date, datebuf, sizeof(out[count].date)-1);
            out[count].date[sizeof(out[count].date)-1] = '\0';
            out[count].wave = wave;
            out[count].infinity_castle_level = inf;
            count++;
        }
    }

    fclose(f);
    return count;   
}
