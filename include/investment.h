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

typedef enum {
    GOLD_POWER_OK = 0,
    GOLD_POWER_INVALID_WAVE,
    GOLD_POWER_INVALID_PACE,
    GOLD_POWER_INVALID_TOTAL_GOLD,
    GOLD_POWER_INVALID_SEASON_INCOME,
    GOLD_POWER_INVALID_SAVED_GOLD,
    GOLD_POWER_INVALID_TARGET_GOLD,
    GOLD_POWER_CALCULATION_OVERFLOW
} GoldPowerStatus;

typedef struct {
    double wave;
    double waves_per_hour;
    double total_investment_gold;
    double gold_per_season;
    int has_saved_gold;
    double saved_gold;
    int has_target_gold;
    double target_gold;
} GoldPowerInput;

typedef struct {
    double current_power;
    double future_power_without_spending;
    double power_loss_per_season;
    double power_gain;
    double power_loss_percent;
    double power_gain_percent;
    int has_power_with_saved_gold;
    double power_with_saved_gold;
    int has_saved_gold_gap;
    double saved_gold_gap;
    int has_target_power;
    double target_power;
    int has_target_gold_gap;
    double target_gold_gap;
} GoldPowerMetrics;

InvestmentMetrics calculate_investment_metrics(UnitType unit_type, long long current_level,
    double target_ratio, long long current_wave, double pace_wph, double hours_in_period);
void calculate_investment_percentages(InvestmentMetrics *metrics, int count);
GoldPowerStatus calculate_gold_power_metrics(const GoldPowerInput *input, GoldPowerMetrics *metrics);

#ifdef __cplusplus
}
#endif

#endif