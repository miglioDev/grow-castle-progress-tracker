// include/file_operations.h
#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "player_data.h"
#include "graph.h"

// Save current player record (append).  0 on error.
int save_player_data(const Player *p);

// Load the last saved player record into p. 1 ==  loaded, 0 if no data / error.
int load_last_player_data(Player *p);

// prototype: fill an array of ProgressData with up to max_entries.
// returns number of entries read (0 = none / file missing)
int read_progress_history(const char *filename, ProgressData *out, int max_entries);


#endif



