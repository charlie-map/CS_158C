#include <stdio.h>
#include <stdlib.h>

#include "deserialize.h"
#include "../utils/helper.h"

hashmap_body_t **deserialize_title(char *title_reader, int *max_hm_body) {
	FILE *index = fopen(title_reader, "r");

	if (!index) {
		printf("\033[0;31m");
		printf("\n** Error opening file **\n");
		printf("\033[0;37m");
	}

	// create hashmap store
	int hm_body_index = 0;
	*max_hm_body = 8;
	hashmap_body_t **hm_body = malloc(sizeof(hashmap_body_t) * *max_hm_body);

	size_t line_buffer_size = sizeof(char) * 8;
	char *line_buffer = malloc(line_buffer_size);

	int *row_num = malloc(sizeof(int)), **split_row_word_len = malloc(sizeof(int *));

	while (getline(&line_buffer, &line_buffer_size, index) != -1) {
		char **split_row = split_string(line_buffer, ':', row_num, "-r-l", num_is_range, split_row_word_len);

		// now pull out the different components into a hashmap value:
		int doc_ID = atoi(split_row[0]);
		free(split_row[0]);

		// find split point between title and the maginitude

	}

	return documents;
}

void destroy_hashmap_body(hashmap_body_t *body_hash) {
	free(body_hash->id);

	deepdestroy__hashmap(body_hash->map);

	free(body_hash);
	return;
}

int destroy_split_string(char **split_string, int *split_string_len) {
	for (int delete_sub = 0; delete_sub < *split_string_len; delete_sub++) {
		free(split_string[delete_sub]);
	}

	free(split_string);

	return 0;
}

hashmap *deserialize(char *index_reader, hashmap *docs, char *** words, int dtf_drop_threshold) {
	hashmap *doc_major_map = make__hashmap(0, NULL, destroy_hashmap_body);

	FILE *index = fopen(index_reader, "r");

	if (!index) {
		printf("\033[0;31m");
		printf("\n** Error opening file **\n");
		printf("\033[0;37m");
	}

	size_t line_buffer_size = sizeof(char) * 8;
	char *line_buffer = malloc(line_buffer_size);

	while (getline(&line_buffer, &line_buffer_size, index) != -1) {
		// splice bag by multi_delimeters:
		// use " " and ":" and "|" as delimeters
		int *word_bag_words_max = malloc(sizeof(int));
		char **word_bag_words = split_string(word_bag, 0, word_bag_words_max, "-d-r", delimeter_check, " :", num_is_range);

		
	}

	return doc;
}