#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "trie.h"
#include "token.h"
#include "k-means.h"
#include "request.h"
#include "hashmap.h"
#include "serialize.h"

#define HOST "cutewiki.charlie.city"
#define PORT "8822"

#define REQ_NAME "permission_data_pull"
#define REQ_PASSCODE "d6bc639b-8235-4c0d-82ff-707f9d47a4ca"

#define THREAD_NUMBER 8

#define K 4
#define DTF_THRESHOLD 0
#define CLUSTER_THRESHOLD 2

trie_t *fill_stopwords(char *stop_word_file) {
	trie_t *trie = trie_create("-pc");

	FILE *stop_fp = fopen(stop_word_file, "r");

	if (!stop_fp) {
		printf("Error with loading stopword from file: %s\n", stop_word_file);
		exit(1);
	}

	size_t word_len = 16 * sizeof(char);
	char *word = malloc(word_len);
	while (getline(&word, &word_len, stop_fp) != -1)
		trie_insert(trie, word);
	
	free(word);
	fclose(stop_fp);

	return trie;
}

typedef struct SerializeObject {
	char **all_IDs;
	char **array_body;
	int *array_length;

	socket_t **sock_data;

	mutex_t idf; // hashmap *idf
	mutex_t index_writer; // FILE *index_writer
	mutex_t title_writer; // FILE *title_writer

	mutex_t doc_bag_length; // int *doc_bag_length
	mutex_t index_doc_bag;  // int *index_doc_bag

	int start_read_body;
	int end_read_body;
} serialize_t;

serialize_t *create_serializer(char **all_IDs, char **array_body, int *array_length,
	socket_t **sock_data, hashmap *idf, FILE *index_writer, FILE *title_writer,
	int *doc_bag_length, int *index_doc_bag, int start_read_body, int end_read_body) {

	serialize_t *new_ser = malloc(sizeof(serialize_t));

	new_ser->all_IDs = all_IDs;
	new_ser->array_body = array_body;
	new_ser->array_length = array_length;

	new_ser->sock_data = sock_data;

	new_ser->idf = { .runner = idf, .mutex = PTHREAD_MUTEX_INITIALIZER };

	new_ser->index_writer = { .runner = index_writer, .mutex = PTHREAD_MUTEX_INITIALIZER };
	new_ser->title_writer = { .runner = title_writer, .mutex = PTHREAD_MUTEX_INITIALIZER };

	new_ser->doc_bag_length = { .runner = doc_bag_length, .mutex = PTHREAD_MUTEX_INITIALIZER };
	new_ser->index_doc_bag =  { .runner = index_doc_bag,  .mutex = PTHREAD_MUTEX_INITIALIZER };

	new_ser->start_read_body = start_read_body;
	new_ser->end_read_body = end_read_body;

	return new_ser;
}

void *data_read(void *meta_ptr);

int main() {
	// create stopword structure
	trie_t *stopword_trie = fill_stopwords("stopwords.txt");

	// create write stream:
	FILE *index_writer = fopen("predocbags.txt", "w");
	FILE *title_writer = fopen("title.txt", "w");

	if (!index_writer || !title_writer) {
		printf("File loading for index or title file didn't work! Try again\n");
		exit(1);
	}

	socket_t **sock_data = malloc(sizeof(socket_t *));
	*sock_data = get_socket(HOST, PORT);

	// calculate idf for each term
	hashmap *idf = make__hashmap(0, NULL, hashmap_destroy_idf);

	// initial header request
	res *response = send_req(*sock_data, "/pull_page_names", "GET", "-q", "?name=$&passcode=$", REQ_NAME, REQ_PASSCODE);

	// pull array
	int *array_length = malloc(sizeof(int));
	char **array_body = handle_array(res_body(response), array_length);

	// each document length (in serialized form)
	int *doc_bag_length = malloc(sizeof(int) * *array_length);
	int *index_doc_bag = malloc(sizeof(int)); // pointer to current position in doc_bag_length
	*index_doc_bag = 0;

	char **all_IDs = malloc(sizeof(char *) * *array_length);
	printf("\nCurrent wiki IDs: %d\n", *array_length);
	// loop pages and pull

	// create pthreads to get data faster
	pthread_t *p_thread = malloc(sizeof(pthread_t) * THREAD_NUMBER);

	// calculate diff between array_length and THREAD_NUMBER
	// to find how many documents each thread ripper should have
	int doc_per_thread = floor(*array_length / THREAD_NUMBER);

	for (int thread_rip = 0; thread_rip < THREAD_NUMBER; thread_rip++) {
		serializer_t *new_serializer = create_serializer(all_IDs, array_body, array_length, sock_data,
			idf, index_writer, title_writer, thread_rip * doc_per_thread,
			thread_rip == THREAD_NUMBER - 1 ? *array_length : (thread_rip + 1) * doc_per_thread);

		pthread_create(&p_thread[thread_rip], NULL, data_read, new_serializer);
	}

	// rejoin threads
	for (int rejoin_thread = 0; rejoin_thread < THREAD_NUMBER; rejoin_thread++) {
		pthread_join(p_thread[rejoin_thread], NULL);
	}

	free(array_body);
	free(array_length);

	res_destroy(response);

	fclose(index_writer);
	fclose(title_writer);

	destroy_socket(sock_data);
	trie_destroy(stopword_trie);

	// now we have idf for all terms, and the length of each bag of terms
	// we can go back through the writer again and pull each document out one
	// at a time and recalculate each term frequency with the new idf value
	FILE *old_reader = fopen("predocbags.txt", "r");
	hashmap_body_t **feature_space = word_bag_idf(old_reader, idf, doc_bag_length, index_doc_bag, DTF_THRESHOLD);

	fclose(old_reader);

	// start k-means to calculate clusters
	cluster_t **cluster = k_means(feature_space, index_doc_bag, idf, K, CLUSTER_THRESHOLD);

	for (int check_cluster = 0; check_cluster < K; check_cluster++) {
		printf("-----Check docs on %d-----\n", check_cluster + 1);

		for (int read_cluster_doc = 0; read_cluster_doc < cluster[check_cluster]->doc_pos_index; read_cluster_doc++) {
			printf("ID: %s\n", feature_space[cluster[check_cluster]->doc_pos[read_cluster_doc]]->id);
		}
	}

	destroy_cluster(cluster, K);

	// free data:
	for (int f_feature_space = 0; f_feature_space < index_doc_bag; f_feature_space++) {
		free(all_IDs[f_feature_space]);
		destroy_hashmap_body(feature_space[f_feature_space]);
	}

	free(feature_space);
	free(doc_bag_length);

	free(all_IDs);

	deepdestroy__hashmap(idf);

	return 0;
}

/*
	META_PTR:
		-- char **all_IDs
		-- char **array_body
		-- int *array_length

		-- socket_t **sock_data

		-- hashmap *idf (requires mutex locking)
		-- FILE *index_writer (requires mutex locking)
		-- FILE *title_writer (requires mutex locking)

		-- int *doc_bag_length (requires mutex locking)
		-- int *index_doc_bag (requires mutex locking)

		-- int start_read_body
		-- int end_read_body
*/
void *data_read(void *meta_ptr) {
	serialize_t *ser_pt = (serialize_t *) meta_ptr;

	// redeclare each component of ser_pt as single variable for ease of comprehensibility
	char **all_IDs = ser_pt->all_IDs;
	char **array_body = ser_pt->array_body;
	int *array_length = ser_pt->array_length;

	socket_t **sock_data = ser_pt->sock_data;

	hashmap *idf = ser_pt->idf;
	FILE *index_writer = ser_pt->index_writer;
	FILE *title_writer = ser_pt->title_writer;

	int *doc_bag_length = ser_pt->doc_bag_length;
	int *index_doc_bag = ser_pt->index_doc_bag;

	int start_read_body = ser_pt->start_read_body;
	int end_read_body = ser_pt->end_read_body;

	for (int read_body = start_read_body; read_body < end_read_body; read_body++) {
		printf("id: %s\n", array_body[read_body]);
		res *wiki_page = send_req(*sock_data, "/pull_data", "POST", "-q-b", "?name=$&passcode=$", REQ_NAME, REQ_PASSCODE, "unique_id=$", array_body[read_body]);

		if (!wiki_page) { // socket close!
			// reset socket:

			destroy_socket(*sock_data);
			*sock_data = get_socket(HOST, PORT);

			// repeat data collection
			read_body--;
			continue;
		}

		// parse the wiki data and write to the bag of words
		token_t *new_wiki_page_token = tokenize('s', res_body(wiki_page));

		// check title for any extra components:
		token_t *check_title_token = grab_token_by_tag(new_wiki_page_token, "title");
		char *curr_title_value = data_at_token(check_title_token);
		if (!strlen(curr_title_value)) {
			// check for a child:
			int *title_max_length = malloc(sizeof(int));
			char *new_title = token_read_all_data(check_title_token, title_max_length, NULL, NULL);

			if (strlen(new_title)) // add it!
				update_token_data(check_title_token, new_title, title_max_length);
			else {// bad data
				printf("An error occured with the title of document: %d\n", read_body);
				exit(1);
			}

			free(title_max_length);
			free(new_title);
		}

		int new_doc_length = word_bag(index_writer, title_writer, stopword_trie, new_wiki_page_token, idf, &all_IDs[read_body]);
		
		pthread_mutex_lock(&(ser_pt->doc_bag_length->mutex));
		pthread_mutex_lock(&(ser_pt->doc_bag_length->mutex));
		doc_bag_length[index_doc_bag++] = new_doc_length;
		pthread_mutex_unlock(&(ser_pt->doc_bag_length->mutex));
		pthread_mutex_unlock(&(ser_pt->doc_bag_length->mutex));

		if (doc_bag_length[index_doc_bag - 1] < 0) {
			printf("\nWRITE ERR\n");
			exit(1);
		}

		destroy_token(new_wiki_page_token);
		res_destroy(wiki_page);

		free(array_body[read_body]);
	}

	return NULL;
}