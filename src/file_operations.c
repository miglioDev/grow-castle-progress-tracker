#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif
#include "../include/file_operations.h"


#define PLAYER_DATA_FILE "data/player_data.csv"
#define CUSTOM_HEROES_FILE "data/custom_heroes.csv"
#define PACE_DATA_FILE "data/pace_data.csv"
#define DATA_DIR "data"


static int ensure_data_dir() {
    int ret = mkdir(DATA_DIR, 0755);
    // Success = dir created or already exists (errno==EEXIST)
    return (ret == 0 || errno == EEXIST) ? 1 : 0;
}

// Save one record (append). The final three fields store recommended ratios.
int save_player_data(const Player *p)
{
    if (!p) return 0;
    
    if (!ensure_data_dir()) {
        fprintf(stderr, "Error: Could not create or access 'data' directory.\n");
        return 0;
    }

    FILE *f = fopen(PLAYER_DATA_FILE, "a");
    if (!f) {
        fprintf(stderr, "Error: Could not open '%s' for writing.\n", PLAYER_DATA_FILE);
        return 0;
    }

        int ret = fprintf(f, "%s,%lld,%lld,%lld,%lld,%lld,%.6f,%.6f,%.6f\n",
            p->last_update,
            p->wave,
            p->infinity_castle_level,
            p->leader_level,
            p->town_archer_level,
            p->castle_level,
            p->recommended_ratios.leader,
            p->recommended_ratios.town_archer,
            p->recommended_ratios.castle);

    if (ret < 0) {
        fprintf(stderr, "Error: Failed to write to '%s'.\n", PLAYER_DATA_FILE);
        fclose(f);
        return 0;
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "Error: Failed to close '%s'.\n", PLAYER_DATA_FILE);
        return 0;
    }
    
    return 1;
}

// Returns 1 if loaded, 0 if file is missing/empty.
int load_last_player_data(Player *p)
{
    if (!p) return 0;
    
    FILE *f = fopen(PLAYER_DATA_FILE, "r");
    if (!f) {
        return 0;
    }

    char line[512];
    int found = 0;
    char last_datebuf[32] = "";
    long long last_wave = 0, last_inf = 0, last_leader = 0, last_town_archer = 0, last_castle = 0;
    RecommendedRatios last_ratios = {
        DEFAULT_LEADER_RATIO,
        DEFAULT_TOWN_ARCHER_RATIO,
        DEFAULT_CASTLE_RATIO
    };
    int lineno = 0;

    while (fgets(line, sizeof(line), f) != NULL) {
        lineno++;
        size_t L = strlen(line);
        while (L > 0 && (line[L-1] == '\n' || line[L-1] == '\r')) { line[--L] = '\0'; }

        if (L == 0) continue; 

        char datebuf[32];
        long long wave = 0, inf = 0, leader = 0, town_archer = 0, castle = 0;
        float ratio_leader = 0.0f, ratio_town_archer = 0.0f, ratio_castle = 0.0f;
        int comma_count = 0;
        for (size_t i = 0; i < L; ++i) {
            if (line[i] == ',') comma_count++;
        }

        int scanned9 = sscanf(line, "%31[^,],%lld,%lld,%lld,%lld,%lld,%f,%f,%f",
                              datebuf, &wave, &inf, &leader, &town_archer, &castle,
                              &ratio_leader, &ratio_town_archer, &ratio_castle);
        int scanned6 = sscanf(line, "%31[^,],%lld,%lld,%lld,%lld,%lld",
                             datebuf, &wave, &inf, &leader, &town_archer, &castle);

        if (comma_count >= 8 && scanned9 == 9) {
            strncpy(last_datebuf, datebuf, sizeof(last_datebuf)-1);
            last_datebuf[sizeof(last_datebuf)-1] = '\0';
            last_wave = wave;
            last_inf = inf;
            last_leader = leader;
            last_town_archer = town_archer;
            last_castle = castle;
            last_ratios.leader = ratio_leader;
            last_ratios.town_archer = ratio_town_archer;
            last_ratios.castle = ratio_castle;
            found = 1;
        } else if (scanned6 == 6 && (comma_count == 5 || comma_count == 6)) {
            long long ignored_hero = 0;
            int scanned7 = sscanf(line, "%31[^,],%lld,%lld,%lld,%lld,%lld,%lld",
                                  datebuf, &wave, &inf, &leader, &ignored_hero, &town_archer, &castle);
            if (comma_count == 5 && scanned6 == 6) {
                strncpy(last_datebuf, datebuf, sizeof(last_datebuf)-1);
                last_datebuf[sizeof(last_datebuf)-1] = '\0';
                last_wave = wave;
                last_inf = inf;
                last_leader = leader;
                last_town_archer = town_archer;
                last_castle = castle;
                found = 1;
            } else if (comma_count == 6 && scanned7 == 7) {
                strncpy(last_datebuf, datebuf, sizeof(last_datebuf)-1);
                last_datebuf[sizeof(last_datebuf)-1] = '\0';
                last_wave = wave;
                last_inf = inf;
                last_leader = leader;
                last_town_archer = town_archer;
                last_castle = castle;
                found = 1;
            } else {
                fprintf(stderr, "Nota: riga %d ignorata (corrotta o formato sbagliato): %s\n", lineno, line);
            }
        }
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "Warning: Could not properly close '%s'.\n", PLAYER_DATA_FILE);
    }

    if (!found) return 0;

    strncpy(p->last_update, last_datebuf, sizeof(p->last_update)-1);
    p->last_update[sizeof(p->last_update)-1] = '\0';
    p->wave = last_wave;
    p->infinity_castle_level = last_inf;
    p->leader_level = last_leader;
    p->town_archer_level = last_town_archer;
    p->castle_level = last_castle;
    p->recommended_ratios = last_ratios;
    return 1;
}

int delete_last_player_record(void)
{
    const char *temporary_file = "data/player_data.csv.tmp";
    FILE *input = fopen(PLAYER_DATA_FILE, "rb");
    if (!input) {
        return 0;
    }

    char line[512];
    long last_record_position = -1;
    long line_position = ftell(input);
    while (fgets(line, sizeof(line), input) != NULL) {
        size_t length = strlen(line);
        int has_content = 0;
        for (size_t index = 0; index < length; ++index) {
            if (line[index] != '\n' && line[index] != '\r' && line[index] != ' ' && line[index] != '\t') {
                has_content = 1;
                break;
            }
        }
        if (has_content) {
            last_record_position = line_position;
        }
        line_position = ftell(input);
    }

    if (ferror(input) || last_record_position < 0) {
        fclose(input);
        return 0;
    }

    if (last_record_position == 0) {
        if (fclose(input) != 0) {
            return 0;
        }
        FILE *empty_file = fopen(PLAYER_DATA_FILE, "wb");
        if (!empty_file) {
            return 0;
        }
        return fclose(empty_file) == 0;
    }

    rewind(input);
    FILE *output = fopen(temporary_file, "wb");
    if (!output) {
        fclose(input);
        return 0;
    }

    long copied = 0;
    int character;
    while (copied < last_record_position && (character = fgetc(input)) != EOF) {
        if (fputc(character, output) == EOF) {
            fclose(input);
            fclose(output);
            remove(temporary_file);
            return 0;
        }
        copied++;
    }

    int success = copied == last_record_position && fclose(input) == 0 && fclose(output) == 0;
    if (!success) {
        remove(temporary_file);
        return 0;
    }

#ifdef _WIN32
    if (!MoveFileExA(temporary_file, PLAYER_DATA_FILE, MOVEFILE_REPLACE_EXISTING)) {
        remove(temporary_file);
        return 0;
    }
#else
    if (rename(temporary_file, PLAYER_DATA_FILE) != 0) {
        remove(temporary_file);
        return 0;
    }
#endif
    return 1;
}

int save_custom_hero(const CustomHero *hero)
{
    if (!hero) return 0;
    if (!ensure_data_dir()) {
        fprintf(stderr, "Error: Could not create or access 'data' directory.\n");
        return 0;
    }

    FILE *f = fopen(CUSTOM_HEROES_FILE, "a");
    if (!f) {
        fprintf(stderr, "Error: Could not open '%s' for writing.\n", CUSTOM_HEROES_FILE);
        return 0;
    }

    int ret = fprintf(f, "%s,%f,%lld\n", hero->name, hero->target_ratio, hero->level);
    if (ret < 0) {
        fprintf(stderr, "Error: Failed to write to '%s'.\n", CUSTOM_HEROES_FILE);
        fclose(f);
        return 0;
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "Error: Failed to close '%s'.\n", CUSTOM_HEROES_FILE);
        return 0;
    }

    return 1;
}

int save_custom_heroes(const CustomHero *heroes, int hero_count)
{
    if (!heroes || hero_count < 0) return 0;
    if (!ensure_data_dir()) {
        fprintf(stderr, "Error: Could not create or access 'data' directory.\n");
        return 0;
    }

    FILE *f = fopen(CUSTOM_HEROES_FILE, "w");
    if (!f) {
        fprintf(stderr, "Error: Could not open '%s' for writing.\n", CUSTOM_HEROES_FILE);
        return 0;
    }

    for (int i = 0; i < hero_count; ++i) {
        if (fprintf(f, "%s,%.6f,%lld\n", heroes[i].name, heroes[i].target_ratio, heroes[i].level) < 0) {
            fclose(f);
            return 0;
        }
    }

    return fclose(f) == 0;
}

int delete_custom_hero(int hero_index)
{
    CustomHero heroes[32];
    int hero_count = load_custom_heroes(heroes, 32);
    if (hero_index < 0 || hero_index >= hero_count) {
        return 0;
    }

    for (int index = hero_index; index < hero_count - 1; ++index) {
        heroes[index] = heroes[index + 1];
    }
    return save_custom_heroes(heroes, hero_count - 1);
}

int load_custom_heroes(CustomHero *heroes, int max_heroes)
{
    if (!heroes || max_heroes <= 0) return 0;

    FILE *f = fopen(CUSTOM_HEROES_FILE, "r");
    if (!f) {
        return 0;
    }

    char line[512];
    int count = 0;

    while (fgets(line, sizeof(line), f) != NULL && count < max_heroes) {
        size_t L = strlen(line);
        while (L > 0 && (line[L - 1] == '\n' || line[L - 1] == '\r')) {
            line[--L] = '\0';
        }

        if (L == 0) continue;

        char name[64] = {0};
        float target_ratio = 0.0f;
        long long level = 0;
        int scanned = sscanf(line, "%63[^,],%f,%lld", name, &target_ratio, &level);
        if (scanned == 3) {
            strncpy(heroes[count].name, name, sizeof(heroes[count].name) - 1);
            heroes[count].name[sizeof(heroes[count].name) - 1] = '\0';
            heroes[count].target_ratio = target_ratio;
            heroes[count].level = level;
            count++;
        }
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "Warning: Could not properly close '%s'.\n", CUSTOM_HEROES_FILE);
    }

    return count;
}

int save_pace_data(const PaceInputs *inputs)
{
    if (!inputs) {
        return 0;
    }

    if (!ensure_data_dir()) {
        fprintf(stderr, "Error: Could not create or access 'data' directory.\n");
        return 0;
    }

    FILE *f = fopen(PACE_DATA_FILE, "w");
    if (!f) {
        fprintf(stderr, "Error: Could not open '%s' for writing.\n", PACE_DATA_FILE);
        return 0;
    }

    int ret = fprintf(f, "%d,%d,%d,%d,%d,%d,%d\n",
        inputs->dhLevel,
        inputs->goldenHorn,
        inputs->horn,
        inputs->gameSpeed,
        (int)inputs->chrono,
        inputs->ob,
        inputs->mbf);

    if (ret < 0) {
        fprintf(stderr, "Error: Failed to write to '%s'.\n", PACE_DATA_FILE);
        fclose(f);
        return 0;
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "Error: Failed to close '%s'.\n", PACE_DATA_FILE);
        return 0;
    }

    return 1;
}

int load_pace_data(PaceInputs *inputs)
{
    if (!inputs) {
        return 0;
    }

    FILE *f = fopen(PACE_DATA_FILE, "r");
    if (!f) {
        return 0;
    }

    int chrono = 0;
    int loaded = fscanf(f, "%d,%d,%d,%d,%d,%d,%d",
        &inputs->dhLevel,
        &inputs->goldenHorn,
        &inputs->horn,
        &inputs->gameSpeed,
        &chrono,
        &inputs->ob,
        &inputs->mbf) == 7;
    inputs->chrono = (PaceChrono)chrono;

    if (fclose(f) != 0) {
        fprintf(stderr, "Warning: Could not properly close '%s'.\n", PACE_DATA_FILE);
    }

    return loaded;
}

static void reverse_progress_range(ProgressData *arr, int start, int end)
{
    while (start < end) {
        ProgressData tmp = arr[start];
        arr[start] = arr[end];
        arr[end] = tmp;
        start++;
        end--;
    }
}

// In-place left rotation (reversal algorithm) so no extra buffer is needed.
static void rotate_progress_left(ProgressData *arr, int count, int shift)
{
    reverse_progress_range(arr, 0, shift - 1);
    reverse_progress_range(arr, shift, count - 1);
    reverse_progress_range(arr, 0, count - 1);
}

int read_progress_history(const char *filename, ProgressData *out, int max_entries)
{
    if (!out || max_entries <= 0 || !filename) return 0;

    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Warning: Could not open '%s' for reading.\n", filename);
        return 0;
    }

    char line[512];
    ProgressData first_entry;
    int has_first = 0;
    long long total_valid = 0;
    int lineno = 0;

    // out[] is used as a circular buffer holding the most recent max_entries rows.
    while (fgets(line, sizeof(line), f)) {
        lineno++;

        size_t L = strlen(line);
        while (L > 0 && (line[L-1] == '\n' || line[L-1] == '\r')) { line[--L] = '\0'; }

        if (L == 0) continue;

        char datebuf[64];
        long long wave = 0, inf = 0, leader = 0;
        int scanned = sscanf(line, "%63[^,],%lld,%lld,%lld", datebuf, &wave, &inf, &leader);
        if (scanned >= 3) {
            ProgressData entry;
            strncpy(entry.date, datebuf, sizeof(entry.date)-1);
            entry.date[sizeof(entry.date)-1] = '\0';
            entry.wave = wave;
            entry.infinity_castle_level = inf;

            if (!has_first) {
                first_entry = entry;
                has_first = 1;
            }

            out[total_valid % max_entries] = entry;
            total_valid++;
        } else {
            fprintf(stderr, "Nota: riga %d ignorata (corrotta o formato sbagliato): %s\n", lineno, line);
        }
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "Warning: Could not properly close '%s'.\n", filename);
    }

    if (total_valid == 0) {
        return 0;
    }

    if (total_valid <= max_entries) {
        return (int)total_valid;
    }

    // More rows than capacity: rotate the buffer into chronological order, then
    // keep the true first record (needed for all-time stats) plus the most recent entries.
    int oldest_index = (int)(total_valid % max_entries);
    if (oldest_index != 0) {
        rotate_progress_left(out, max_entries, oldest_index);
    }
    out[0] = first_entry;
    return max_entries;
}
