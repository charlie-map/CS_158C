#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

	// calculate idf for each term
	hashmap *idf = make__hashmap(0, NULL, hashmap_destroy_idf);
	int *doc_bag_length = malloc(sizeof(int) * 8);
	int doc_bag_length_max = 8, index_doc_bag = 0;

	// initial header request
	res *response = send_req(sock_data, "/pull_page_names", "GET", "-q", "?name=$&passcode=$", REQ_NAME, REQ_PASSCODE);

	// pull array
	int *array_length = malloc(sizeof(int));
	char **array_body = handle_array(res_body(response), array_length);

	printf("\nCurrent wiki IDs: %d\n", *array_length);
	// loop pages and pull
	for (int print_array = 0; print_array < 5; print_array++) {
		printf("id: %s\n", array_body[print_array]);
		res *wiki_page = send_req(sock_data, "/pull_data", "POST", "-q-b", "?name=$&passcode=$", REQ_NAME, REQ_PASSCODE, "unique_id=$", array_body[print_array]);

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
				printf("An error occured with the title of document: %d\n", print_array);
				exit(1);
			}

			free(title_max_length);
			free(new_title);
		}

		doc_bag_length[index_doc_bag++] = word_bag(index_writer, title_writer, stopword_trie, new_wiki_page_token, idf);

		if (doc_bag_length[index_doc_bag - 1] < 0) {
			printf("\nWRITE ERR\n");
			return 1;
		}

		// check resize:
		if (index_doc_bag == doc_bag_length_max) {
			doc_bag_length_max *= 2;
			doc_bag_length = realloc(doc_bag_length, sizeof(int) * doc_bag_length_max);
		}

		destroy_token(new_wiki_page_token);
		res_destroy(wiki_page);

		free(array_body[print_array]);
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
	cluster_t **cluster = k_means(feature_space, index_doc_bag, idf, 2, CLUSTER_THRESHOLD);

	for (int check_cluster = 0; check_cluster < 2; check_cluster++) {
		printf("-----Check docs on %d-----\n", check_cluster + 1);

		for (int read_cluster_doc = 0; read_cluster_doc < cluster[check_cluster]->doc_pos_index; read_cluster_doc++) {
			printf("ID: %s\n", feature_space[cluster[check_cluster]->doc_pos[read_cluster_doc]]->id);
		}
	}

	// free data:
	for (int f_feature_space = 0; f_feature_space < index_doc_bag; f_feature_space++) {
		destroy_hashmap_body(feature_space[f_feature_space]);
	}

	free(feature_space);
	free(doc_bag_length);

	deepdestroy__hashmap(idf);

	return 0;
}