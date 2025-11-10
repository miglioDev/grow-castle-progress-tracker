// include/file_operations.h
#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "player_data.h"

// Save current player record (append). Returns 1 on success, 0 on error.
int save_player_data(const Player *p);

// Load the last saved player record into p. Returns 1 if loaded, 0 if no data / error.
int load_last_player_data(Player *p);

// (future) you could add functions to load the whole history, export to CSV, etc.

#endif
