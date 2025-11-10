#include <stdio.h>
#include "../include/calculator.h"

#include "../include/player_data.h"

// function for  test
double calculate_ratio(Player *p)
{
    if (p->hero_avg_level == 0)
        return 0;
    return (p->wave + p->infinity_castle_level + p->leader_level) / (double)p->hero_avg_level;
}