#include "profit.h"

static double active_period(double period, double downtime)
{
    if (period <= 0.0) {
        return 0.0;
    }
    if (downtime <= 0.0) {
        return period;
    }
    return downtime < period ? period - downtime : 0.0;
}

double calculate_tab_profit(double rwph, double wave, double period_hours, double downtime_hours)
{
    return rwph > 0.0 && wave > 0.0
        ? rwph * TAB_GOLD_PER_WAVE * wave * active_period(period_hours, downtime_hours)
        : 0.0;
}

