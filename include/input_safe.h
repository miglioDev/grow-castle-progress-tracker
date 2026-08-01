#ifndef INPUT_SAFE_H
#define INPUT_SAFE_H

#ifdef __cplusplus
extern "C" {
#endif

int safe_input_int(const char *prompt, int *result, int min_val, int max_val);
int safe_input_long_long(const char *prompt, long long *result, long long min_val, long long max_val);
int safe_input_float(const char *prompt, float *result, float min_val, float max_val);
int safe_input_string(const char *prompt, char *result, size_t size);

#ifdef __cplusplus
}
#endif

#endif // INPUT_SAFE_H
