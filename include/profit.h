#ifndef PROFIT_H
#define PROFIT_H

#ifdef __cplusplus
extern "C" {
#endif

#define TAB_GOLD_PER_WAVE 449.0

double calculate_tab_profit(double rwph, double wave, double period_hours, double downtime_hours);

#ifdef __cplusplus
}
#endif

#endif