#ifndef UPGRADING_H
#define UPGRADING_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	UNIT_TYPE_CASTLE = 0,
	UNIT_TYPE_TOWN_ARCHERS,
	UNIT_TYPE_INFINITY_CASTLE,
	UNIT_TYPE_LEADER,
	UNIT_TYPE_CUSTOM_HERO
} UnitType;

typedef struct {
	double quadratic;
	double linear;
	double constant;
} UnitCostParameters;

const UnitCostParameters *get_unit_cost_parameters(UnitType unit_type);
double cost_function(UnitType unit_type, double level);

#ifdef __cplusplus
}
#endif

#endif // UPGRADING_H
