#include <stdio.h>
#include <stdlib.h>

#include "k-means.h"

// calculate the distance by going through the centroids word map
// if the doc doesn't have the word (or vice-versa) then the dot-product
// will be 0
// the close the return value is to 1, the more closely related
// the two documents are
float cosine_similarity(hashmap *doc, float doc_sqrt_mag, hashmap *centroid, float centroid_sqrt_mag) {
	int *centroid_key_len = malloc(sizeof(int));
	char **centroid_key = (char **) keys__hashmap(doc, centroid_key_len);

	float dot_product = 0;
	for (int calc_dp = 0; calc_dp < *centroid_key_len; calc_dp++) {
		// grab value from each map
		void *pre_doc_word_tfidf = get__hashmap(centroid, centroid_key[calc_dp], 0);
		if (!pre_doc_word_tfidf) {// insert into centroid with new data
			float *new_centroid_data = malloc(sizeof(float));
			*new_centroid_data = 0.0;
			insert__hashmap(centroid, centroid_key[calc_dp], new_centroid_data, "", NULL, NULL);
		}

		int doc_word_tfidf = pre_doc_word_tfidf ? *(int *) pre_doc_word_tfidf : 0;
		int centroid_word_tfidf = *(int *) get__hashmap(doc, centroid_key[calc_dp], 0);

		dot_product += (doc_word_tfidf * centroid_word_tfidf) * 1.0;
	}

	// use each document magnitude to get the final similarity value
	float similarity = dot_product / (doc_sqrt_mag * centroid_sqrt_mag);

	return similarity;
}

int copy__hashmap(hashmap *m1, hashmap *m2);

float *centroid_mean_calculate(cluster_t **centroids, int k, hashmap_body_t **doc);
int has_changed(float *mean_shift, int k, float threshold);

cluster_t **k_means(hashmap_body_t **doc, int doc_len, hashmap *idf, int k, int cluster_threshold) {
	// will choose the first k documents as centroids :D

	// create centroids based off of first k documents
	cluster_t **cluster = malloc(sizeof(cluster_t *) * k);
	for (int create_centroid = 0; create_centroid < k; create_centroid++) {
		hashmap *new_centroid = make__hashmap(0, NULL, destroy_hashmap_float);

		copy__hashmap(new_centroid, doc[create_centroid]->map);

		cluster[create_centroid] = malloc(sizeof(cluster_t));
		cluster[create_centroid]->doc_pos = malloc(sizeof(int) * 8);
		cluster[create_centroid]->max_doc_pos = 8;
		cluster[create_centroid]->doc_pos_index = 0;
		
		cluster[create_centroid]->sqrt_mag = doc[create_centroid]->sqrt_mag;

		cluster[create_centroid]->centroid = new_centroid;
	}

	float *mean_shifts;
	
	while(has_changed(mean_shifts, k, cluster_threshold)) {
		// go through non-centroid documents and assign them to centroids
		for (int find_doc_centroid = 0; find_doc_centroid < doc_len; find_doc_centroid++) {

			// find most similar centroid using cosine similarity (largest value returned is most similar):
			int max_centroid = 0;
			float min = cosine_similarity(doc[find_doc_centroid]->map, doc[find_doc_centroid]->sqrt_mag, cluster[0]->centroid, cluster[0]->sqrt_mag);
			for (int curr_centroid = 1; curr_centroid < k; curr_centroid++) {
				
				float check_max = cosine_similarity(doc[find_doc_centroid]->map, doc[find_doc_centroid]->sqrt_mag,
					cluster[curr_centroid]->centroid, cluster[curr_centroid]->sqrt_mag);

				if (check_max < min) {
					min = check_max;
					max_centroid = curr_centroid;
				}
			}

			// do something with this information...
			cluster[max_centroid]->doc_pos[cluster[max_centroid]->doc_pos_index++] = find_doc_centroid;

			// resize check
			if (cluster[max_centroid]->doc_pos_index == cluster[max_centroid]->max_doc_pos) {
				cluster[max_centroid]->max_doc_pos *= 2;

				cluster[max_centroid]->doc_pos = realloc(cluster[max_centroid]->doc_pos, sizeof(int) * cluster[max_centroid]->max_doc_pos);
			}
		}

		// calculate new averages ([k]means) for each centroid
		if (mean_shifts)
			free(mean_shifts);
		mean_shifts = centroid_mean_calculate(cluster, k, doc);
	}

	return cluster;
}

int has_changed(float *mean_shift, int k, float threshold) {
	for (int check_mean = 0; check_mean < k; check_mean++) {
		if (mean_shift[check_mean] > threshold)
			return 1;
	}

	return 0;
}

float *centroid_mean_calculate(cluster_t **centroids, int k, hashmap_body_t **doc) {
	float *mean_shift = malloc(sizeof(float) * k); // calculate the shifting amount per doc

	// for each centroid
	for (int find_mean_centroid = 0; find_mean_centroid < k; find_mean_centroid++) {
		mean_shift[find_mean_centroid] = 0;
		// calculate a average for each word in centroid
		int cluster_size = centroids[find_mean_centroid]->doc_pos_index;

		int *cluster_word_len = malloc(sizeof(int));
		char **cluster_word = (char **) keys__hashmap(centroids[find_mean_centroid]->centroid, cluster_word_len);

		// for each word in cluster, add up tf-idf from each document in cluster and devide by number of documents
		for (int word_mean = 0; word_mean < *cluster_word_len; word_mean++) {
			float *centroid_tfidf = get__hashmap(centroids[find_mean_centroid]->centroid, cluster_word[word_mean], 0);
			float prev_centroid_tfidf = *centroid_tfidf;			
			*centroid_tfidf = 0;

			for (int check_doc = 0; check_doc < cluster_size; check_doc++) {
				void *doc_tfidf = get__hashmap(doc[centroids[find_mean_centroid]->doc_pos[check_doc]]->map, cluster_word[word_mean], 0);

				if (!doc_tfidf)
					continue;

				*centroid_tfidf += *(float *) doc_tfidf;
			}

			*centroid_tfidf /= cluster_size;
			float tfidf_diff = (*centroid_tfidf) - prev_centroid_tfidf;
			mean_shift[find_mean_centroid] += tfidf_diff * tfidf_diff;
		}
	}

	return mean_shift;
}

// take all values in m2 and copy them into m1
int copy__hashmap(hashmap *m1, hashmap *m2) {
	int *m2_value_len = malloc(sizeof(int));
	char **m2_words = (char **) keys__hashmap(m2, m2_value_len);

	for (int cp_value = 0; cp_value < *m2_value_len; cp_value++) {
		float *m1_new_value = malloc(sizeof(float));
		void *test = get__hashmap(m2, m2_words[cp_value], 0);

		*m1_new_value = *(int *) test * 1.0;

		insert__hashmap(m1, m2_words[cp_value], m1_new_value, "", NULL, NULL);

		free(m2_words[cp_value]);
	}

	free(m2_value_len);
	free(m2_words);

	return 0;
}