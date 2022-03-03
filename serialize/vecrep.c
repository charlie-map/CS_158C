#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "trie.h"
#include "token.h"
#include "../utils/request.h"
#include "../utils/hashmap.h"
#include "serialize.h"

#define HOST getenv("WIKIREAD_HOST")
#define PORT getenv("WIKIREAD_PORT")

#define REQ_NAME getenv("WIKIREAD_REQ_NAME")
#define REQ_PASSCODE getenv("WIKIREAD_REQ_PASSCODE")

#define THREAD_NUMBER 8

#define K 8
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
	pthread_mutex_t *sock_mutex;

	trie_t *stopword_trie;

	mutex_t *term_freq; // has mutex_t for each word (crayyyyy)
	mutex_t *title_writer; // FILE *title_writer

	int start_read_body;
	int end_read_body;
} serialize_t;

serialize_t *create_serializer(char **all_IDs, char **array_body, int *array_length,
	socket_t **sock_data, pthread_mutex_t *sock_mutex, trie_t *stopword_trie,
	mutex_t *term_freq, mutex_t *title_writer,
	int start_read_body, int end_read_body) {

	serialize_t *new_ser = malloc(sizeof(serialize_t));

	new_ser->all_IDs = all_IDs;
	new_ser->array_body = array_body;
	new_ser->array_length = array_length;

	new_ser->sock_data = sock_data;
	new_ser->sock_mutex = sock_mutex;

	new_ser->stopword_trie = stopword_trie;

	new_ser->term_freq = term_freq;
	new_ser->title_writer = title_writer;

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

	socket_t *sock_data = get_socket(HOST, PORT);

	// calculating term frequency for each term
	hashmap *term_freq = make__hashmap(0, NULL, destroy_tf_t);

	// initial header request
	res *response = send_req(sock_data, "/pull_page_names", "POST", "-b", "name=$&passcode=$", REQ_NAME, REQ_PASSCODE);

	// pull array
	int *array_length = malloc(sizeof(int));
	char **array_body = handle_array(res_body(response), array_length);

	char **all_IDs = malloc(sizeof(char *) * *array_length);
	printf("\nCurrent wiki IDs: %d\n", *array_length);
	// loop pages and pull

	// create pthreads to get data faster
	pthread_t *p_thread = malloc(sizeof(pthread_t) * THREAD_NUMBER);

	// calculate diff between array_length and THREAD_NUMBER
	// to find how many documents each thread ripper should have
	int doc_per_thread = floor(*array_length / THREAD_NUMBER);

	// setup mutexes for any params that need a shared mutex:
	/* e.g.:
		socket, idf, index_writer, and title_writer
	*/
	printf("%d\n", THREAD_NUMBER / 2);
	pthread_mutex_t **sock_mutex = malloc(sizeof(pthread_mutex_t *) * (THREAD_NUMBER / 2));
	socket_t ***socket_holder = malloc(sizeof(socket_t **) * (THREAD_NUMBER / 2));
	socket_holder[0] = malloc(sizeof(socket_t *));
	*(socket_holder[0]) = sock_data;

	for (int create_sock_mutex = 0; create_sock_mutex < THREAD_NUMBER / 2; create_sock_mutex++) {
		sock_mutex[create_sock_mutex] = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(sock_mutex[create_sock_mutex], NULL);

		if (create_sock_mutex > 0) {
			socket_holder[create_sock_mutex] = malloc(sizeof(socket_t *));
			*(socket_holder[create_sock_mutex]) = get_socket(HOST, PORT);
		}
	}

	mutex_t *term_freq_mutex = malloc(sizeof(mutex_t));
	*term_freq_mutex = newMutexLocker(term_freq);
	mutex_t *title_writer_mutex = malloc(sizeof(mutex_t));
	*title_writer_mutex = newMutexLocker(title_writer);

	serialize_t **serializers = malloc(sizeof(serialize_t *) * THREAD_NUMBER);

	for (int thread_rip = 0; thread_rip < THREAD_NUMBER; thread_rip++) {
		serializers[thread_rip] = create_serializer(all_IDs, array_body, array_length, socket_holder[thread_rip / 3],
			sock_mutex[thread_rip / 3], stopword_trie, term_freq_mutex,
			title_writer_mutex, thread_rip * doc_per_thread,
			thread_rip == THREAD_NUMBER - 1 ? *array_length : (thread_rip + 1) * doc_per_thread);

		pthread_create(&p_thread[thread_rip], NULL, data_read, serializers[thread_rip]);
	}

	// rejoin threads
	for (int rejoin_thread = 0; rejoin_thread < THREAD_NUMBER; rejoin_thread++) {
		pthread_join(p_thread[rejoin_thread], NULL);

		free(serializers[rejoin_thread]);
	}

	free(serializers);
	free(term_freq_mutex);

	free(p_thread);
	free(array_body);

	res_destroy(response);

	free(title_writer_mutex);
	fclose(title_writer);

	trie_destroy(stopword_trie);

	/* SAVE term freq and idf AND CREATE A DIFFERENT PROGRAM FOR BELOW */
	// in form:
		// word DF:ID,freq|ID,freq|\n
	int *word_len = malloc(sizeof(int));
	char **words = (char **) keys__hashmap(term_freq, word_len, "");

	for (int fp_word = 0; fp_word < *word_len; fp_word++) {
		tf_t *dat = get__hashmap(term_freq, words[fp_word], 0);

		int doc_freq_len = (int) log10(dat->doc_freq) + 2;
		char *doc_freq_str = malloc(sizeof(char) * doc_freq_len);
		sprintf(doc_freq_str, "%d", dat->doc_freq);

		fputs(words[fp_word], index_writer);
		fputc(' ', index_writer);
		fputs(doc_freq_str, index_writer);

		free(doc_freq_str);

		fputc(':', index_writer);
		fputs(dat->full_rep, index_writer);
		fputc('\n', index_writer);
	}
	printf("%d\n", *word_len);

	free(word_len);
	free(words);
	fclose(index_writer);

	deepdestroy__hashmap(term_freq);

	// free data:
	for (int del_id = 0; del_id < *array_length; del_id++) {
		free(all_IDs[del_id]);
	}
	free(all_IDs);

	free(array_length);

	// free sockets
	for (int free_socket = 0; free_socket < THREAD_NUMBER / 2; free_socket++) {
		free(sock_mutex[free_socket]);

		destroy_socket(*(socket_holder[free_socket]));
		free(socket_holder[free_socket]);
	}

	free(sock_mutex);
	free(socket_holder);

	return 0;

	// now we have idf for all terms, and the length of each bag of terms
	// we can go back through the writer again and pull each document out one
	// at a time and recalculate each term frequency with the new idf value
	// FILE *old_reader = fopen("predocbags.txt", "r");
	// hashmap_body_t **feature_space = word_bag_idf(old_reader, idf, doc_bag_length, *array_length, DTF_THRESHOLD);

	// fclose(old_reader);

	// // start k-means to calculate clusters
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

	// free(feature_space);

	// return 0;
}

/*
	META_PTR:
		-- char **all_IDs
		-- char **array_body
		-- int *array_length

		-- socket_t **sock_data (requires mutex locking)

		-- hashmap *term_freq (requires mutex locking)
		-- FILE *title_writer (requires mutex locking)

		-- int *doc_bag_length

		-- int start_read_body
		-- int end_read_body
*/
void *data_read(void *meta_ptr) {
	serialize_t *ser_pt = (serialize_t *) meta_ptr;

	// redeclare each component of ser_pt as single variable for ease of comprehensibility
	char **all_IDs = ser_pt->all_IDs;
	char **array_body = ser_pt->array_body;
	int *array_length = ser_pt->array_length;

	trie_t *stopword_trie = ser_pt->stopword_trie;

	int start_read_body = ser_pt->start_read_body;
	int end_read_body = ser_pt->end_read_body;

	for (int read_body = start_read_body; read_body < end_read_body; read_body++) {
		printf("lock %d\n", read_body);
		pthread_mutex_lock(ser_pt->sock_mutex);
		printf("****locked %d\n", read_body);

		res *wiki_page = send_req(*(ser_pt->sock_data), "/pull_data", "POST", "-b", "unique_id=$&name=$&passcode=$", array_body[read_body], REQ_NAME, REQ_PASSCODE);
		if (!wiki_page) { // socket close!
			// reset socket:

			destroy_socket(*(ser_pt->sock_data));
			*(ser_pt->sock_data) = get_socket(HOST, PORT);

			// repeat data collection
			pthread_mutex_unlock(ser_pt->sock_mutex);
			read_body--;
			continue;
		}
		pthread_mutex_unlock(ser_pt->sock_mutex);

		// printf("CHECK: %s\n", res_body(wiki_page));
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

		pthread_mutex_lock(&(ser_pt->term_freq->mutex));
		int new_doc_length = word_bag(ser_pt->term_freq->runner, ser_pt->title_writer, stopword_trie, new_wiki_page_token, &all_IDs[read_body]);
		
		if (new_doc_length < 0) {
			printf("\nWRITE ERR\n");
			exit(1);
		}

		pthread_mutex_unlock(&(ser_pt->term_freq->mutex));

		destroy_token(new_wiki_page_token);
		res_destroy(wiki_page);

		free(array_body[read_body]);
	}

	return NULL;
}