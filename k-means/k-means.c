#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "k-means.h"
#include "heap.h"

cluster_centroid_data *create_cluster_centroid_data(float data) {
	cluster_centroid_data *new_ccd = malloc(sizeof(cluster_centroid_data));

	new_ccd->tf_idf = data;
	new_ccd->standard_deviation = 0;
	new_ccd->doc_freq = 0;

	return new_ccd;
}

void destroy_cluster_centroid_data(void *ccd) {
	free((cluster_centroid_data *) ccd);

	return;
}

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

		void *pre_doc_word_tfidf = get__hashmap(centroid, key, "");
		if (!pre_doc_word_tfidf) { // insert into centroid with new data
			cluster_centroid_data *new_centroid_data = create_cluster_centroid_data(0.0);
			insert__hashmap(centroid, key, new_centroid_data, "", compareCharKey, NULL);
		}

		float doc_word_tfidf = pre_doc_word_tfidf ? *(float *) pre_doc_word_tfidf : 0.0;

		float centroid_word_tfidf = *(float *) get__hashmap(doc, key, "");

		dot_product += (doc_word_tfidf * centroid_word_tfidf);
	}

	free(centroid_key);
	free(centroid_key_len);

	// use each document magnitude to get the final similarity value
	float similarity = dot_product / (doc_sqrt_mag * centroid_sqrt_mag);

	return similarity;
}

int copy__hashmap(hashmap *m1, hashmap *m2);

float *centroid_mean_calculate(cluster_t** centroids, int start_pos, int k, float* mean_shift, hashmap* doc);
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

typedef struct DocumentVectorHeapRep {
	int curr_cluster, doc_pos_index;
} document_vector_heap_rep_t;

int heap_document_vector_distance_compare(void *d1, void *d2) {
	return *(float *) d1 > *(float *) d2;
}

cluster_t **k_means(hashmap *doc, int k, int cluster_threshold) {
	srand(time(NULL));
	// choosing random k documents

	int *doc_ID_len = malloc(sizeof(int));
	char **doc_ID = (char **) keys__hashmap(doc, doc_ID_len, "");
	// create centroids based off of first k documents
	cluster_t **cluster = malloc(sizeof(cluster_t *) * k);
	for (int create_centroid = 0; create_centroid < k; create_centroid++) {
		hashmap *new_centroid = make__hashmap(0, NULL, destroy_cluster_centroid_data);

		int rand_copy_map_val = rand() % *doc_ID_len;

		hashmap_body_t *copyer_hashmap = get__hashmap(doc, doc_ID[rand_copy_map_val], "");
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

			hashmap_body_t *curr_doc = ((hashmap_body_t *) get__hashmap(doc, doc_ID[find_doc_centroid], ""));
			
			cluster_t *curr_max_centroid = find_closest_cluster(cluster, k, curr_doc);

			// do something with this information...
			curr_max_centroid->doc_pos[curr_max_centroid->doc_pos_index++] = doc_ID[find_doc_centroid];

			// resize check
			if (curr_max_centroid->doc_pos_index == curr_max_centroid->max_doc_pos) {
				curr_max_centroid->max_doc_pos *= 2;

				curr_max_centroid->doc_pos = realloc(curr_max_centroid->doc_pos, sizeof(char *) * curr_max_centroid->max_doc_pos);
			}
		}

		// calculate new averages ([k]means) for each centroid
		centroid_mean_calculate(cluster, 0, k, mean_shifts, doc);

		// setup initial heap for documents with high distances from their centroid
		heap_t* heap_document_high_distance_v_centroid = (heap_t*) heap_create(heap_document_vector_distance_compare);
		int* empty_cluster = malloc(sizeof(int) * (k - 1)); // keep track of empty clusters
		int empty_cluster_index = 0;

		// loop through centroids
		cluster_t* curr_cluster;
		document_vector_t* curr_document_vector;
		for (int look_at_cluster = 0; look_at_cluster < k; look_at_cluster++) {
			curr_cluster = cluster[look_at_cluster];

			// check for empty clusters
			if (curr_cluster->doc_pos_index == 0) {
				empty_cluster[empty_cluster_index] = look_at_cluster;
				empty_cluster_index++;

				continue;
			}

			// look for document vector with highest distance from centroid
			document_vector_heap_rep_t* most_distance_doc_pos = malloc(sizeof(document_vector_heap_rep_t));
			most_distance_doc_pos->curr_cluster = look_at_cluster;
			most_distance_doc_pos->doc_pos_index = 0;

			curr_document_vector = get__hashmap(doc, curr_cluster->doc_pos[0], "");
			float* most_distance_doc_value = malloc(sizeof(float));
			*most_distance_doc_value = cosine_similarity(curr_document_vector->map, curr_document_vector->sqrt_mag,
				curr_cluster->centroid, curr_cluster->sqrt_mag);
			for (int find_distant_doc = 1; find_distant_doc < curr_cluster->doc_pos_index; find_distant_doc++) {
				if (!curr_cluster->doc_pos[find_distant_doc])
					continue;

				curr_document_vector = get__hashmap(doc, curr_cluster->doc_pos[find_distant_doc], "");
				float check_distance_doc_value = cosine_similarity(curr_document_vector->map, curr_document_vector->sqrt_mag,
				curr_cluster->centroid, curr_cluster->sqrt_mag);

				if (check_distance_doc_value > *most_distance_doc_value) {

					most_distance_doc_pos->doc_pos_index = find_distant_doc;
					*most_distance_doc_value = check_distance_doc_value;
				}
			}

			// add the most distant document position and distance value into the heap
			heap_push(heap_document_high_distance_v_centroid, most_distance_doc_pos, most_distance_doc_value);
		}

		// heap is loaded
		// search through empty_cluster for clusters that need documents
		for (int setup_empty_cluster = 0; setup_empty_cluster < empty_cluster_index; setup_empty_cluster++) {
			curr_cluster = cluster[empty_cluster[setup_empty_cluster]];

			document_vector_heap_rep_t* document_vector_to_add_pos = heap_pop(heap_document_high_distance_v_centroid);

			// set position in original cluster to NULL
			char *document_vector_ID = cluster[document_vector_to_add_pos->curr_cluster]->doc_pos[document_vector_to_add_pos->doc_pos_index];
			cluster[document_vector_to_add_pos->curr_cluster]->doc_pos[document_vector_to_add_pos->doc_pos_index] = NULL;

			curr_cluster->doc_pos[0] = document_vector_ID;
			curr_cluster->doc_pos_index++;

			// recalculate centroid means
			centroid_mean_calculate(cluster, empty_cluster[setup_empty_cluster], empty_cluster[setup_empty_cluster] + 1, mean_shifts, doc);
			centroid_mean_calculate(cluster, document_vector_to_add_pos->curr_cluster, document_vector_to_add_pos->curr_cluster + 1, mean_shifts, doc);
		
			free(document_vector_to_add_pos);
		}

		heap_destroy(&heap_document_high_distance_v_centroid);
		free(empty_cluster);
	} while(has_changed(mean_shifts, prev_mean_shifts, k, cluster_threshold));

	free(doc_ID_len);
	free(doc_ID);

	free(mean_shifts);
	free(prev_mean_shifts);

	return cluster;
}

int has_changed(float* mean_shift, float* prev_mean_shift, int k, float threshold) {
	for (int check_mean = 0; check_mean < k; check_mean++) {
		float diff = mean_shift[check_mean] - prev_mean_shift[check_mean];
		if ((diff > 0 && diff > threshold) || (diff < 0 && diff * -1 > threshold))
			return 1;
	}

	return 0;
}

float *centroid_mean_calculate(cluster_t** centroids, int start_pos, int k, float* mean_shift, hashmap* doc) {
	// for each centroid
	for (int find_mean_centroid = start_pos; find_mean_centroid < k; find_mean_centroid++) {
		mean_shift[find_mean_centroid] = 0;
		// calculate a average for each word in centroid
		int cluster_size = centroids[find_mean_centroid]->doc_pos_index;

		int* cluster_word_len = malloc(sizeof(int));
		char** cluster_word = (char**) keys__hashmap(centroids[find_mean_centroid]->centroid, cluster_word_len, "");

		// for each word in cluster, add up tf-idf from each document in cluster and devide by number of documents
		for (int word_mean = 0; word_mean < *cluster_word_len; word_mean++) {
			cluster_centroid_data* centroid_tfidf = get__hashmap(centroids[find_mean_centroid]->centroid, cluster_word[word_mean], "");
			float mean_tfidf = centroid_tfidf->tf_idf;			
			centroid_tfidf->tf_idf = 0;
			centroid_tfidf->doc_freq = 0;

			float numerator = 0;
			float standard_deviation = 0;
			for (int check_doc = 0; check_doc < cluster_size; check_doc++) {
				if (!centroids[find_mean_centroid]->doc_pos[check_doc])
					continue;

				float* doc_tfidf = (float*) get__hashmap(((hashmap_body_t*) get__hashmap(doc,
					centroids[find_mean_centroid]->doc_pos[check_doc], ""))->map, cluster_word[word_mean], "");

				if (!doc_tfidf)
					continue;

				centroid_tfidf->doc_freq++;

				float numerator_addon = *doc_tfidf - mean_tfidf;
				numerator += numerator_addon * numerator_addon;
				centroid_tfidf->tf_idf += *doc_tfidf;
			}

			// calculate rest of standard deviation
			centroid_tfidf->standard_deviation = sqrt(numerator / (cluster_size - 1));

			centroid_tfidf->tf_idf /= cluster_size;
			float tfidf_diff = centroid_tfidf->tf_idf - mean_tfidf;
			mean_shift[find_mean_centroid] += tfidf_diff * tfidf_diff;
		}

		free(cluster_word_len);
		free(cluster_word);
	}

	return mean_shift;
}

// take all values in m2 and copy them into m1
int copy__hashmap(hashmap* m1, hashmap* m2) {
	int *m2_value_len = malloc(sizeof(int));
	char **m2_words = (char **) keys__hashmap(m2, m2_value_len, "");

	for (int cp_value = 0; cp_value < *m2_value_len; cp_value++) {

		float m1_value = *(float *) get__hashmap(m2, m2_words[cp_value], "");

		cluster_centroid_data *new_ccd = create_cluster_centroid_data(m1_value);

		insert__hashmap(m1, m2_words[cp_value], new_ccd, "", compareCharKey, NULL);
	}

	free(m2_value_len);
	free(m2_words);

	return 0;
}

cluster_t *find_closest_cluster(cluster_t **cluster, int k, hashmap_body_t *doc) {
	// find most similar centroid using cosine similarity (largest value returned is most similar):
	int max_centroid = 0;
	float max = cosine_similarity(doc->map, doc->sqrt_mag, cluster[0]->centroid, cluster[0]->sqrt_mag);
	for (int curr_centroid = 1; curr_centroid < k; curr_centroid++) {
		
		float check_max = cosine_similarity(doc->map, doc->sqrt_mag,
			cluster[curr_centroid]->centroid, cluster[curr_centroid]->sqrt_mag);

		if (check_max > max) {
			max = check_max;
			max_centroid = curr_centroid;
		}
	}

	return cluster[max_centroid];
}

 /*
	Pattern for serialization will be:

	cluster_mag doc_amount:doc_id,doc_id,doc_id,
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

		fprintf(reader_file, "%1.8f %d:", curr_cluster->sqrt_mag, curr_cluster->doc_pos_index);

		for (int read_doc = 0; read_doc < curr_cluster->doc_pos_index; read_doc++) {
			if (!curr_cluster->doc_pos[read_doc])
				continue;

			fprintf(reader_file, "%s,", curr_cluster->doc_pos[read_doc]);
		}

		fputc('\n', reader_file);
	}

	fclose(reader_file);

	return 0;
}