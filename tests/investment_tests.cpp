#include "investment.h"

#include <cassert>
#include <cmath>
#include <cstdio>

static void testCostFunction() {
    assert(fabs(cost_function(UNIT_TYPE_CASTLE, 10.0) - 125000.0) < 1e-6);
    assert(fabs(cost_function(UNIT_TYPE_TOWN_ARCHERS, 10.0) - 50000.0) < 1e-6);
    assert(cost_function(UNIT_TYPE_CASTLE, 0.0) == 0.0);
}

static void testInvestmentPercentages() {
    InvestmentMetrics metrics[2] = {};
    metrics[0].investment_gold = 50000.0;
    metrics[1].investment_gold = 200000.0;

    calculate_investment_percentages(metrics, 2);

    assert(fabs(metrics[0].investment_percent - 0.2) < 1e-6);
    assert(fabs(metrics[1].investment_percent - 0.8) < 1e-6);
}

static void testCostToTargetNextPeriod() {
    InvestmentMetrics metrics = calculate_investment_metrics(
        UNIT_TYPE_TOWN_ARCHERS, 10, 0.2, 100, 10.0, 5.0);

    assert(fabs(metrics.investment_gold - 50000.0) < 1e-6);
    assert(fabs(metrics.cost_to_target_now - 150000.0) < 1e-6);
    assert(fabs(metrics.cost_to_target_next_period - 250000.0) < 1e-6);
}

int main() {
    testCostFunction();
    testInvestmentPercentages();
    testCostToTargetNextPeriod();
    std::printf("investment_tests: all checks passed\n");
    return 0;
}