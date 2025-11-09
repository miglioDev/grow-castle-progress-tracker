#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/player_data.h"
#include "../include/file_operations.h"

void dummy_player_function()
{
    Player p = {"ExamplePlayer", 10, 1.25};
    printf("Loaded player: %s (Level %d, Ratio %.2f)\n", p.name, p.level, p.ratio);
}
