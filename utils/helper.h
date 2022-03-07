#ifndef __HELPER_L__
#define __HELPER_L__

/* RESIZE FUNCTIONALITY */
void *resize_array(void *arr, int *max_len, int curr_index, size_t singleton_size);

/* SPLIT STRING FUNCTIONALITY */
int char_is_range(char _char);
int num_is_range(char _char);
char **split_string(char *full_string, char delimeter, int *arr_len, char *extra, ...);

/* DELIMETER CHECK ON SPLIT STRING */
int delimeter_check(char curr_char, char *delims);

#endif /* __HELPER_L__ */