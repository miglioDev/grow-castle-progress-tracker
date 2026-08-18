#include <stddef.h>
#include "../include/upgrading.h"

static const UnitCostParameters UNIT_COST_MODELS[] = {
    {1250.0, 0.0, 0.0}, /* Castle: verified cumulative cost is 1250 * level^2. */
    {500.0, 0.0, 0.0},  /* Town Archers: derived from the existing upgrade formula. */
    {1250.0, 0.0, 0.0}, /* Infinity Castle currently uses the Castle model until calibrated data is supplied. */
    {2500.0, 0.0, 0.0}, /* Leader and custom heroes use the highest existing hero tier as a conservative model. */
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


