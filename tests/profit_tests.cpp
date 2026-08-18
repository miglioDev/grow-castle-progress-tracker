#include "profit.h"

#include <cassert>
#include <cmath>
#include <cstdio>

static void testProfitWithoutDowntime()
{
    assert(fabs(calculate_tab_profit(100.0, 50.0, 2.0, 0.0) - 4490000.0) < 1e-6);
}

static void testProfitWithFullDowntime()
{
    assert(calculate_tab_profit(100.0, 50.0, 2.0, 2.0) == 0.0);
}

static void testProfitUsesSeparateUnits()
{
    const double tab = calculate_tab_profit(10.0, 10.0, 24.0, 12.0);

    assert(fabs(tab - 538800.0) < 1e-6);
}

int main()
{
    testProfitWithoutDowntime();
    testProfitWithFullDowntime();
    testProfitUsesSeparateUnits();
    std::printf("profit_tests: all checks passed\n");
    return 0;
}