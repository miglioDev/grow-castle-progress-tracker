#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/player_data.h"
#include "../include/file_operations.h"

#include <stdio.h>
#include "../include/player_data.h"

void dummy_player_function()
{
    Player p = {10, 5, 12, 40, "2025-11-09"};
    printf("Loaded player: wave=%d, infinity=%d, leader=%d, heroes_avg=%d\n",
           p.wave, p.infinity_castle_level, p.leader_level, p.hero_avg_level);
}