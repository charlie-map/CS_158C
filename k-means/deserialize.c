#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "deserialize.h"
#include "../utils/helper.h"

void destroy_hashmap_float(void *v) {
	free((float *) v);

	return;
}

hashmap_body_t *create_hashmap_body(char *id, char *title, float mag) {
	hashmap_body_t *hm = (hashmap_body_t *) malloc(sizeof(hashmap_body_t));

	hm->id = id;
	hm->title = title;

	hm->mag = mag;
	hm->sqrt_mag = sqrt(mag);

	hm->map = make__hashmap(0, NULL, destroy_hashmap_float);

	return hm;
}

void destroy_hashmap_body(hashmap_body_t *body_hash) {
	free(body_hash->id);
	free(body_hash->title);

	deepdestroy__hashmap(body_hash->map);

	free(body_hash);
	return;
}

void hm_destroy_hashmap_body(void *hm_body) {
	return destroy_hashmap_body((hashmap_body_t *) hm_body);
}

hashmap *deserialize_title(char *title_reader) {
	FILE *index = fopen(title_reader, "r");

	if (!index) {
		printf("\033[0;31m");
		printf("\n** Error opening file **\n");
		printf("\033[0;37m");
	}

	hashmap *documents = make__hashmap(0, NULL, hm_destroy_hashmap_body);
	// create hashmap store
	size_t line_buffer_size = sizeof(char) * 8;
	char *line_buffer = malloc(line_buffer_size);

	int *row_num = malloc(sizeof(int));

	int line_buffer_length = 0;
	while ((line_buffer_length = getline(&line_buffer, &line_buffer_size, index)) != -1) {
		char **split_row = split_string(line_buffer, 0, row_num, "-d-r", delimeter_check, ": ", num_is_range);

		int title_length = line_buffer_length - (strlen(split_row[0]) + strlen(split_row[*row_num - 1]) + 2);
		// now pull out the different components into a hashmap value:
		char *doc_ID = split_row[0];

		float mag = atof(split_row[*row_num - 1]);
		free(split_row[*row_num - 1]);

		char *doc_title = malloc(sizeof(char) * title_length);
		strcpy(doc_title, split_row[1]);

		free(split_row[1]);

		for (int cp_doc_title = 2; cp_doc_title < *row_num - 1; cp_doc_title++) {
			strcat(doc_title, " ");
			strcat(doc_title, split_row[cp_doc_title]);

			free(split_row[cp_doc_title]);
		}

		free(split_row);

		insert__hashmap(documents, doc_ID, create_hashmap_body(doc_ID, doc_title, mag), "", compareCharKey, NULL);
	}

	free(row_num);
	free(line_buffer);

	fclose(index);

	return documents;
}

int destroy_split_string(char **split_string, int *split_string_len) {
	for (int delete_sub = 0; delete_sub < *split_string_len; delete_sub++) {
		free(split_string[delete_sub]);
	}

	free(split_string);

	return 0;
}

char **deserialize(char *index_reader, hashmap *docs, int *max_words) {
	int words_index = 0; *max_words = 132;
	char **words = malloc(sizeof(char *) * *max_words);

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
		int *line_sub_max = malloc(sizeof(int));
		char **line_subs = split_string(line_buffer, 0, line_sub_max, "-d-r", delimeter_check, " :,|", num_is_range);

		words[words_index] = line_subs[0];

		// ladder 9:124,1|93,1|245,2|190,1|193,2|19,1|104,1|55,3|57,2|
		// go through each document and compute normalized (using document frequency) term frequencies
		float doc_freq = atof(line_subs[1]);
		free(line_subs[1]);

		for (int read_doc_freq = 2; read_doc_freq < *line_sub_max; read_doc_freq += 2) {
			hashmap_body_t *doc = get__hashmap(docs, line_subs[read_doc_freq], 0);
			free(line_subs[read_doc_freq]);

			// calculate term_frequency / document_frequency
			float term_frequency = atof(line_subs[read_doc_freq + 1]);
			free(line_subs[read_doc_freq + 1]);

			float *normal_term_freq = malloc(sizeof(float));
			*normal_term_freq = term_frequency / doc_freq;

			insert__hashmap(doc->map, words[words_index], normal_term_freq, "", compareCharKey, NULL);
		}

		words_index++;
		words = resize_array(words, max_words, words_index, sizeof(char *));

		free(line_sub_max);
		free(line_subs);
	}

	free(line_buffer);
	fclose(index);

	*max_words = words_index;

	return words;
}