#ifndef GRAPH_H
#define GRAPH_H

#ifdef __cplusplus
extern "C" {
#endif

// Minimal struct with the fields needed for progress history.
typedef struct {
    char date[32];
    long long wave;
    long long infinity_castle_level;
} ProgressData;

#ifdef __cplusplus
}
#endif

#endif // GRAPH_H
