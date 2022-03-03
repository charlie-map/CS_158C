#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "k-means.h"
#include "deserialize.h"
#include "../utils/hashmap.h"

#define THREAD_NUMBER 8

#define K 8
#define DTF_THRESHOLD 0
#define CLUSTER_THRESHOLD 2

int main() {
	
	
	// now we have bags for all terms, and the length of each bag of terms
	// we can go back through the writer again and pull each document out one at a time
	hashmap_body_t **feature_space = deserialize("../serialize/predocbags.txt", DTF_THRESHOLD);

	// start k-means to calculate clusters
	// cluster_t **cluster = k_means(feature_space, *array_length, idf, K, CLUSTER_THRESHOLD);

	// for (int check_cluster = 0; check_cluster < K; check_cluster++) {
	// 	printf("-----Check docs on %d-----\n", check_cluster + 1);

	// 	for (int read_cluster_doc = 0; read_cluster_doc < cluster[check_cluster]->doc_pos_index; read_cluster_doc++) {
	// 		printf("ID: %s\n", feature_space[cluster[check_cluster]->doc_pos[read_cluster_doc]]->id);
	// 	}
	// }

	// destroy_cluster(cluster, K);

	// for (int f_feature_space = 0; f_feature_space < *array_length; f_feature_space++) {
	// 	free(all_IDs[f_feature_space]);
	// 	destroy_hashmap_body(feature_space[f_feature_space]);
	// }

	free(feature_space);

	return 0;
}