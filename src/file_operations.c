#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
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

static int g_progress_history_invalid_rows = 0;

static int is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

static int is_valid_progress_date(const char *text)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int parsed_length = 0;
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (!text || sscanf(text, "%d-%d-%d%n", &year, &month, &day, &parsed_length) != 3
        || (text[parsed_length] != '\0' && text[parsed_length] != ' ')) {
        return 0;
    }
    if (year < 1 || month < 1 || month > 12 || day < 1) {
        return 0;
    }
    if (month == 2 && is_leap_year(year)) {
        days_in_month[1] = 29;
    }
    if (day > days_in_month[month - 1]) {
        return 0;
    }
    if (text[parsed_length] == '\0') {
        return 1;
    }

    int hour = 0;
    int minute = 0;
    int second = 0;
    char trailing = '\0';
    return sscanf(text + parsed_length + 1, "%d:%d:%d%c", &hour, &minute, &second, &trailing) == 3
        && hour >= 0 && hour <= 23
        && minute >= 0 && minute <= 59
        && second >= 0 && second <= 59;
}

static int has_valid_player_values(long long wave, long long infinity_castle, long long leader,
    long long town_archer, long long castle, float leader_ratio, float town_archer_ratio, float castle_ratio)
{
    return wave >= 0 && infinity_castle >= 0 && leader >= 0 && town_archer >= 0 && castle >= 0
        && isfinite(leader_ratio) && isfinite(town_archer_ratio) && isfinite(castle_ratio)
        && leader_ratio > 0.0f && town_archer_ratio > 0.0f && castle_ratio > 0.0f;
}

static int is_valid_custom_hero_name(const char *name)
{
    size_t length = 0;
    size_t first_non_space = 0;
    size_t last_non_space = 0;

    if (!name) return 0;
    while (length < 64 && name[length] != '\0') {
        if (name[length] == ',' || name[length] == '\r' || name[length] == '\n') {
            return 0;
        }
        length++;
    }
    if (length == 0 || length >= 64) return 0;

    while (first_non_space < length
        && (name[first_non_space] == ' ' || name[first_non_space] == '\t')) {
        first_non_space++;
    }
    if (first_non_space == length) return 0;

    last_non_space = length;
    while (last_non_space > first_non_space
        && (name[last_non_space - 1] == ' ' || name[last_non_space - 1] == '\t')) {
        last_non_space--;
    }
    return last_non_space > first_non_space;
}


static int ensure_data_dir() {
    int ret = mkdir(DATA_DIR, 0755);
    // success = dir created or already exists (errno==EEXIST)
    return (ret == 0 || errno == EEXIST) ? 1 : 0;
}

static int replace_file(const char *temporary_file, const char *destination_file)
{
#ifdef _WIN32
    if (!MoveFileExA(temporary_file, destination_file, MOVEFILE_REPLACE_EXISTING)) {
        remove(temporary_file);
        return 0;
    }
#else
    if (rename(temporary_file, destination_file) != 0) {
        remove(temporary_file);
        return 0;
    }
#endif
    return 1;
}

// Save one record (always append) The final three fields store ratios
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

int replace_last_player_data(const Player *p)
{
    const char *temporary_file = "data/player_data.csv.tmp";
    FILE *input;
    FILE *output;
    char line[512];
    long last_record_position = -1;
    long line_position;
    long copied = 0;
    int character;

    if (!p || !ensure_data_dir()) {
        return 0;
    }
    input = fopen(PLAYER_DATA_FILE, "rb");
    if (!input) {
        return 0;
    }

    line_position = ftell(input);
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

    rewind(input);
    output = fopen(temporary_file, "wb");
    if (!output) {
        fclose(input);
        return 0;
    }
    while (copied < last_record_position && (character = fgetc(input)) != EOF) {
        if (fputc(character, output) == EOF) {
            fclose(input);
            fclose(output);
            remove(temporary_file);
            return 0;
        }
        copied++;
    }
    if (copied != last_record_position || fclose(input) != 0
        || fprintf(output, "%s,%lld,%lld,%lld,%lld,%lld,%.6f,%.6f,%.6f\n",
            p->last_update, p->wave, p->infinity_castle_level, p->leader_level,
            p->town_archer_level, p->castle_level, p->recommended_ratios.leader,
            p->recommended_ratios.town_archer, p->recommended_ratios.castle) < 0
        || fclose(output) != 0) {
        remove(temporary_file);
        return 0;
    }
    return replace_file(temporary_file, PLAYER_DATA_FILE);
}

// ret 1 if loaded, 0 if file is missing/empty
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

        if (comma_count >= 8 && scanned9 == 9 && is_valid_progress_date(datebuf)
            && has_valid_player_values(wave, inf, leader, town_archer, castle,
                ratio_leader, ratio_town_archer, ratio_castle)) {
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
            if (comma_count == 5 && scanned6 == 6 && is_valid_progress_date(datebuf)
                && wave >= 0 && inf >= 0 && leader >= 0 && town_archer >= 0 && castle >= 0) {
                strncpy(last_datebuf, datebuf, sizeof(last_datebuf)-1);
                last_datebuf[sizeof(last_datebuf)-1] = '\0';
                last_wave = wave;
                last_inf = inf;
                last_leader = leader;
                last_town_archer = town_archer;
                last_castle = castle;
                found = 1;
            } else if (comma_count == 6 && scanned7 == 7 && is_valid_progress_date(datebuf)
                && wave >= 0 && inf >= 0 && leader >= 0 && town_archer >= 0 && castle >= 0) {
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

    return replace_file(temporary_file, PLAYER_DATA_FILE);
}

int save_custom_hero(const CustomHero *hero)
{
    if (!hero) return 0;
    if (!is_valid_custom_hero_name(hero->name)
        || !isfinite(hero->target_ratio) || hero->target_ratio <= 0.0f
        || hero->target_ratio > 10.0f || hero->level < 1) {
        return 0;
    }
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
    const char *temporary_file = "data/custom_heroes.csv.tmp";
    if (!heroes || hero_count < 0 || hero_count > 32) return 0;
    for (int i = 0; i < hero_count; ++i) {
        if (!is_valid_custom_hero_name(heroes[i].name)
            || !isfinite(heroes[i].target_ratio) || heroes[i].target_ratio <= 0.0f
            || heroes[i].target_ratio > 10.0f || heroes[i].level < 1) {
            return 0;
        }
    }
    if (!ensure_data_dir()) {
        fprintf(stderr, "Error: Could not create or access 'data' directory.\n");
        return 0;
    }

    FILE *f = fopen(temporary_file, "w");
    if (!f) {
        fprintf(stderr, "Error: Could not open '%s' for writing.\n", temporary_file);
        return 0;
    }

    for (int i = 0; i < hero_count; ++i) {
        if (fprintf(f, "%s,%.6f,%lld\n", heroes[i].name, heroes[i].target_ratio, heroes[i].level) < 0) {
            fclose(f);
            remove(temporary_file);
            return 0;
        }
    }

    if (fclose(f) != 0) {
        remove(temporary_file);
        return 0;
    }
    return replace_file(temporary_file, CUSTOM_HEROES_FILE);
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
        if (scanned == 3 && isfinite(target_ratio) && target_ratio > 0.0f && target_ratio <= 10.0f && level >= 1) {
            size_t name_length = strlen(name);
            size_t name_start = 0;
            size_t name_end = name_length;
            while (name_start < name_length && (name[name_start] == ' ' || name[name_start] == '\t')) {
                name_start++;
            }
            while (name_end > name_start && (name[name_end - 1] == ' ' || name[name_end - 1] == '\t')) {
                name_end--;
            }
            if (name_start == name_end) continue;
            memmove(name, name + name_start, name_end - name_start);
            name[name_end - name_start] = '\0';
            if (!is_valid_custom_hero_name(name)) continue;
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
    const char *temporary_file = "data/pace_data.csv.tmp";
    if (!inputs) {
        return 0;
    }

    if (!ensure_data_dir()) {
        fprintf(stderr, "Error: Could not create or access 'data' directory.\n");
        return 0;
    }

    FILE *f = fopen(temporary_file, "w");
    if (!f) {
        fprintf(stderr, "Error: Could not open '%s' for writing.\n", temporary_file);
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
        fprintf(stderr, "Error: Failed to write to '%s'.\n", temporary_file);
        fclose(f);
        remove(temporary_file);
        return 0;
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "Error: Failed to close '%s'.\n", temporary_file);
        remove(temporary_file);
        return 0;
    }

    return replace_file(temporary_file, PACE_DATA_FILE);
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

    PaceInputs loaded_inputs = {0};
    int chrono = 0;
    int loaded = fscanf(f, "%d,%d,%d,%d,%d,%d,%d",
        &loaded_inputs.dhLevel,
        &loaded_inputs.goldenHorn,
        &loaded_inputs.horn,
        &loaded_inputs.gameSpeed,
        &chrono,
        &loaded_inputs.ob,
        &loaded_inputs.mbf) == 7;
    if (loaded && (loaded_inputs.dhLevel < 0 || loaded_inputs.dhLevel > 5
        || (loaded_inputs.goldenHorn != 0 && loaded_inputs.goldenHorn != 1)
        || (loaded_inputs.horn != 0 && loaded_inputs.horn != 1)
        || (loaded_inputs.gameSpeed != 2 && loaded_inputs.gameSpeed != 3)
        || chrono < PACE_CHRONO_NONE || chrono > PACE_CHRONO_BLUE
        || (loaded_inputs.ob != 0 && loaded_inputs.ob != 1)
        || (loaded_inputs.mbf != 0 && loaded_inputs.mbf != 1))) {
        loaded = 0;
    }
    if (loaded) {
        loaded_inputs.chrono = (PaceChrono)chrono;
        *inputs = loaded_inputs;
    }

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

// In-place left rotation (reversal algorithm) (no extra buffer is needed)
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
    g_progress_history_invalid_rows = 0;
    ProgressData first_entry;
    int has_first = 0;
    long long total_valid = 0;
    int lineno = 0;

    // out[] is used as a circular buffer holding the most recent max_entries rows
    while (fgets(line, sizeof(line), f)) {
        lineno++;

        size_t L = strlen(line);
        while (L > 0 && (line[L-1] == '\n' || line[L-1] == '\r')) { line[--L] = '\0'; }

        if (L == 0) continue;

        char datebuf[64];
        long long wave = 0, inf = 0, leader = 0;
        int scanned = sscanf(line, "%63[^,],%lld,%lld,%lld", datebuf, &wave, &inf, &leader);
        if (scanned >= 3 && is_valid_progress_date(datebuf) && wave >= 0 && inf >= 0 && leader >= 0) {
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
            g_progress_history_invalid_rows++;
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
    // keep the true first record (needed for all-time stats) + the most recent entries
    int oldest_index = (int)(total_valid % max_entries);
    if (oldest_index != 0) {
        rotate_progress_left(out, max_entries, oldest_index);
    }
    out[0] = first_entry;
    return max_entries;
}

int get_progress_history_invalid_row_count(void)
{
    return g_progress_history_invalid_rows;
}
