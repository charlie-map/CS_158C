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

int is_delim(char de, char *delims) {
	return delims[0] == de || delims[1] == de || delims[2] == de;
}

cluster_t **deserialize_cluster(char *filename, int k, hashmap *doc_map, char **word_bag, int *word_bag_len) {
	FILE *read_cluster = fopen(filename, "r");

	if (!read_cluster) {
		printf("\033[0;31m");
		printf("\n** Unable to open %s **\n", filename);
		printf("\033[0;37m");
	}

	cluster_t **cluster = malloc(sizeof(cluster_t *) * k);
	int doc_pos_index, max_doc_pos;

	size_t cluster_string_size = sizeof(char);
	char *cluster_string = malloc(cluster_string_size);
	
	int *line_len = malloc(sizeof(int));
	int *doc_key_len = malloc(sizeof(int));

	for (int curr_cluster = 0; curr_cluster < k; curr_cluster++) {
		cluster[curr_cluster] = malloc(sizeof(cluster_t));

		getline(&cluster_string, &cluster_string_size, read_cluster);
		char **cluster_data = split_string(cluster_string, ' ', line_len, "-d-r", is_delim, " :,", NULL);

		// first value is just the cluster mag:
		cluster[curr_cluster]->sqrt_mag = atof(cluster_data[0]);
		free(cluster_data[0]);

		// doc pos index
		doc_pos_index = atoi(cluster_data[1]);
		free(cluster_data[1]);
		max_doc_pos = doc_pos_index + 1;

		char **doc_pos = malloc(sizeof(char *) * max_doc_pos);

		cluster[curr_cluster]->doc_pos_index = doc_pos_index;
		cluster[curr_cluster]->max_doc_pos = max_doc_pos;

		hashmap *centroid = make__hashmap(0, NULL, destroy_cluster_centroid_data);

		// connect each document into the doc_pos
		// and recompute the centroid hashmap
		int read_doc;
		for (read_doc = 0; read_doc < doc_pos_index; read_doc++) {
			char *doc_key = getKey__hashmap(doc_map, cluster_data[read_doc + 2]);
			free(cluster_data[read_doc + 2]);

			doc_pos[read_doc] = doc_key;

			hashmap_body_t *curr_doc = get__hashmap(doc_map, doc_key, 0);

			// look at all keys in the document
			char **curr_doc_keys = (char **) keys__hashmap(curr_doc->map, doc_key_len, "");

			for (int read_curr_doc_data = 0; read_curr_doc_data < *doc_key_len; read_curr_doc_data++) {
				cluster_centroid_data *data_pt = get__hashmap(centroid, curr_doc_keys[read_curr_doc_data], 0);
				float curr_doc_value = *(float *) get__hashmap(curr_doc->map, curr_doc_keys[read_curr_doc_data], 0);

				if (!data_pt) {
					data_pt = create_cluster_centroid_data(curr_doc_value / doc_pos_index);

					insert__hashmap(centroid, curr_doc_keys[read_curr_doc_data], data_pt, "", compareCharKey, NULL);
					continue;
				}

				data_pt->tf_idf += curr_doc_value / doc_pos_index;
				data_pt->doc_freq++;
			}

			free(curr_doc_keys);
		}

		free(cluster_data[read_doc + 2]);
		free(cluster_data);

		cluster[curr_cluster]->doc_pos = doc_pos;
		cluster[curr_cluster]->centroid = centroid;
	}

	free(line_len);
	free(doc_key_len);

	free(cluster_string);
	fclose(read_cluster);

	return cluster;
}