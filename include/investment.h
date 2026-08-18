#ifndef INVESTMENT_H
#define INVESTMENT_H

#include "upgrading.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double investment_gold;
    double investment_percent;
    double cost_to_target_now;
    double cost_to_target_next_period;
} InvestmentMetrics;

InvestmentMetrics calculate_investment_metrics(UnitType unit_type, int current_level,
    double target_ratio, int current_wave, double pace_wph, double hours_in_period);
void calculate_investment_percentages(InvestmentMetrics *metrics, int count);

#ifdef __cplusplus
}
#endif

#endif