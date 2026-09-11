#ifndef PLAYER_DATA_H
#define PLAYER_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

// Player structure fields (must match EVERYWHERE in the code)
typedef struct {
    char name[64];
    float target_ratio;
    long long level;
} CustomHero;

typedef struct {
    float leader;
    float town_archer;
    float castle;
} RecommendedRatios;

#define DEFAULT_LEADER_RATIO 0.045f
#define DEFAULT_TOWN_ARCHER_RATIO 0.1f
#define DEFAULT_CASTLE_RATIO 0.05f

typedef struct {
    long long wave;
    long long infinity_castle_level;
    long long leader_level;
    long long town_archer_level;
    long long castle_level;
    char last_update[20];    // format "YYYY-MM-DD HH:MM:SS"
    RecommendedRatios recommended_ratios;
} Player;

#ifdef __cplusplus
}
#endif

#endif