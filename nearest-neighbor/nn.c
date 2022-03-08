#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "../k-means/k-means.h"
#include "../k-means/deserialize.h"
#include "../utils/hashmap.h"

#define K 32
#define CLUSTER_THRESHOLD 2

int main() {
	// see k-means folder for more on these functions
	hashmap *doc_map = deserialize_title("../serialize/title.txt");
	int *word_bag_len = malloc(sizeof(int));
	char **word_bag = deserialize("../serialize/predocbags.txt", doc_map, word_bag_len);

	cluster_t **cluster = k_means(doc_map, K, CLUSTER_THRESHOLD);

	int *doc_map_length = malloc(sizeof(int));
	char **doc_map_keys = (char **) keys__hashmap(doc_map, doc_map_length, "");

	// pick random doc and compute nearest neighbor
	srand(time(NULL));

	int rand_doc_pos = rand() % *doc_map_length;
	char *rand_doc_key = doc_map_keys[rand_doc_pos];

	hashmap_body_t *rand_doc = (hashmap_body_t *) get__hashmap(doc_map, rand_doc_key, 0);
	printf("grabbed: %s\n", rand_doc->title);

	free(doc_map_length);
	free(doc_map_keys);

	cluster_t *closest_cluster = find_closest_cluster(cluster, K, rand_doc);

	printf("CLOSEST CLUSTER:\n");
	for (int read_cluster_doc = 0; read_cluster_doc < closest_cluster->doc_pos_index; read_cluster_doc++) {
		char *title = ((hashmap_body_t *) get__hashmap(doc_map, closest_cluster->doc_pos[read_cluster_doc], 0))->title;
		printf("Document %s: %s\n", closest_cluster->doc_pos[read_cluster_doc], title);
	}

	destroy_cluster(cluster, K);

	deepdestroy__hashmap(doc_map);

	for (int free_word = 0; free_word < *word_bag_len; free_word++)
		free(word_bag[free_word]);

	free(word_bag_len);
	free(word_bag);

	return 0;
}