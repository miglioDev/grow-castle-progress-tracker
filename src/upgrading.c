#include <stddef.h>
#include "../include/upgrading.h"

static const UnitCostParameters UNIT_COST_MODELS[] = {
    {1250.0, 0.0, 0.0}, /* Castle: cumulative cost = 1250 * level^2. */
    {500.0, 0.0, 0.0},  /* Town Archers: derived from existing upgrade formula. */
    {1250.0, 0.0, 0.0}, /* Infinity Castle: uses Castle model until calibrated. */
    {0.0, 0.0, 0.0},    /* Leader/Hero/Tower: uses tiered cumulative cost. */
    {0.0, 0.0, 0.0}     /* Custom Hero: uses tiered cumulative cost. */
};

static double triangular_number(double level)
{
    return level * (level - 1.0) / 2.0;
}

static double hero_cumulative_cost(double level)
{
    double cost = 0.0;

    if (level > 0.0) {
        const double first_tier_end = level < 5000.0 ? level : 5000.0;
        cost += 3000.0 * triangular_number(first_tier_end);
    }
    if (level > 5000.0) {
        const double second_tier_end = level < 10000.0 ? level : 10000.0;
        cost += 4000.0 * (triangular_number(second_tier_end) - triangular_number(5000.0));
    }
    if (level > 10000.0) {
        cost += 5000.0 * (triangular_number(level) - triangular_number(10000.0));
    }

    return cost;
}

const UnitCostParameters *get_unit_cost_parameters(UnitType unit_type)
{
    if (unit_type < UNIT_TYPE_CASTLE || unit_type > UNIT_TYPE_CUSTOM_HERO) {
        return NULL;
    }
    return &UNIT_COST_MODELS[unit_type];
}

double cost_function(UnitType unit_type, double level)
{
    const UnitCostParameters *parameters = get_unit_cost_parameters(unit_type);
    if (!parameters || level <= 0.0) {
        return 0.0;
    }
    if (unit_type == UNIT_TYPE_LEADER || unit_type == UNIT_TYPE_CUSTOM_HERO) {
        return hero_cumulative_cost(level);
    }
    return parameters->quadratic * level * level + parameters->linear * level + parameters->constant;
}


