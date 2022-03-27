#ifndef __K_MEANS_L__
#define __K_MEANS_L__

#include "document-vector.h"
#include "../serialize/serialize.h"
#include "../utils/hashmap.h"

typedef struct Cluster {
	int max_doc_pos, doc_pos_index;
	char** doc_pos;

	float sqrt_mag;

	hashmap* centroid;
} cluster_t;

typedef struct HashmapData {
	float tf_idf;
	float standard_deviation;
	int doc_freq;
} cluster_centroid_data;

cluster_centroid_data* create_cluster_centroid_data(float data);
void destroy_cluster_centroid_data(void* ccd);

int destroy_cluster(cluster_t** cluster, int k);

cluster_t** k_means(hashmap* doc, int k, int cluster_threshold);
cluster_t* find_closest_cluster(cluster_t** cluster, int k, document_vector_t* doc);

int cluster_to_file(cluster_t** cluster, int k, char* filename);

#endif