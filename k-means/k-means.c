#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "k-means.h"

// calculate the distance by going through the centroids word map
// if the doc doesn't have the word (or vice-versa) then the dot-product
// will be 0
// the close the return value is to 1, the more closely related
// the two documents are
float cosine_similarity(hashmap *doc, float doc_sqrt_mag, hashmap *centroid, float centroid_sqrt_mag) {
	int *centroid_key_len = malloc(sizeof(int));
	char **centroid_key = (char **) keys__hashmap(doc, centroid_key_len, "");

	float dot_product = 0;
	for (int calc_dp = 0; calc_dp < *centroid_key_len; calc_dp++) {
		// grab value from each map
		char *key = centroid_key[calc_dp];

		void *pre_doc_word_tfidf = get__hashmap(centroid, key, 0);
		if (!pre_doc_word_tfidf) { // insert into centroid with new data
			float *new_centroid_data = malloc(sizeof(float));
			*new_centroid_data = 0.0;
			insert__hashmap(centroid, key, new_centroid_data, "", compareCharKey, NULL);
		}

		float doc_word_tfidf = pre_doc_word_tfidf ? *(float *) pre_doc_word_tfidf : 0.0;

		float centroid_word_tfidf = *(float *) get__hashmap(doc, key, 0);

		dot_product += (doc_word_tfidf * centroid_word_tfidf);
	}

	free(centroid_key);
	free(centroid_key_len);

	// use each document magnitude to get the final similarity value
	float similarity = dot_product / (doc_sqrt_mag * centroid_sqrt_mag);

	return similarity;
}

int copy__hashmap(hashmap *m1, hashmap *m2);

float *centroid_mean_calculate(cluster_t **centroids, float *mean_shift, int k, hashmap *doc);
int has_changed(float *mean_shift, float *prev_mean_shift, int k, float threshold);

int destroy_cluster(cluster_t **cluster, int k) {
	for (int del_cluster = 0; del_cluster < k; del_cluster++) {
		free(cluster[del_cluster]->doc_pos);

		deepdestroy__hashmap(cluster[del_cluster]->centroid);
		free(cluster[del_cluster]);
	}

	free(cluster);

	return 0;
}

cluster_t **k_means(hashmap *doc, int k, int cluster_threshold) {
	srand(time(NULL));
	// choosing random k documents

	int *doc_ID_len = malloc(sizeof(int));
	char **doc_ID = (char **) keys__hashmap(doc, doc_ID_len, "");
	// create centroids based off of first k documents
	cluster_t **cluster = malloc(sizeof(cluster_t *) * k);
	for (int create_centroid = 0; create_centroid < k; create_centroid++) {
		hashmap *new_centroid = make__hashmap(0, NULL, destroy_hashmap_float);

		int rand_copy_map_val = rand() % *doc_ID_len;

		hashmap_body_t *copyer_hashmap = get__hashmap(doc, doc_ID[rand_copy_map_val], 0);
		copy__hashmap(new_centroid, copyer_hashmap->map);

		cluster[create_centroid] = malloc(sizeof(cluster_t));
		cluster[create_centroid]->doc_pos = malloc(sizeof(char *) * 8);
		cluster[create_centroid]->max_doc_pos = 8;
		cluster[create_centroid]->doc_pos_index = 0;
		
		cluster[create_centroid]->sqrt_mag = copyer_hashmap->sqrt_mag;

		cluster[create_centroid]->centroid = new_centroid;
	}

	float *mean_shifts = malloc(sizeof(float) * k);
	float *prev_mean_shifts = malloc(sizeof(float) * k);
	// fill mean_shifts with 0's initially
	memset(mean_shifts, 0, sizeof(float) * k);
	memset(prev_mean_shifts, 0, sizeof(float) * k);
	
	do {
		printf("RUNNING\n");
		// reset clusters and prev_mean_shifts:
		for (int cluster_reset = 0; cluster_reset < k; cluster_reset++) {
			cluster[cluster_reset]->doc_pos_index = 0;
			prev_mean_shifts[cluster_reset] = mean_shifts[cluster_reset];
		}

		// go through non-centroid documents and assign them to centroids
		for (int find_doc_centroid = 0; find_doc_centroid < *doc_ID_len; find_doc_centroid++) {

			hashmap_body_t *curr_doc = ((hashmap_body_t *) get__hashmap(doc, doc_ID[find_doc_centroid], 0));
			// find most similar centroid using cosine similarity (largest value returned is most similar):
			int max_centroid = 0;
			float max = cosine_similarity(curr_doc->map, curr_doc->sqrt_mag, cluster[0]->centroid, cluster[0]->sqrt_mag);
			for (int curr_centroid = 1; curr_centroid < k; curr_centroid++) {
				
				float check_max = cosine_similarity(curr_doc->map, curr_doc->sqrt_mag,
					cluster[curr_centroid]->centroid, cluster[curr_centroid]->sqrt_mag);

				if (check_max > max) {
					max = check_max;
					max_centroid = curr_centroid;
				}
			}

			cluster_t *curr_max_centroid = cluster[max_centroid];

			// do something with this information...
			curr_max_centroid->doc_pos[curr_max_centroid->doc_pos_index++] = doc_ID[find_doc_centroid];

			// resize check
			if (curr_max_centroid->doc_pos_index == curr_max_centroid->max_doc_pos) {
				curr_max_centroid->max_doc_pos *= 2;

				curr_max_centroid->doc_pos = realloc(curr_max_centroid->doc_pos, sizeof(char *) * cluster[max_centroid]->max_doc_pos);
			}
		}

		// calculate new averages ([k]means) for each centroid
		centroid_mean_calculate(cluster, mean_shifts, k, doc);
	} while(has_changed(mean_shifts, prev_mean_shifts, k, cluster_threshold));

	free(doc_ID_len);
	free(doc_ID);

	free(mean_shifts);
	free(prev_mean_shifts);

	return cluster;
}

int has_changed(float *mean_shift, float *prev_mean_shift, int k, float threshold) {
	for (int check_mean = 0; check_mean < k; check_mean++) {
		float diff = mean_shift[check_mean] - prev_mean_shift[check_mean];
		if ((diff > 0 && diff > threshold) || (diff < 0 && diff * -1 > threshold))
			return 1;
	}

	return 0;
}

float *centroid_mean_calculate(cluster_t **centroids, float *mean_shift, int k, hashmap *doc) {
	// for each centroid
	for (int find_mean_centroid = 0; find_mean_centroid < k; find_mean_centroid++) {
		mean_shift[find_mean_centroid] = 0;
		// calculate a average for each word in centroid
		int cluster_size = centroids[find_mean_centroid]->doc_pos_index;

		int *cluster_word_len = malloc(sizeof(int));
		char **cluster_word = (char **) keys__hashmap(centroids[find_mean_centroid]->centroid, cluster_word_len, "");

		// for each word in cluster, add up tf-idf from each document in cluster and devide by number of documents
		for (int word_mean = 0; word_mean < *cluster_word_len; word_mean++) {
			float *centroid_tfidf = get__hashmap(centroids[find_mean_centroid]->centroid, cluster_word[word_mean], 0);
			float prev_centroid_tfidf = *centroid_tfidf;			
			*centroid_tfidf = 0;

			for (int check_doc = 0; check_doc < cluster_size; check_doc++) {
				float *doc_tfidf = (float *) get__hashmap(((hashmap_body_t *) get__hashmap(doc,
					centroids[find_mean_centroid]->doc_pos[check_doc], 0))->map, cluster_word[word_mean], 0);

				if (!doc_tfidf)
					continue;

				*centroid_tfidf += *doc_tfidf;
			}

			*centroid_tfidf /= cluster_size;
			float tfidf_diff = (*centroid_tfidf) - prev_centroid_tfidf;
			mean_shift[find_mean_centroid] += tfidf_diff * tfidf_diff;
		}

		free(cluster_word_len);
		free(cluster_word);
	}

	return mean_shift;
}

// take all values in m2 and copy them into m1
int copy__hashmap(hashmap *m1, hashmap *m2) {
	int *m2_value_len = malloc(sizeof(int));
	char **m2_words = (char **) keys__hashmap(m2, m2_value_len, "");

	for (int cp_value = 0; cp_value < *m2_value_len; cp_value++) {

		float *m1_new_value = malloc(sizeof(float));
		void *test = get__hashmap(m2, m2_words[cp_value], 0);

		if (!test) {
			printf("UH OH %s\n", m2_words[cp_value]);
			continue;
		}

		*m1_new_value = *(float *) test;

		insert__hashmap(m1, m2_words[cp_value], m1_new_value, "", compareCharKey, NULL);
	}

	free(m2_value_len);
	free(m2_words);

	return 0;
}

/*
	Pattern for serialization will be:

	cluster_index cluster_mag doc_amount:doc_id,doc_id,doc_id,
	...
	...
*/
int cluster_to_file(cluster_t **cluster, int k, char *filename) {
	FILE *reader_file = fopen(filename, "w");

	if (!reader_file) {
		printf("\033[0;31m");
		printf("\n** Error opening file **\n");
		printf("\033[0;37m");
	}

	// for each cluster
	cluster_t *curr_cluster;
	for (int serialize_cluster = 0; serialize_cluster < k; serialize_cluster++) {
		curr_cluster = cluster[serialize_cluster];

		fprintf(reader_file, "%d %1.8f %d: ", serialize_cluster, curr_cluster->sqrt_mag, curr_cluster->doc_pos_index);

		for (int read_doc = 0; read_doc < curr_cluster->doc_pos_index; read_doc++) {
			fprintf(reader_file, "%s,", curr_cluster->doc_pos[read_doc]);
		}

		fputc('\n', reader_file);
	}

	fclose(reader_file);

	return 0;
}