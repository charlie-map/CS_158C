#ifndef __K_MEANS_L__
#define __K_MEANS_L__

#include "serialize.h"
#include "hashmap.h"

typedef struct Cluster {
	int max_doc_pos, doc_pos_index;
	int *doc_pos;

	float sqrt_mag;

	hashmap *centroid;
} cluster_t;

cluster_t **k_means(hashmap_body_t **doc, int doc_len, hashmap *idf, int k, int cluster_threshold);

#endif