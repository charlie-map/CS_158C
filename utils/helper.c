#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "helper.h"

void *resize_array(void *arr, int *max_len, int curr_index, size_t singleton_size) {
	while (curr_index >= *max_len) {
		*max_len *= 2;

		arr = realloc(arr, singleton_size * *max_len);
	}
	
	return arr;
}

// create index structure
int delimeter_check(char curr_char, char *delims) {
	for (int check_delim = 0; delims[check_delim]; check_delim++) {
		if (delims[check_delim] == curr_char)
			return 1;
	}

	return 0;
}

/*
	goes through a char * to create an array of char * that are split
	based on the delimeter character. If the string was:

	char *example = "Hi, I am happy, as of now, thanks";

	then using split_string(example, ','); would return:

	["Hi", "I am happy", "as of now", "thanks"]

	-- UPDATE: 
	char *param:
		place directly after arr_len,
		-- uses a pattern to signify more parameters
	int **minor_length:
		stores the length of each position in the returned char **
		use: "-l" to access push in minor_length
		-- only used if passed, otherwise null
		-- int ** will have same length as char **
	int (*is_delim)(char delim, char *delimeters):
		gives the options to instead have multi delimeters
		use: "-d" to have is_delim and char *delimeters passed in
	int (*is_range)(char _char):
		checks for range: default is (with chars) first one:
				number range as well is second one:
		use: "-r" to access range functions
*/
	int char_is_range(char _char) {
		return (((int) _char >= 65 && (int) _char <= 90) || ((int) _char >= 97 && (int) _char <= 122));
	}

	int num_is_range(char _char) {
		return (((int) _char >= 65 && (int) _char <= 90) ||
			((int) _char >= 97 && (int) _char <= 122) ||
			((int) _char >= 48 && (int) _char <= 57));
	}
/*
*/
char **split_string(char *full_string, char delimeter, int *arr_len, char *extra, ...) {
	va_list param;
	va_start(param, extra);

	int **minor_length = NULL;
	int (*is_delim)(char, char *) = NULL;
	char *multi_delims = NULL;

	int (*is_range)(char _char) = char_is_range;

	for (int check_extra = 0; extra[check_extra]; check_extra++) {
		if (extra[check_extra] != '-')
			continue;

		if (extra[check_extra + 1] == 'l')
			minor_length = va_arg(param, int **);
		else if (extra[check_extra + 1] == 'd') {
			is_delim = va_arg(param, int (*)(char, char *));
			multi_delims = va_arg(param, char *);
		} else if (extra[check_extra + 1] == 'r') {
			is_range = va_arg(param, int (*)(char));
		}
	}

	int arr_index = 0;
	*arr_len = 8;
	char **arr = malloc(sizeof(char *) * *arr_len);

	if (minor_length)
		*minor_length = realloc(*minor_length, sizeof(int) * *arr_len);

	int *max_curr_sub_word = malloc(sizeof(int)), curr_sub_word_index = 0;
	*max_curr_sub_word = 8;
	arr[arr_index] = malloc(sizeof(char) * *max_curr_sub_word);
	arr[arr_index][0] = '\0';

	for (int read_string = 0; full_string[read_string]; read_string++) {
		if ((is_delim && is_delim(full_string[read_string], multi_delims)) || full_string[read_string] == delimeter) { // next phrase
			// check that current word has some length
			if (arr[arr_index][0] == '\0')
				continue;

			// quickly copy curr_sub_word_index into minor_length if minor_length is defined:
			if (minor_length) {
				(*minor_length)[arr_index] = curr_sub_word_index;
			}

			arr_index++;

			while (arr_index >= *arr_len) {
				*arr_len *= 2;
				arr = realloc(arr, sizeof(char *) * *arr_len);

				if (minor_length)
					*minor_length = realloc(*minor_length, sizeof(int *) * *arr_len);
			}

			curr_sub_word_index = 0;
			*max_curr_sub_word = 8;
			arr[arr_index] = malloc(sizeof(char) * *max_curr_sub_word);
			arr[arr_index][0] = '\0';

			continue;
		}

		// if not in range, skip:
		if (is_range && !is_range(full_string[read_string]))
			continue;

		// if a capital letter, lowercase
		if ((int) full_string[read_string] <= 90 && (int) full_string[read_string] >= 65)
			full_string[read_string] = (char) ((int) full_string[read_string] + 32);

		arr[arr_index][curr_sub_word_index] = full_string[read_string];
		curr_sub_word_index++;

		arr[arr_index] = resize_array(arr[arr_index], max_curr_sub_word, curr_sub_word_index, sizeof(char));

		arr[arr_index][curr_sub_word_index] = '\0';
	}

	if (arr[arr_index][0] == '\0') { // free position
		free(arr[arr_index]);

		arr_index--;
	} else if (minor_length)
			(*minor_length)[arr_index] = curr_sub_word_index;

	*arr_len = arr_index + 1;

	free(max_curr_sub_word);

	return arr;
}