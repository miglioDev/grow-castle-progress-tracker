#ifndef INPUT_SAFE_H
#define INPUT_SAFE_H

int safe_input_int(const char *prompt, int *result, int min_val, int max_val);
int safe_input_long_long(const char *prompt, long long *result, long long min_val, long long max_val);

#endif // INPUT_SAFE_H
