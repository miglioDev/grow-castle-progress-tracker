#include "pace_analysis.h"

#include <math.h>
#include <string.h>

#define GS_NUMERATOR 100.0
#define FIXED_OVERHEAD 4.0
#define GH_MULTIPLIER (10.0 / 7.0)
#define H_MULTIPLIER (10.0 / 9.0)
#define OB_SKIP_FRACTION 0.20
#define MBF_SKIP_FRACTION 0.40
#define HOURS_PER_DAY 24.0
#define HOURS_PER_SEASON 120.0

static void setValidationMessage(char *message, size_t messageSize, const char *text) {
    if (message && messageSize > 0) {
        strncpy(message, text, messageSize - 1);
        message[messageSize - 1] = '\0';
    }
}

static double getChronoMultiplier(PaceChrono chrono) {
    switch (chrono) {
        case PACE_CHRONO_NONE: return 1.00;
        case PACE_CHRONO_PASSIVE: return 1.10;
        case PACE_CHRONO_YELLOW: return 1.14;
        case PACE_CHRONO_BLUE: return 1.20;
        default: return 0.0;
    }
}

void resetPaceInputs(PaceInputs *inputs) {
    if (!inputs) {
        return;
    }

    memset(inputs, 0, sizeof(*inputs));
    inputs->gameSpeed = 2;
    inputs->chrono = PACE_CHRONO_NONE;
}

int validatePaceInputs(const PaceInputs *inputs, char *message, size_t messageSize) {
    if (!inputs) {
        setValidationMessage(message, messageSize, "Invalid pace input state.");
        return 0;
    }

    if (inputs->dhLevel < 0 || inputs->dhLevel > 5) {
        setValidationMessage(message, messageSize, "DH level must be between 0 and 5.");
        return 0;
    }

    if (inputs->gameSpeed != 2 && inputs->gameSpeed != 3) {
        setValidationMessage(message, messageSize, "Game speed must be 2x or 3x.");
        return 0;
    }

    if (inputs->chrono < PACE_CHRONO_NONE || inputs->chrono > PACE_CHRONO_BLUE) {
        setValidationMessage(message, messageSize, "Invalid Chrono selection.");
        return 0;
    }

    if ((inputs->goldenHorn != 0 && inputs->goldenHorn != 1) ||
        (inputs->horn != 0 && inputs->horn != 1) ||
        (inputs->ob != 0 && inputs->ob != 1) ||
        (inputs->mbf != 0 && inputs->mbf != 1)) {
        setValidationMessage(message, messageSize, "Toggle values must be enabled or disabled.");
        return 0;
    }

    if (message && messageSize > 0) {
        message[0] = '\0';
    }
    return 1;
}

void calculatePaceStats(const PaceInputs *inputs, PaceStats *stats) {
    if (!inputs || !stats) {
        return;
    }

    memset(stats, 0, sizeof(*stats));
    if (!validatePaceInputs(inputs, stats->validationMessage, sizeof(stats->validationMessage))) {
        return;
    }

    double multiplier = getChronoMultiplier(inputs->chrono);
    if (inputs->goldenHorn) {
        multiplier *= GH_MULTIPLIER;
    }
    if (inputs->horn) {
        multiplier *= H_MULTIPLIER;
    }

    const double baseTimePerWave = (GS_NUMERATOR / (double)inputs->gameSpeed) / multiplier + FIXED_OVERHEAD;
    const double rwphBase = 3600.0 / baseTimePerWave;
    double skipFraction = (double)inputs->dhLevel;
    if (inputs->ob) {
        skipFraction += OB_SKIP_FRACTION;
    }
    if (inputs->mbf) {
        skipFraction += MBF_SKIP_FRACTION;
    }

    stats->rwph = (int)round(rwphBase);
    stats->wph = rwphBase * (1.0 + skipFraction);
    stats->wavesPerDay = stats->wph * HOURS_PER_DAY;
    stats->wavesPerSeason = stats->wph * HOURS_PER_SEASON;
    stats->isValid = 1;
}