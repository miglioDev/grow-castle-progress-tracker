#ifndef GRAPH_H
#define GRAPH_H

// Minimal struct with the fields needed for progress graph
typedef struct {
    char date[32];   // "YYYY-MM-DD" or similar
    int wave;
    int infinity_castle_level;
} ProgressData;

// Draw a horizontal progress graph in the terminal.
// data: array of ProgressData (oldest first)
// count: number of entries
// terminal_width: pass 0 to auto-detect (preferred)
void draw_progress_graph(const ProgressData *data, int count, int terminal_width);

#endif // GRAPH_H
