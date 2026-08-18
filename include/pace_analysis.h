#ifndef PACE_ANALYSIS_H
#define PACE_ANALYSIS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef enum {
    PACE_CHRONO_NONE = 0,
    PACE_CHRONO_PASSIVE,
    PACE_CHRONO_YELLOW,
    PACE_CHRONO_BLUE
} PaceChrono;

typedef struct {
    int dhLevel;
    int goldenHorn;
    int horn;
    int gameSpeed;
    PaceChrono chrono;
    int ob;
    int mbf;
} PaceInputs;

typedef struct {
    int rwph;
    double wph;
    double wavesPerDay;
    double wavesPerSeason;
    int isValid;
    char validationMessage[256];
} PaceStats;

void resetPaceInputs(PaceInputs *inputs);
int validatePaceInputs(const PaceInputs *inputs, char *message, size_t messageSize);
void calculatePaceStats(const PaceInputs *inputs, PaceStats *stats);

#ifdef __cplusplus
}
#endif

#endif