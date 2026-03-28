#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "../include/upgrading.h"
#include "../include/input_safe.h"

void how_to_use()
{
    printf("In this section you can calculate:\n");
    printf("The gold required to reach your target level for Castle, Town Archers or Hero Leader and Tower\n");
    printf("Choose an option to get started!\n\n");

    printf("1) Castle \n");
    printf("2) Town archers\n");
    printf("3) Hero, Leader and Tower\n");
    printf("4) Back to Main Menu\n\n");
}

void upgrading_cost()
{
    how_to_use();
    int sub_menu, valid = 0;
    long long A, Z;
    unsigned long long cost = 0ULL;
    double display_cost;
    const char* unit;

    do
    {
        char buffer[64];
        printf("Option: ");
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            printf("Error reading input.\n");
            continue;
        }

        char *endptr;
        errno = 0;
        long val = strtol(buffer, &endptr, 10);

        if (errno != 0 || endptr == buffer || val < 1 || val > 4) {
            printf("Error: invalid choice! Please enter 1, 2, 3 or 4.\n");
            continue;
        }

        sub_menu = (int)val;
        valid = 1;
    }
    while(!valid);

    switch(sub_menu)
    {
        case 1:
            do
            {
                if (!safe_input_long_long("Upgrading from level: ", &A, 1, LLONG_MAX)) {
                    printf("Error: invalid input.\n");
                    continue;
                }
                if (!safe_input_long_long("To level: ", &Z, 1, LLONG_MAX)) {
                    printf("Error: invalid input.\n");
                    continue;
                }

                valid = (A < Z && A > 0 && Z > 0);

                if(!valid) {
                    printf("\nError: 'From level' must be less than 'To level' and both must be positive!\n");
                }
            }
            while(!valid);

            cost = 1250ULL * (unsigned long long)(Z - A) * (unsigned long long)(Z + A - 1);
            break;

        case 2:
            do
            {
                if (!safe_input_long_long("Upgrading from level: ", &A, 1, LLONG_MAX)) {
                    printf("Error: invalid input.\n");
                    continue;
                }
                if (!safe_input_long_long("To level: ", &Z, 1, LLONG_MAX)) {
                    printf("Error: invalid input.\n");
                    continue;
                }

                valid = (A < Z && A > 0 && Z > 0);

                if(!valid) {
                    printf("\nError: 'From level' must be less than 'To level' and both must be positive!\n");
                }
            }
            while(!valid);

            cost = 500ULL * (unsigned long long)(Z - A) * (unsigned long long)(Z + A);
            break;

        case 3:
            do
            {
                if (!safe_input_long_long("Upgrading from level: ", &A, 1, LLONG_MAX)) {
                    printf("Error: invalid input.\n");
                    continue;
                }
                if (!safe_input_long_long("To level: ", &Z, 1, LLONG_MAX)) {
                    printf("Error: invalid input.\n");
                    continue;
                }

                valid = (A < Z && A > 0 && Z > 0);

                if(!valid) {
                    printf("\nError: 'From level' must be less than 'To level' and both must be positive!\n");
                }
            }
            while(!valid);

            {
                long long current = A;

                if (current < 5000) {
                    long long limit = (Z < 5000) ? Z : 5000;
                    unsigned long long limit_sq = (unsigned long long)limit * limit;
                    unsigned long long current_sq = (unsigned long long)current * current;
                    cost += 1500ULL * (limit_sq - current_sq);
                    current = limit;
                }

                if (current < 10000 && current < Z) {
                    long long limit = (Z < 10000) ? Z : 10000;
                    unsigned long long limit_sq = (unsigned long long)limit * limit;
                    unsigned long long current_sq = (unsigned long long)current * current;
                    cost += 2000ULL * (limit_sq - current_sq);
                    current = limit;
                }

                if (current < Z) {
                    unsigned long long Z_sq = (unsigned long long)Z * Z;
                    unsigned long long current_sq = (unsigned long long)current * current;
                    cost += 2500ULL * (Z_sq - current_sq);
                }
            }
            break;

        case 4:
            return;
    }

    if (cost >= 1000000000000ULL) {
        display_cost = (double)cost / 1000000000000.0;
        unit = "trillion";
    } else if (cost >= 1000000000ULL) {
        display_cost = (double)cost / 1000000000.0;
        unit = "billion";
    } else if (cost >= 1000000ULL) {
        display_cost = (double)cost / 1000000.0;
        unit = "million";
    } else if (cost >= 1000ULL) {
        display_cost = (double)cost / 1000.0;
        unit = "thousand";
    } else {
        display_cost = (double)cost;
        unit = "";
    }

    printf("\nIt will cost you: %.2f %s gold\n", display_cost, unit);
}
