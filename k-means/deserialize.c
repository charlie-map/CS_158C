#include <stdio.h>
#include <stdlib.h>

#include "deserialize.h"
#include "../utils/helper.h"

hashmap_body_t *create_hashmap_body(char *id, char *title, float mag) {
	hashmap_body_t *hm = (hashmap_body_t *) malloc(sizeof(hashmap_body_t));

	hm->id = id;
	hm->title = title;

	hm->mag = mag;
	hm->sqrt_mag = sqrt(mag);

	return hm;
}

hashmap_body_t **deserialize_title(char *title_reader, int *max_hm_body) {
	FILE *index = fopen(title_reader, "r");

	if (!index) {
		printf("\033[0;31m");
		printf("\n** Error opening file **\n");
		printf("\033[0;37m");
	}

	// create hashmap store
	int hm_body_index = 0; *max_hm_body = 8;
	hashmap_body_t **hm_body = malloc(sizeof(hashmap_body_t *) * *max_hm_body);

	size_t line_buffer_size = sizeof(char) * 8;
	char *line_buffer = malloc(line_buffer_size);

	int *row_num = malloc(sizeof(int)), **split_row_word_len = malloc(sizeof(int *));

	int line_buffer_length = 0;
	while ((line_buffer_length = getline(&line_buffer, &line_buffer_size, index)) != -1) {
		char **split_row = split_string(line_buffer, ':', row_num, "-r-l", num_is_range, split_row_word_len);

		int id_mag_length = strlen(split_row[0]) + strlen(split_row[*row_num - 1]);
		// now pull out the different components into a hashmap value:
		char *doc_ID = split_row[0];

		float mag = (float) split_row[*row_num - 1];
		free(split_row[*row_num - 1]);

		char *doc_title = malloc(sizeof(char) * id_mag_length);
		strcpy(doc_title, split_row[1]);

		for (int cp_doc_title = 2; cp_doc_title < *row_num - 1; cp_doc_title++) {
			strcat(doc_title, split_row[cp_doc_title]);
		}

		hm_body[hm_body_index] = create_hashmap_body(doc_ID, doc_title, mag);
	}

	return documents;
}

void destroy_hashmap_body(hashmap_body_t *body_hash) {
	free(body_hash->id);
	free(body_hash->title);

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