#include <stddef.h>
#include "../include/upgrading.h"

static const UnitCostParameters UNIT_COST_MODELS[] = {
    {1250.0, 0.0, 0.0}, /* Castle: cumulative cost = 1250 * level^2. */
    {500.0, 0.0, 0.0},  /* Town Archers: derived from existing upgrade formula. */
    {1250.0, 0.0, 0.0}, /* Infinity Castle: uses Castle model until calibrated. */
    {2500.0, 0.0, 0.0}, /* Leader/custom heroes: highest existing hero tier. */
    {2500.0, 0.0, 0.0}
};

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
    return parameters->quadratic * level * level + parameters->linear * level + parameters->constant;
}


