// src/file_operations.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif
#include "../include/file_operations.h"
#include "../include/graph.h" 


#define PLAYER_DATA_FILE "data/player_data.csv"
#define DATA_DIR "data"


static int ensure_data_dir() {
    int ret = mkdir(DATA_DIR, 0755);
    // Success = dir created or already exists (errno==EEXIST)
    return (ret == 0 || errno == EEXIST) ? 1 : 0;
}

// Save one record (append). Format: YYYY-MM-DD,wave,infinity_castle_level,leader_level,heroes_avg_level,town_archer_level,castle_level
int save_player_data(const Player *p)
{
    if (!p) return 0;
    
    if (!ensure_data_dir()) {
        fprintf(stderr, "Error: Could not create or access 'data' directory.\n");
        return 0;
    }

    FILE *f = fopen(PLAYER_DATA_FILE, "a");
    if (!f) {
        fprintf(stderr, "Error: Could not open '%s' for writing.\n", PLAYER_DATA_FILE);
        return 0;
    }

    // Write CSV line with all fields (date + 6 ints)
    int ret = fprintf(f, "%s,%d,%d,%d,%d,%d,%d\n",
            p->last_update,
            p->wave,
            p->infinity_castle_level,
            p->leader_level,
            p->heroes_avg_level,
            p->town_archer_level,
            p->castle_level);

    if (ret < 0) {
        fprintf(stderr, "Error: Failed to write to '%s'.\n", PLAYER_DATA_FILE);
        fclose(f);
        return 0;
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "Error: Failed to close '%s'.\n", PLAYER_DATA_FILE);
        return 0;
    }
    
    return 1;
}

// Load last record 
// 0 for file missing/empty
int load_last_player_data(Player *p)
{
    if (!p) return 0;
    
    FILE *f = fopen(PLAYER_DATA_FILE, "r");
    if (!f) {
        return 0;
    }

    char line[512];
    int found = 0;
    // temp storage  (last valid parsed record)
    char last_datebuf[32] = "";
    int last_wave = 0, last_inf = 0, last_leader = 0, last_heroes = 0, last_town_archer = 0, last_castle = 0;
    int lineno = 0;

    while (fgets(line, sizeof(line), f) != NULL) {
        lineno++;
        size_t L = strlen(line);
        while (L > 0 && (line[L-1] == '\n' || line[L-1] == '\r')) { line[--L] = '\0'; }

        if (L == 0) continue; 

        char datebuf[32];
        int wave = 0, inf = 0, leader = 0, heroes = 0, town_archer = 0, castle = 0;
        int scanned = sscanf(line, "%31[^,],%d,%d,%d,%d,%d,%d",
                             datebuf, &wave, &inf, &leader, &heroes, &town_archer, &castle);

        if (scanned == 7) {
            // keep it as the last valid record
            strncpy(last_datebuf, datebuf, sizeof(last_datebuf)-1);
            last_datebuf[sizeof(last_datebuf)-1] = '\0';
            last_wave = wave;
            last_inf = inf;
            last_leader = leader;
            last_heroes = heroes;
            last_town_archer = town_archer;
            last_castle = castle;
            found = 1;
        } else {
            fprintf(stderr, "Nota: riga %d ignorata (corrotta o formato sbagliato): %s\n", lineno, line);
        }
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "Warning: Could not properly close '%s'.\n", PLAYER_DATA_FILE);
    }

    if (!found) return 0; // no valid records 

    // load Player struct with last valid record
    strncpy(p->last_update, last_datebuf, sizeof(p->last_update)-1);
    p->last_update[sizeof(p->last_update)-1] = '\0';
    p->wave = last_wave;
    p->infinity_castle_level = last_inf;
    p->leader_level = last_leader;
    p->heroes_avg_level = last_heroes;
    p->town_archer_level = last_town_archer;
    p->castle_level = last_castle;
    return 1;
}

int read_progress_history(const char *filename, ProgressData *out, int max_entries)
{
    if (!out || max_entries <= 0 || !filename) return 0;

    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Warning: Could not open '%s' for reading.\n", filename);
        return 0;
    }

    char line[512];
    int count = 0;

    // read line by line, parse CSV
    while (fgets(line, sizeof(line), f) && count < max_entries) {

        size_t L = strlen(line);
        while (L > 0 && (line[L-1] == '\n' || line[L-1] == '\r')) { line[--L] = '\0'; }

        if (L == 0) continue;

        static int lineno = 0;
        lineno++;

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
        } else {
            fprintf(stderr, "Nota: riga %d ignorata (corrotta o formato sbagliato): %s\n", lineno, line);
        }
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "Warning: Could not properly close '%s'.\n", filename);
    }
    
    return count;   
}
