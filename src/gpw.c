#include "gpw.h"

GpwCalculationError calculate_gpw_analysis(const double *gold_samples, int sample_count,
    double wave_reached, double cost_per_wave_unit, GpwAnalysis *result)
{
    if (!gold_samples || sample_count < 1 || !result) {
        return GPW_CALCULATION_NO_SAMPLES;
    }
    if (wave_reached <= 0.0) {
        return GPW_CALCULATION_INVALID_WAVE;
    }

    double total_gold = 0.0;
    double minimum_gold = gold_samples[0];
    double maximum_gold = gold_samples[0];
    for (int index = 0; index < sample_count; ++index) {
        const double gold = gold_samples[index];
        if (gold < 0.0) {
            return GPW_CALCULATION_NEGATIVE_SAMPLE;
        }
        total_gold += gold;
        if (gold < minimum_gold) minimum_gold = gold;
        if (gold > maximum_gold) maximum_gold = gold;
    }

    result->gpw.avg = (total_gold / sample_count) / wave_reached;
    result->gpw.max = maximum_gold / wave_reached;
    result->gpw.min = minimum_gold / wave_reached;
    result->profit.avg = result->gpw.avg - cost_per_wave_unit;
    result->profit.max = result->gpw.max - cost_per_wave_unit;
    result->profit.min = result->gpw.min - cost_per_wave_unit;
    result->sampleCount = sample_count;
    result->isProfitable = result->profit.avg > 0.0;
    result->reliabilityWarning = sample_count < 5;
    return GPW_CALCULATION_OK;
}