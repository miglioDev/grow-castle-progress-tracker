#ifndef PLAYER_STATS_H
#define PLAYER_STATS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "player_data.h"

void analyze_player_data(Player *p, float *r_leader, float *r_colony, float *r_ta, float *r_castle);
double colony_stats_calculation(Player *p);

#ifdef __cplusplus
}
#endif

#endif // PLAYER_STATS_H
