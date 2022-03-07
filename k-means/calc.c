#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "k-means.h"
#include "deserialize.h"
#include "../utils/hashmap.h"

#define THREAD_NUMBER 8

#define K 8
#define CLUSTER_THRESHOLD 2

int main() {
	
	
	// now we have bags for all terms, and the length of each bag of terms
	// we can go back through the writer again and pull each document out one at a time
	hashmap *doc_map = deserialize_title("../serialize/title.txt");
	int *word_bag_len = malloc(sizeof(int));
	char **word_bag = deserialize("../serialize/predocbags.txt", doc_map, word_bag_len);

	// start k-means to calculate clusters
	// cluster_t **cluster = k_means(doc_map, K, CLUSTER_THRESHOLD);

	// for (int check_cluster = 0; check_cluster < K; check_cluster++) {
	// 	printf("-----Check docs on %d-----\n", check_cluster + 1);

	// 	for (int read_cluster_doc = 0; read_cluster_doc < cluster[check_cluster]->doc_pos_index; read_cluster_doc++) {
	// 		printf("ID: %s\n", cluster[check_cluster]->doc_pos[read_cluster_doc]);
	// 	}
	// }

	// destroy_cluster(cluster, K);

	// for (int f_feature_space = 0; f_feature_space < *array_length; f_feature_space++) {
	// 	free(all_IDs[f_feature_space]);
	// 	destroy_hashmap_body(feature_space[f_feature_space]);
	// }

	// free(feature_space);

	deepdestroy__hashmap(doc_map);

	for (int free_word = 0; free_word < *word_bag_len; free_word++)
		free(word_bag[free_word]);

	free(word_bag_len);
	free(word_bag);

	return 0;
}