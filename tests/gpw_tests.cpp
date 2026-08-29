#include "gpw.h"

#include <cassert>
#include <cmath>
#include <cstdio>

static void testKnownProfitableSample()
{
    const double samples[] = {137970.9};
    GpwAnalysis analysis = {};
    const GpwCalculationError error = calculate_gpw_analysis(samples, 1, 262.46, 456.0, &analysis);

    assert(error == GPW_CALCULATION_OK);
    assert(fabs(analysis.gpw.avg - 525.68) < 0.01);
    assert(fabs(analysis.profit.avg - 69.68) < 0.01);
    assert(analysis.isProfitable);
    assert(analysis.reliabilityWarning);
}

static void testSampleRanges()
{
    const double samples[] = {100.0, 200.0, 300.0, 400.0, 500.0};
    GpwAnalysis analysis = {};

    assert(calculate_gpw_analysis(samples, 5, 100.0, 3.0, &analysis) == GPW_CALCULATION_OK);
    assert(analysis.sampleCount == 5);
    assert(fabs(analysis.gpw.min - 1.0) < 1e-6);
    assert(fabs(analysis.gpw.avg - 3.0) < 1e-6);
    assert(fabs(analysis.gpw.max - 5.0) < 1e-6);
    assert(fabs(analysis.profit.avg) < 1e-6);
    assert(!analysis.isProfitable);
    assert(!analysis.reliabilityWarning);
}

static void testValidation()
{
    const double negative_samples[] = {100.0, -1.0};
    GpwAnalysis analysis = {};

    assert(calculate_gpw_analysis(NULL, 0, 100.0, 456.0, &analysis) == GPW_CALCULATION_NO_SAMPLES);
    assert(calculate_gpw_analysis(negative_samples, 2, 100.0, 456.0, &analysis) == GPW_CALCULATION_NEGATIVE_SAMPLE);
}

int main()
{
    testKnownProfitableSample();
    testSampleRanges();
    testValidation();
    std::printf("gpw_tests: all checks passed\n");
    return 0;
}