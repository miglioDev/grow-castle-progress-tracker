#include "pace_analysis.h"

#include <cassert>
#include <cmath>
#include <cstdio>

static void testBasePace() {
    PaceInputs inputs = {0};
    resetPaceInputs(&inputs);

    PaceStats stats = {0};
    calculatePaceStats(&inputs, &stats);

    assert(stats.isValid == 1);
    assert(stats.rwph == 67);
    assert(fabs(stats.wph - 66.6666666667) < 1e-6);
    assert(fabs(stats.wavesPerDay - 1600.0) < 1e-6);
    assert(fabs(stats.wavesPerSeason - 8000.0) < 1e-6);
}

static void testAdditiveSkips() {
    PaceInputs inputs = {0};
    resetPaceInputs(&inputs);
    inputs.dhLevel = 2;
    inputs.ob = 1;
    inputs.mbf = 1;

    PaceStats stats = {0};
    calculatePaceStats(&inputs, &stats);

    assert(stats.isValid == 1);
    assert(fabs(stats.wph - 240.0) < 1e-6);
}

static void testSpeedBonuses() {
    PaceInputs inputs = {0};
    resetPaceInputs(&inputs);
    inputs.gameSpeed = 3;
    inputs.goldenHorn = 1;
    inputs.horn = 1;
    inputs.chrono = PACE_CHRONO_BLUE;

    PaceStats stats = {0};
    calculatePaceStats(&inputs, &stats);

    assert(stats.isValid == 1);
        assert(stats.rwph == 167);
        assert(fabs(stats.wph - 167.4418604651) < 1e-6);
}

static void testInvalidInputs() {
    PaceInputs inputs = {0};
    resetPaceInputs(&inputs);
    inputs.gameSpeed = 1;

    PaceStats stats = {0};
    calculatePaceStats(&inputs, &stats);

    assert(stats.isValid == 0);
    assert(stats.validationMessage[0] != '\0');
}

int main() {
    testBasePace();
    testAdditiveSkips();
    testSpeedBonuses();
    testInvalidInputs();
    std::printf("pace_analysis_tests: all checks passed\n");
    return 0;
}