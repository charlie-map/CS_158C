#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "token.h"
#include "hashmap.h"

/*
	goes through a char * to create an array of char * that are split
	based on the delimeter character. If the string was:

	char *example = "Hi, I am happy, as of now, thanks";

	then using split_string(example, ','); would return:

	["Hi", "I am happy", "as of now", "thanks"]
*/
char **split_string(char *full_string, char delimeter, int *string_arr_index) {
	int max_string_arr = 8;
	*string_arr_index = 0;
	char **string_arr = malloc(sizeof(char *) * max_string_arr);
	string_arr[*string_arr_index] = malloc(sizeof(char) * 8);
	memset(string_arr[*string_arr_index], '\0', sizeof(char) * 8);

	int *string_len = malloc(sizeof(int)), curr_inner_index = 0;;
	*string_len = 0;

	// search through full string while continuing to add to the string_arr
	// full_string should have a null terminator!
	for (int check_full_string = 0; full_string[check_full_string]; check_full_string++) {
		if (full_string[check_full_string] == delimeter) { // jump to next char
			(*string_arr_index)++;

			string_arr = resize_arraylist(string_arr, &max_string_arr, *string_arr_index);

			string_arr[*string_arr_index] = malloc(sizeof(char) * 8);
			memset(string_arr[*string_arr_index], '\0', sizeof(char) * 8);

			curr_inner_index = 0;
			*string_len = 8;

			continue;
		}

		string_arr[*string_arr_index][curr_inner_index] = full_string[check_full_string];
		curr_inner_index++;

		// check string_arr length
		string_arr[*string_arr_index] = resize_arraylist(string_arr[*string_arr_index], string_len, curr_inner_index);
		string_arr[*string_arr_index][curr_inner_index] = '\0';
	}

	(*string_arr_index) += *string_arr_index != 0;

	free(string_len);
	return string_arr;
}

int destroy_split_string(char **split_string, int *split_string_len) {
	for (int delete_sub = 0; delete_sub < *split_string_len; delete_sub++) {
		free(split_string[delete_sub]);
	}

	free(split_string);

	return 0;
}

/*
	serialize will try to search through the void * to pull out informaiton
	for serialization. this is challenging to define as a single "general"
	process. however, using the char *param and function overloading the concept
	can theoretically be used (with a function header that connects to necessary
	functions). char *param will take in quite a few inputs:

	NOTES:
		- order does NOT matter
		- "i" is the required minimum inputs for the indexing to work, others are optional

	- "i": tells the program what the name (char *) of the output file
		for the generated index is (so all data can be written directly into the
		files)
	- "t": tells the program what the name (char *) of the title
		file is
		- if "t" is used, IMMEDIATELY after the (char *) that holds the title output filename
			should be the pointer to the function that gets the title of whatever
	- "o": defines the order the output file held within "i" should have
		- "w: i" is an example where "w" is the current word and "i" (in the local context)
			is the "id"
		- currently the above is the only parameter option (defaults to this behavior)

	- an example of the above could be:
		"ito", "invertedIndex.txt", "titleIndex.txt", "w: i"
*/
int serializeMETA(void *tree, char *param, ...) {
	// setup any initial requirements
	va_list additional_arg;
	va_start(additional_arg, param);

	FILE *index_fp = NULL;
	FILE *title_fp = NULL;

	char *format = "text/ [i]: [id]"; // default

	for (int check_param = 0; param[check_param]; check_param++) {
		if (param[check_param] == 'i') {
			index_fp = fopen(va_arg(additional_arg, char *), "w");
		} else if (param[check_param] == 't') {
			title_fp = fopen(va_arg(additional_arg, char *), "w");
		} else if (param[check_param] == 'o') {
			format = va_arg(additional_arg, char *);
		}
	}

	// format reader to prep for the next step

}

int main() {
	token_t *tree_root = tokenize("miniXML.txt");

	int *total_tags = malloc(sizeof(int));
	*total_tags = 0;
	char **tag = token_get_tag_data(tree_root, "id", total_tags);

	for (int check_tag = 0; check_tag < *total_tags; check_tag++) {
		printf("check tag: %s -- %d\n", tag[check_tag], strlen(tag[check_tag]));
	}

	free(total_tags);
	free(tag);

	int *split_res_length = malloc(sizeof(int));
	*split_res_length = 0;
	char **test_split = split_string("Hi I am happy as of now thanks", ' ', split_res_length);

	for (int read_split = 0; read_split < *split_res_length; read_split++) {
		printf("word at pos %d: %s\n", read_split, test_split[read_split]);
	}

	destroy_split_string(test_split, split_res_length);
	free(split_res_length);

	destroy_token(tree_root);

	return 0;
}