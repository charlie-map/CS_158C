#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "kd-tree.h"
#include "../k-means/k-means.h"
#include "../k-means/deserialize.h"
#include "../utils/hashmap.h"

#define K 32
#define CLUSTER_THRESHOLD 2

int weight(void *map1_val, void *map2_val);
void *member_extract(void *map, void *dimension);
void *next_dimension(void *curr_dimension);

hashmap *dimensions;
int build_dimensions(cluster_t *curr_cluster, hashmap *doc_map);

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

	// now with closest cluster, the next step is calculating which document is the closest
	// be wary of the document itself
	// start by creating a k-d tree with the documents in the closest cluster as the inputs
	
	// setup hashmap of important characters
	build_dimensions(closest_cluster, doc_map);

	// build array of documents within the closest cluster:
	hashmap_body_t **cluster_docs = malloc(sizeof(hashmap_body_t *) * closest_cluster->doc_pos_index);
	for (int pull_cluster_doc = 0; pull_cluster_doc < closest_cluster->doc_pos_index; pull_cluster_doc++) {
		cluster_docs[pull_cluster_doc] = (hashmap_body_t *) get__hashmap(doc_map, closest_cluster->doc_pos[pull_cluster_doc], 0);
	}

	kdtree_t *cluster_rep = kdtree_create(weight, member_extract, closest_cluster->doc_pos[0], next_dimension);

	// load k-d tree
	kdtree_load(cluster_rep, cluster_docs, closest_cluster->doc_pos_index);

	destroy_cluster(cluster, K);

	deepdestroy__hashmap(doc_map);

	for (int free_word = 0; free_word < *word_bag_len; free_word++)
		free(word_bag[free_word]);

	free(word_bag_len);
	free(word_bag);

	return 0;
}

hashmap *build_dimensions(cluster_t *curr_cluster) {
	float cluster_size = log(curr_cluster->doc_pos_index);

	int *key_length = malloc(sizeof(int));
	char **keys = keys__hashmap(curr_cluster->centroid, key_length, "");

	for (int read_best = 0; read_best < cluster_size; read_best++) {

		int best_stddev_pos = read_best;
		float best_stddev = ((cluster_centroid_data *) get__hashmap(curr_cluster->centroid, keys[best_key_pos], 0))->standard_deviation;
		for (int find_best_key = read_best + 1; find_best_key < *key_length; find_best_key++) {

			float test_stddev = ((cluster_centroid_data *) get__hashmap(curr_cluster->centroid, keys[find_best_key], 0))->standard_deviation;

			// if test_stddev is greater than best_stddev, update best_stddev_pos and best_stddev
			if (test_stddev > best_stddev) {
				best_stddev = test_stddev;

				best_stddev_pos = find_best_key;
			}
		}

		// swap best_stddev_pos and read_best char * values
		char *key_buffer = keys[read_best];
		keys[read_best] = keys[best_stddev_pos];
		keys[best_stddev_pos] = key_buffer;
	}

	// then build a hashmap that points from one char * to the next, circularly linked
	dimensions = make__hashmap(0, NULL, NULL);

	for (int insert_word = 0; insert_word < *key_length - 1; insert_word++) {
		insert__hashmap(dimensions, keys[insert_word], keys[insert_word + 1], "", compareCharKey, NULL);
	}

	insert__hashmap(dimensions, keys[*key_length - 1], keys[0], "", compareCharKey, NULL);

	return 0;
}

int weight(void *map1_val, void *map2_val) {
	return *(float *) map1_val < *(float *) map2_val;
}

void *member_extract(void *map_body, void *dimension) {
	return get__hashmap(((hashmap *) map_body)->map, (char *) dimension, 0);
}

void *next_dimension(void *curr_dimension) {
	// curr dimension is a char * that searches into a hashmap for the next value
	// this hashmap has each char * pointing to the next, which allows for the
	// dimensions to be based on an initial weighting from the cluster centroid
	return get__hashmap(dimensions, (char *) curr_dimensions, 0);
}

/* search:
is the square dist between current best's dimension X and this split's dimension X greater
	or less than the overall dist between current and search term?
----If it's greater
	bail and do not recur
----If it's less
	you can determine nothing, so recur
*/