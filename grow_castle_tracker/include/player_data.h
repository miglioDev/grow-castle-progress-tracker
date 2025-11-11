#ifndef PLAYER_DATA_H
#define PLAYER_DATA_H

// Player structure: fields must match everywhere in the code
typedef struct {
    int wave;
    int infinity_castle_level;
    int leader_level;
    int heroes_avg_level;
    int town_archer_level;    
    int castle_level;        
    char last_update[20];    // "YYYY-MM-DD"
} Player;

#endif