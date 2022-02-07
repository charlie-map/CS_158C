#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "hashmap.h"
#include "serialize.h"

// create index structure

/*
	goes through a char * to create an array of char * that are split
	based on the delimeter character. If the string was:

	char *example = "Hi, I am happy, as of now, thanks";

	then using split_string(example, ','); would return:

	["Hi", "I am happy", "as of now", "thanks"]
*/
char **split_string(char *full_string, char delimeter, int *arr_len) {
	int arr_index = 0;
	*arr_len = 8;
	char **arr = malloc(sizeof(char *) * *arr_len);

	int *max_curr_sub_word = malloc(sizeof(int)), curr_sub_word_index = 0;
	*max_curr_sub_word = 8;
	arr[arr_index] = malloc(sizeof(char) * *max_curr_sub_word);

	for (int read_string = 0; full_string[read_string]; read_string++) {
		if (full_string[read_string] == delimeter) { // next phrase
			arr_index++;

			if (arr_index == *arr_len) {
				*arr_len *= 2;
				arr = realloc(arr, sizeof(char *) * *arr_len);
			}

			curr_sub_word_index = 0;
			*max_curr_sub_word = 8;
			arr[arr_index] = malloc(sizeof(char) * *max_curr_sub_word);

			continue;
		}

		// if not in range, skip:
		if (((int) full_string[read_string] < 65 || (int) full_string[read_string] > 90) &&
			((int) full_string[read_string] < 97 || (int) full_string[read_string] > 122))
			continue;

		// if a capital letter, lowercase
		if ((int) full_string[read_string] <= 90)
			full_string[read_string] = (char) ((int) full_string[read_string] + 32);

		arr[arr_index][curr_sub_word_index++] = full_string[read_string];

		if (curr_sub_word_index == *max_curr_sub_word) {
			*max_curr_sub_word *= 2;
			arr[arr_index] = realloc(arr[arr_index], sizeof(char *) * *max_curr_sub_word);
		}

		arr[arr_index][curr_sub_word_index] = '\0';
	}

	*arr_len = arr_index + 1;

	return arr;
}

int destroy_split_string(char **split_string, int *split_string_len) {
	for (int delete_sub = 0; delete_sub < *split_string_len; delete_sub++) {
		free(split_string[delete_sub]);
	}

	free(split_string);

	return 0;
}

void destroy_hashmap_val(void *ptr) {
	free((int *) ptr);

	return;
}

int word_bag(FILE *index_fp, FILE *title_fp, trie_t *stopword_trie, token_t *full_page) {
	// create title page:
	// get ID
	int *ID_len = malloc(sizeof(int));
	char *ID = token_read_all_data(grab_token_by_tag(full_page, "id"), ID_len);

	// get title
	int *title_len = malloc(sizeof(int));
	char *title = token_read_all_data(grab_token_by_tag(full_page, "title"), title_len);

	// write to title_fp
	fputs(ID, title_fp);
	fputs(":", title_fp);
	fputs(title, title_fp);
	fputs("\n", title_fp);

	free(ID_len);
	free(title_len);
	free(title);

	// grab full page data
	int *page_data_len = malloc(sizeof(int)), *word_number_max = malloc(sizeof(int));
	char **full_page_data = split_string(token_read_all_data(grab_token_by_tag(full_page, "text"), page_data_len), ' ', word_number_max);

	// create hashmap representation:
	hashmap *word_freq_hash = make__hashmap(0, NULL, destroy_hashmap_val);

	// loop through full_page_data and for each word:
		// check if the word is already in the hashmap, if it is:
			// add to the frequency for that word
		// if it isn't:
			// insert into hashmap at word with frequency = 0
	for (int add_hash = 0; add_hash < *word_number_max; add_hash++) {
		// check if word is in the stopword trie:
		if (trie_search(stopword_trie, full_page_data[add_hash]))
			continue; // skip

		// get from word_freq_hash:
		int *hashmap_freq = get__hashmap(word_freq_hash, full_page_data[add_hash]);

		if (hashmap_freq) {
			(*hashmap_freq)++;
			continue;
		}

		hashmap_freq = malloc(sizeof(int));
		*hashmap_freq = 0;

		insert__hashmap(word_freq_hash, full_page_data[add_hash], hashmap_freq, "", compareIntKey, destroy_hashmap_val);
	}

	int *key_len = malloc(sizeof(int));
	char **keys = (char **) keys__hashmap(word_freq_hash, key_len);

	for (int test_keys = 0; test_keys < *key_len; test_keys++) {
		printf("key: %s\n", (char *) keys[test_keys]);
	}

	return 0;
}