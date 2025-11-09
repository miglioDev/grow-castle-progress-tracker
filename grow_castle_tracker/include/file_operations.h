#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "player_data.h"

void save_player(Player *p, const char *filename);
void load_player(Player *p, const char *filename);

#endif
