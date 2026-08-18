// include/file_operations.h
#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "player_data.h"
#include "graph.h"
#include "pace_analysis.h"

// Save current player record (append).  0 on error.
int save_player_data(const Player *p);

// Load the last saved player record into p. 1 ==  loaded, 0 if no data / error.
int load_last_player_data(Player *p);
int delete_last_player_record(void);

// prototype: fill an array of ProgressData with up to max_entries.
// returns number of entries read (0 = none / file missing)
int read_progress_history(const char *filename, ProgressData *out, int max_entries);

int save_custom_hero(const CustomHero *hero);
int load_custom_heroes(CustomHero *heroes, int max_heroes);
int save_custom_heroes(const CustomHero *heroes, int hero_count);
int delete_custom_hero(int hero_index);
int save_pace_data(const PaceInputs *inputs);
int load_pace_data(PaceInputs *inputs);

#ifdef __cplusplus
}
#endif

#endif



