#include "../include/player_stats.h"

void analyze_player_data(Player *p, float *r_leader, float *r_colony, float *r_ta, float *r_castle)
{
    if (!p || !r_leader || !r_colony || !r_ta || !r_castle || p->wave <= 0) {
        if (r_leader) *r_leader = 0.0f;
        if (r_colony) *r_colony = 0.0f;
        if (r_ta) *r_ta = 0.0f;
        if (r_castle) *r_castle = 0.0f;
        return;
    }
    (*r_leader) = (float)((double)p->leader_level / (double)p->wave);
    (*r_colony) = (float)((double)p->infinity_castle_level / (double)p->wave);
    (*r_ta) = (float)((double)p->town_archer_level / (double)p->wave);
    (*r_castle) = (float)((double)p->castle_level / (double)p->wave);
}

double colony_stats_calculation(Player *p)
{
    if (!p) {
        return 0.0;
    }
    double gold;
    gold = 4500.0 * (double)p->infinity_castle_level + 10000.0;
    return gold;
}

