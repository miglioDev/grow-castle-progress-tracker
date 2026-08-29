#ifndef GPW_H
#define GPW_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double avg;
    double max;
    double min;
} GpwRange;

typedef struct {
    GpwRange gpw;
    GpwRange profit;
    int sampleCount;
    int isProfitable;
    int reliabilityWarning;
} GpwAnalysis;

typedef enum {
    GPW_CALCULATION_OK = 0,
    GPW_CALCULATION_NO_SAMPLES,
    GPW_CALCULATION_NEGATIVE_SAMPLE,
    GPW_CALCULATION_INVALID_WAVE
} GpwCalculationError;

GpwCalculationError calculate_gpw_analysis(const double *gold_samples, int sample_count,
    double wave_reached, double cost_per_wave_unit, GpwAnalysis *result);

#ifdef __cplusplus
}
#endif

#endif