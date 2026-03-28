#ifndef PLAYER_STATS_H
#define PLAYER_STATS_H

#include "player_data.h"

void analyze_player_data(Player *p, float *r_hero, float *r_leader, float *r_colony, float *r_ta, float *r_castle);
double colony_stats_calculation(Player *p);
void stats_print_infinite_town(Player *p, float *r_colony, double gold_from_infinity_town);

#endif // PLAYER_STATS_H
