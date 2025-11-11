#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/player_data.h"
#include "../include/file_operations.h"

void dummy_player_function()
{
    Player p = {10, 5, 12, 40, 25, 30, "2025-11-09"};  
    // wave, infinity_castle, leader, heroes_avg, town_archer, castle, last_update

    printf("Loaded player: wave=%d, infinity=%d, leader=%d, heroes_avg=%d, town_archer=%d, castle=%d\n",
           p.wave,
           p.infinity_castle_level,
           p.leader_level,
           p.heroes_avg_level,
           p.town_archer_level,
           p.castle_level);
}
