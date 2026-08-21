#include "investment.h"

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