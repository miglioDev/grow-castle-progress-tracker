#include "investment.h"

#include <cassert>
#include <cmath>
#include <cstdio>

static void testCostFunction() {
    assert(fabs(cost_function(UNIT_TYPE_CASTLE, 10.0) - 125000.0) < 1e-6);
    assert(fabs(cost_function(UNIT_TYPE_TOWN_ARCHERS, 10.0) - 50000.0) < 1e-6);
    assert(cost_function(UNIT_TYPE_CASTLE, 0.0) == 0.0);
}

static void testHeroTieredCostFunction() {
    const double first_tier_cost = 3000.0 * (5000.0 * 4999.0 / 2.0);
    const double second_tier_cost = 4000.0 * (10000.0 * 9999.0 / 2.0 - 5000.0 * 4999.0 / 2.0);

    assert(cost_function(UNIT_TYPE_LEADER, 1.0) == 0.0);
    assert(fabs(cost_function(UNIT_TYPE_LEADER, 5000.0) - first_tier_cost) < 1e-6);
    assert(fabs(cost_function(UNIT_TYPE_LEADER, 10000.0) - (first_tier_cost + second_tier_cost)) < 1e-6);
    assert(fabs(cost_function(UNIT_TYPE_LEADER, 10001.0)
        - (first_tier_cost + second_tier_cost + 50000000.0)) < 1e-6);
    assert(fabs(cost_function(UNIT_TYPE_CUSTOM_HERO, 5001.0)
        - (first_tier_cost + 20000000.0)) < 1e-6);
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
    testHeroTieredCostFunction();
    testInvestmentPercentages();
    testCostToTargetNextPeriod();
    std::printf("investment_tests: all checks passed\n");
    return 0;
}