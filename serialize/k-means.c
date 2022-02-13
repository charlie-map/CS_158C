#include <stdio.h>
#include <stdlib.h>

#include "k-means.h"
#include "serialize.h"

// calculate the distance by going through the centroids word map
// if the doc doesn't have the word (or vice-versa) then the dot-product
// will be 0
// the close the return value is to 1, the more closely related
// the two documents are
float cosine_similarity(hashmap *doc, hashmap *centroid) {
	int *centroid_key_len = malloc(sizeof(int));
	char **centroid_key = keys__hashmap(centroid, centroid_key_len);

	float dot_product = 0;
	for (int calc_dp = 0; calc_dp < *centroid_key_len; calc_dp++) {
		// grab value from each map
		void *pre_doc_word_tfidf = get__hashmap(doc, centroid_key[calc_dp], 0);
		int doc_word_tfidf = pre_doc_word_tfidf ? *(int *) pre_doc_word_tfidf : 0;
		int centroid_word_tfidf = *(int *) get__hashmap(centroid, centroid_key[calc_dp], 0);

		dot_product += (doc_word_tfidf * centroid_word_tfidf) * 1.0;
	}

	// use each document magnitude to get the final similarity value
	float similarity = dot_product / (doc->sqrt_mag * doc->sqrt_mag);

	return similarity;
}

cluster_t *k_means(hashmap_body_t **doc, int doc_len, hashmap *idf, int k) {
	// will choose the first k documents as centroids :D

	// go through non-centroid documents and assign them to centroids
	for (int find_doc_centroid = k; find_doc_centroid < doc_len; find_doc_centroid++) {

		// find most similar centroid using cosine similarity (largest value returned is most similar):
		int max_centroid = 0, curr_centroid;
		float max = cosine_similarity(doc[find_doc_centroid]->map, doc[0]->map);
		for (curr_centroid = 1; curr_centroid < k; curr_centroid++) {
			
			float check_min = cosine_similarity(doc[find_doc_centroid]->map, doc[curr_centroid]->map);

			if (check_min < max) {
				max = check_min;
				max_centroid = curr_centroid;
			}
		}

		// do something with this information...
	}
}