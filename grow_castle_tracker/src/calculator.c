#include <stdio.h>
#include "../include/calculator.h"

double calculate_ratio(Player *p)
{
    if (!p) return 0.0;
    // Dummy formula
    return (p->level * 1.5);
}
