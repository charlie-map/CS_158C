#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "k-means.h"
#include "deserialize.h"
#include "document-vector.h"
#include "../utils/hashmap.h"

#define K 48
#define CLUSTER_THRESHOLD 2

int main() {
	
	// now we have bags for all terms, and the length of each bag of terms
	// we can go back through the writer again and pull each document out one at a time
	hashmap *doc_map = make__hashmap(0, NULL, hm_destroy_hashmap_body);
	hashmap *term_freq_map = make__hashmap(0, NULL, destroy_tf_t);

	int* ID_len = malloc(sizeof(int)), ID_index = 0; *ID_len = 8;
	char** ID = malloc(sizeof(char *) * *ID_len);
	deserialize_title("../serialize/title.txt", doc_map, &ID, ID_len);
	int *word_bag_len = malloc(sizeof(int));
	char **word_bag = deserialize("../serialize/predocbags.txt", term_freq_map, doc_map, word_bag_len);

	// start k-means to calculate clusters
	cluster_t **cluster = k_means(doc_map, K, CLUSTER_THRESHOLD);

	for (int check_cluster = 0; check_cluster < K; check_cluster++) {
		printf("-----Check docs on %d-----\n", check_cluster + 1);

		for (int read_cluster_doc = 0; read_cluster_doc < cluster[check_cluster]->doc_pos_index; read_cluster_doc++) {
			if (!cluster[check_cluster]->doc_pos[read_cluster_doc])
				continue;

			char *title = ((document_vector_t*) get__hashmap(doc_map, cluster[check_cluster]->doc_pos[read_cluster_doc], ""))->title;
			printf("Document %s: %s\n", cluster[check_cluster]->doc_pos[read_cluster_doc], title);
		}
	}

	// save documents to cluster.txt
	cluster_to_file(cluster, K, "cluster.txt");

	destroy_cluster(cluster, K);

	deepdestroy__hashmap(doc_map);
	deepdestroy__hashmap(term_freq_map);

	for (int free_word = 0; free_word < *word_bag_len; free_word++)
		free(word_bag[free_word]);

	free(word_bag_len);
	free(word_bag);
	free(ID_len);
	free(ID);

	return 0;
}