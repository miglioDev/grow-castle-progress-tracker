#include "../include/player_stats.h"

void analyze_player_data(Player *p, float *r_leader, float *r_colony, float *r_ta, float *r_castle)
{
    (*r_leader) = (float)p->leader_level / p->wave;
    (*r_colony) = (float)p->infinity_castle_level / p->wave;
    (*r_ta) = (float)p->town_archer_level / p->wave;
    (*r_castle) = (float)p->castle_level / p->wave;
}

double colony_stats_calculation(Player *p)
{
    double gold;
    gold = 4500 * p->infinity_castle_level + 10000;
    return gold;
}

