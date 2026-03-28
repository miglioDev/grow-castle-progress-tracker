#include <stdio.h>
#include "../include/player_stats.h"

void analyze_player_data(Player *p, float *r_hero, float *r_leader, float *r_colony, float *r_ta, float *r_castle)
{
    long int target_ratio1 = 0, target_ratio2 = 0, gap = 0;
    (*r_hero) = (float)p->heroes_avg_level / p->wave;
    (*r_leader) = (float)p->leader_level / p->wave;
    (*r_colony) = (float)p->infinity_castle_level / p->wave;
    (*r_ta) = (float)p->town_archer_level / p->wave;
    (*r_castle) = (float)p->castle_level / p->wave;

    printf("Wave:\t\tsubject:\tyour ratio:\t\trecommanded ratio:\ttarget/gap\n");

    target_ratio1 = p->wave * 0.2;
    target_ratio2 = p->wave * 0.4;
    printf("%d\t\t Main hero\t%f\t\t ratio: 0.02-0.04\t%ld-%ld\n", p->wave, *r_hero, target_ratio1, target_ratio2);

    target_ratio1 = p->wave * 0.3;
    gap = p->leader_level - target_ratio1;
    printf("%d\t\t Leader:\t%f\t\t ratio: 0.03\t\t%ld\n", p->wave, *r_leader, gap);

    printf("%d\t\t Infinite C.:\t%f\t\tas high as possible!\t--\n", p->wave, *r_colony);

    target_ratio1 = p->wave * 0.5;
    gap = p->town_archer_level - target_ratio1;
    printf("%d\t\t Town Archer:\t%f\t\t ratio: 0.5\t\t%ld\n", p->wave, *r_ta, gap);

    target_ratio1 = p->wave * 0.25;
    gap = p->castle_level - target_ratio1;
    printf("%d\t\t Castle:\t%f\t\t ratio: 0.25\t\t%ld\n", p->wave, *r_castle, gap);
}

double colony_stats_calculation(Player *p)
{
    double gold;
    gold = 4500 * p->infinity_castle_level + 10000;
    return gold;
}

void stats_print_infinite_town(Player *p, float *r_colony, double gold_from_infinity_town)
{
    double gold_with_xpbuff, gold_with_whip_and_xp;
    gold_with_xpbuff = gold_from_infinity_town * 1.20;
    gold_with_whip_and_xp = gold_from_infinity_town * 1.35;

    printf("Level of Infinite Town: %d\n", (p->infinity_castle_level));
    printf("Ratio with your wawe: %f\n", *r_colony);
    printf("Gold obtained, (Base Infinite Town): %.0f\n", gold_from_infinity_town);
    printf("Gold with lv 20 colony gold skill: %.0f\n", gold_with_xpbuff);
    printf("Gold with Whip + Skill: %.0f\n", gold_with_whip_and_xp);
}
