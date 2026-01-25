// src/file_operations.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/file_operations.h"
#include "../include/graph.h" 


#define PLAYER_DATA_FILE "data/player_data.csv"

// Save one record (append). Format: YYYY-MM-DD,wave,infinity_castle_level,leader_level,heroes_avg_level,town_archer_level,castle_level
int save_player_data(const Player *p)
{
    FILE *f = fopen(PLAYER_DATA_FILE, "a");
    if (!f) {
        f = fopen("player_data.csv", "a");
        if (!f) return 0;
    }

    // Write CSV line with all fields (date + 6 ints)
    fprintf(f, "%s,%d,%d,%d,%d,%d,%d\n",
            p->last_update,
            p->wave,
            p->infinity_castle_level,
            p->leader_level,
            p->heroes_avg_level,
            p->town_archer_level,
            p->castle_level);

    fclose(f);
    return 1;
}

// Load last record 
int load_last_player_data(Player *p)
{
    FILE *f = fopen(PLAYER_DATA_FILE, "r");
    if (!f) {
        f = fopen("player_data.csv", "r");
        if (!f) return 0;
    }

    char line[512];
    char last_line[512];
    last_line[0] = '\0';

    while (fgets(line, sizeof(line), f) != NULL) {
        strncpy(last_line, line, sizeof(last_line) - 1);
        last_line[sizeof(last_line)-1] = '\0';
    }

    fclose(f);

    if (last_line[0] == '\0') return 0; // empty file

    // parse last_line: full format first (date,wave,inf,leader,heroes,town_archer,castle)
    char datebuf[32];
    int wave = 0, inf = 0, leader = 0, heroes = 0, town_archer = 0, castle = 0;
    int scanned = sscanf(last_line, "%31[^,],%d,%d,%d,%d,%d,%d",
                         datebuf, &wave, &inf, &leader, &heroes, &town_archer, &castle);

    if (scanned == 7) {
        strncpy(p->last_update, datebuf, sizeof(p->last_update)-1);
        p->last_update[sizeof(p->last_update)-1] = '\0';
        p->wave = wave;
        p->infinity_castle_level = inf;
        p->leader_level = leader;
        p->heroes_avg_level = heroes;
        p->town_archer_level = town_archer;
        p->castle_level = castle;
        return 1;
    }

    return 0;
}

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

        size_t L = strlen(line);
        while (L > 0 && (line[L-1] == '\n' || line[L-1] == '\r')) { line[--L] = '\0'; }

        if (L == 0) continue;

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
