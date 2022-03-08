#ifndef __K_MEANS_L__
#define __K_MEANS_L__

#include "../serialize/serialize.h"
#include "../utils/hashmap.h"
#include "deserialize.h"

typedef struct Cluster {
	int max_doc_pos, doc_pos_index;
	char **doc_pos;

	float sqrt_mag;

	hashmap *centroid;
} cluster_t;

int destroy_cluster(cluster_t **cluster, int k);

cluster_t **k_means(hashmap *doc, int k, int cluster_threshold);
cluster_t *find_closest_cluster(cluster_t **cluster, int k, hashmap_body_t *doc);

#endif