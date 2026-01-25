#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/graph.h"

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

// Ratio thresholds and maximum values for different scales
#define RATIO_MICRO_THRESHOLD  0.01f  // Below this: micro scale
#define RATIO_LOW_THRESHOLD    1.0f   // Below this: low scale, above: high scale
#define MAX_RATIO_HIGH         20.0f  // Maximum for high ratio scale
#define MAX_RATIO_LOW          1.0f   // Maximum for low ratio scale
#define MAX_RATIO_MICRO        0.01f  // Maximum for micro ratio scale

#define DEFAULT_TERM_WIDTH 80

// get terminal width from environment COLUMNS
static int detect_terminal_width(void) {
    char *cols = getenv("COLUMNS");
    if (cols) {
        int w = atoi(cols);
        if (w > 40) return w;
    }
    return DEFAULT_TERM_WIDTH;
}

// Determine which scale 
static int get_ratio_scale(float ratio) {
    if (ratio < RATIO_MICRO_THRESHOLD) return 0; // Micro scale
    if (ratio < RATIO_LOW_THRESHOLD) return 1;   // Low scale
    return 2;                                    // High scale
}

// Calculate bar length based on ratio and scale type
static int calculate_bar_length(float ratio, int scale_type, int graph_area) {
    float display_ratio = ratio;
    int capped = 0;
    
    switch (scale_type) {
        case 0: // Micro scale: 0.0 to 0.01
            if (ratio > MAX_RATIO_MICRO) {
                display_ratio = MAX_RATIO_MICRO;
                capped = 1;
            }
            return (int)((display_ratio / MAX_RATIO_MICRO) * (float)graph_area + 0.5f);
            
        case 1: // Low scale: 0.01 to 1.0
            if (ratio > MAX_RATIO_LOW) {
                display_ratio = MAX_RATIO_LOW;
                capped = 1;
            }
            return (int)((display_ratio / MAX_RATIO_LOW) * (float)graph_area + 0.5f);
            
        case 2: // High scale: 1.0 to 20.0
            if (ratio > MAX_RATIO_HIGH) {
                display_ratio = MAX_RATIO_HIGH;
                capped = 1;
            }
            return (int)(((display_ratio - RATIO_LOW_THRESHOLD) / 
                         (MAX_RATIO_HIGH - RATIO_LOW_THRESHOLD)) * (float)graph_area + 0.5f);
            
        default:
            return 0;
    }
}


// [date] [wave] | ###...  ratio (numeric)
void draw_progress_graph(const ProgressData *data, int count, int terminal_width)
{
    if (!data || count <= 0) {
        printf("No progress data to display.\n");
        return;
    }

    if (terminal_width <= 0) terminal_width = detect_terminal_width();

    // Layout constants
    const int date_col = 12;         // width for date string
    const int wave_col = 8;          // width for wave column
    const int sep_width = 3;         // " | " spacing
    const int right_label = 12;      // space for numeric ratio and extra chars
    int graph_area = terminal_width - (date_col + wave_col + sep_width + right_label);

    if (graph_area < 10) graph_area = 10; // minimal width

    printf("\n=== Progress History (oldest to newest) ===\n");
    printf("Date         Wave     | Ratio graph (IC / Wave)\n");
    printf("-------------------------------------------------------------\n");

    for (int i = 0; i < count; ++i) {
        const ProgressData *d = &data[i];

        float ratio = 0.0f;
        if (d->wave > 0) ratio = (float)d->infinity_castle_level / (float)d->wave;

        // Determine scale type and calculate bar length
        int scale_type = get_ratio_scale(ratio);
        int capped = 0;
        float display_ratio = ratio;
        
        // Check for capping
        switch (scale_type) {
            case 0: if (ratio > MAX_RATIO_MICRO) { display_ratio = MAX_RATIO_MICRO; capped = 1; } break;
            case 1: if (ratio > MAX_RATIO_LOW) { display_ratio = MAX_RATIO_LOW; capped = 1; } break;
            case 2: if (ratio > MAX_RATIO_HIGH) { display_ratio = MAX_RATIO_HIGH; capped = 1; } break;
        }
        
        int bar_len = calculate_bar_length(ratio, scale_type, graph_area);
        if (bar_len < 0) bar_len = 0;
        if (bar_len > graph_area) bar_len = graph_area;

        char date_buf[32];
        strncpy(date_buf, d->date, sizeof(date_buf) - 1);
        date_buf[sizeof(date_buf) - 1] = '\0';

        printf("%-12s %6d | ", date_buf, d->wave);

        for (int b = 0; b < bar_len; ++b) printf("#");

        for (int b = bar_len; b < graph_area; ++b) putchar(' ');

        if (capped) {
            printf(" %6.2f >", ratio);
        } else if (scale_type == 0) {
            // Micro 
            printf(" %8.4f", ratio);
        } else if (scale_type == 1) {
            // Low scale: show standard precision
            printf(" %8.4f", ratio);
        } else {
            // High scale: show standard precision
            printf(" %8.4f", ratio);
        }

        putchar('\n');
    }

    printf("-------------------------------------------------------------\n");
    printf("Scaling explanation:\n");
    printf("  Micro: 0.0000 to 0.0100 - full bar represents 0.01\n");
    printf("  Low:   0.0100 to 1.0000 - full bar represents 1.00\n");
    printf("  High:  1.0000 to 20.000 - full bar represents 20.00\n");
    printf("  Values exceeding scale maximum shown with '>' marker\n\n");
}