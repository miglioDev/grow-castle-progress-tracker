#include "investment.h"

#include <math.h>

static int is_valid_positive(double value)
{
    return isfinite(value) && value > 0.0;
}

InvestmentMetrics calculate_investment_metrics(UnitType unit_type, long long current_level,
    double target_ratio, long long current_wave, double pace_wph, double hours_in_period)
{
    InvestmentMetrics metrics = {0};
    double target_level = target_ratio * (double)current_wave;
    double projected_wave = (double)current_wave + pace_wph * hours_in_period;
    double projected_target_level = target_ratio * projected_wave;
    double target_cost = cost_function(unit_type, target_level);

    metrics.investment_gold = cost_function(unit_type, (double)current_level);
    metrics.cost_to_target_now = target_cost - metrics.investment_gold;
    if (metrics.cost_to_target_now < 0.0) {
        metrics.cost_to_target_now = 0.0;
    }

    metrics.cost_to_target_next_period = cost_function(unit_type, projected_target_level) - target_cost;
    if (metrics.cost_to_target_next_period < 0.0) {
        metrics.cost_to_target_next_period = 0.0;
    }
    return metrics;
}

void calculate_investment_percentages(InvestmentMetrics *metrics, int count)
{
    if (!metrics || count <= 0) {
        return;
    }

    double total_investment = 0.0;
    for (int index = 0; index < count; ++index) {
        total_investment += metrics[index].investment_gold;
    }

    for (int index = 0; index < count; ++index) {
        metrics[index].investment_percent = total_investment > 0.0
            ? metrics[index].investment_gold / total_investment
            : 0.0;
    }
}

GoldPowerStatus calculate_gold_power_metrics(const GoldPowerInput *input, GoldPowerMetrics *metrics)
{
    const long double K = sqrtl(961.0L) / 20.0L; //Was this necessary? No. but it a cool way to write 1.55
    long double wave_squared_factor;
    long double current_power;
    long double future_power_without_spending;
    long double power_loss_per_season;
    long double power_gain;

    if (!input || !metrics || !is_valid_positive(input->wave)) {
        return GOLD_POWER_INVALID_WAVE;
    }
    if (!is_valid_positive(input->waves_per_hour)) {
        return GOLD_POWER_INVALID_PACE;
    }
    if (!is_valid_positive(input->total_investment_gold)) {
        return GOLD_POWER_INVALID_TOTAL_GOLD;
    }
    if (!is_valid_positive(input->gold_per_season)) {
        return GOLD_POWER_INVALID_SEASON_INCOME;
    }
    if (input->has_saved_gold && (!isfinite(input->saved_gold) || input->saved_gold < 0.0)) {
        return GOLD_POWER_INVALID_SAVED_GOLD;
    }
    if (input->has_target_gold && (!isfinite(input->target_gold) || input->target_gold < 0.0)) {
        return GOLD_POWER_INVALID_TARGET_GOLD;
    }

    *metrics = (GoldPowerMetrics){0};
    wave_squared_factor = input->wave * input->wave * K;
    current_power = (long double)input->total_investment_gold / wave_squared_factor;
    future_power_without_spending = (long double)input->total_investment_gold
        / (((long double)input->wave + input->waves_per_hour)
            * ((long double)input->wave + input->waves_per_hour) * K);
    power_loss_per_season = future_power_without_spending - current_power;
    power_gain = (long double)input->gold_per_season
        / (((long double)input->wave + input->waves_per_hour)
            * ((long double)input->wave + input->waves_per_hour) * K);
    metrics->current_power = (double)current_power;
    metrics->future_power_without_spending = (double)future_power_without_spending;
    metrics->power_loss_per_season = (double)power_loss_per_season;
    metrics->power_gain = (double)power_gain;

    if (!isfinite(wave_squared_factor) || !isfinite(metrics->current_power)
        || !isfinite(metrics->future_power_without_spending)
        || !isfinite(metrics->power_loss_per_season) || !isfinite(metrics->power_gain)
        ) {
        return GOLD_POWER_CALCULATION_OVERFLOW;
    }

    if (metrics->current_power != 0.0) {
        metrics->power_loss_percent = 1.0 - (metrics->future_power_without_spending / metrics->current_power);
        metrics->power_gain_percent = metrics->power_gain / metrics->current_power;
    }

    if (input->has_saved_gold) {
        metrics->has_power_with_saved_gold = 1;
        metrics->has_saved_gold_gap = 1;
        const long double power_with_saved_gold = ((long double)input->total_investment_gold + input->saved_gold)
            / wave_squared_factor;
        metrics->power_with_saved_gold = (double)power_with_saved_gold;
        metrics->saved_gold_gap = (double)((power_with_saved_gold - current_power) * wave_squared_factor);
        if (!isfinite(metrics->power_with_saved_gold) || !isfinite(metrics->saved_gold_gap)) {
            return GOLD_POWER_CALCULATION_OVERFLOW;
        }
    }
    if (input->has_target_gold) {
        metrics->has_target_power = 1;
        metrics->has_target_gold_gap = 1;
        const long double target_power = (long double)input->target_gold / wave_squared_factor;
        metrics->target_power = (double)target_power;
        metrics->target_gold_gap = (double)((target_power - current_power) * wave_squared_factor);
        if (!isfinite(metrics->target_power) || !isfinite(metrics->target_gold_gap)) {
            return GOLD_POWER_CALCULATION_OVERFLOW;
        }
    }
    return GOLD_POWER_OK;
}