#ifndef __K_MEANS_L__
#define __K_MEANS_L__

typedef struct Cluster {
	int max_doc_pos, doc_pos_index;
	int *doc_pos;

	hashmap_body_t *centroid;
} cluster_t;

cluster_t *k_means(hashmap_body_t **doc, int doc_len, hashmap *idf, int k);

#endif