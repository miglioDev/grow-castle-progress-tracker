#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/file_operations.h"

void save_player(Player *p, const char *filename)
{
    printf("[SAVE] Player data would be saved to '%s'\n", filename);
}

void load_player(Player *p, const char *filename)
{
    printf("[LOAD] Player data would be loaded from '%s'\n", filename);
}
