#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/graph.h"

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

// Default maximum ratio to plot. Ratios above this are capped and shown with a '>' marker.
#define MAX_RATIO 20.0f

// Default graph width if we cannot detect terminal width
#define DEFAULT_TERM_WIDTH 80

// Try to get terminal width from environment COLUMNS, fallback to DEFAULT_TERM_WIDTH.
// This is simple and cross-platform friendly (works in many shells). If you run this
// in environments that set `COLUMNS`, it will adapt.
static int detect_terminal_width(void) {
    char *cols = getenv("COLUMNS");
    if (cols) {
        int w = atoi(cols);
        if (w > 40) return w;
    }
    return DEFAULT_TERM_WIDTH;
}

// draw_progress_graph: prints for each entry a single line with:
// [date] [wave] | ██████...  ratio (numeric)
// Oldest entries appear first (top -> newest bottom).
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

    // Find max ratio among data (optional) or rely on MAX_RATIO
    // We will scale against MAX_RATIO so that ratio values are always comparable.
    printf("\n=== Progress History (oldest to newest) ===\n");
    printf("Date         Wave     | Ratio graph (IC / Wave)\n");
    printf("-------------------------------------------------------------\n");

    for (int i = 0; i < count; ++i) {
        const ProgressData *d = &data[i];

        float ratio = 0.0f;
        if (d->wave > 0) ratio = (float)d->infinity_castle_level / (float)d->wave;

        // clamp and compute bar length
        int capped = 0;
        float display_ratio = ratio;
        if (ratio > MAX_RATIO) {
            display_ratio = MAX_RATIO;
            capped = 1;
        }

        int bar_len = (int)((display_ratio / MAX_RATIO) * (float)graph_area + 0.5f);
        if (bar_len < 0) bar_len = 0;
        if (bar_len > graph_area) bar_len = graph_area;

        // Print date (left aligned), wave (right aligned)
        char date_buf[32];
        strncpy(date_buf, d->date, sizeof(date_buf) - 1);
        date_buf[sizeof(date_buf) - 1] = '\0';

        printf("%-12s %6d | ", date_buf, d->wave);

        // Print bar
        for (int b = 0; b < bar_len; ++b) printf("#");

        // Fill remainder with spaces to keep columns aligned
        for (int b = bar_len; b < graph_area; ++b) putchar(' ');

        // Print ratio numeric (2 decimal places). If capped, add '>' to indicate overflow.
        if (capped) {
            printf(" %6.2f >", ratio);
        } else {
            printf(" %8.4f", ratio);
        }

        putchar('\n');
    }

    printf("-------------------------------------------------------------\n");
    printf("Notes: bar length scaled to ratio in range [0 .. %.0f]. Values greater than\n", MAX_RATIO);
    printf("       shown with '>' marker on the right. More data -> better trend view.\n\n");
}
