#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trie.h"
#include "token.h"
#include "request.h"
#include "hashmap.h"
#include "serialize.h"

#define HOST "cutewiki.charlie.city"
#define PORT "8822"

#define REQ_NAME "permission_data_pull"
#define REQ_PASSCODE "d6bc639b-8235-4c0d-82ff-707f9d47a4ca"

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
	FILE *index_writer = fopen("docbags.txt", "w");
	FILE *title_writer = fopen("title.txt", "w");
	socket_t *sock_data = get_socket(HOST, PORT);

	res *response = send_req(sock_data, "/pull_page_names", "GET", "-q", "?name=$&passcode=$", REQ_NAME, REQ_PASSCODE);

	// pull array
	int *array_length = malloc(sizeof(int));
	char **array_body = handle_array(res_body(response), array_length);

	printf("\nCurrent wiki IDs: %d\n", *array_length);
	for (int print_array = 0; print_array < *array_length; print_array++) {
		printf("id: %s\n", array_body[print_array]);
		res *wiki_page = send_req(sock_data, "/pull_data", "POST", "-q-b", "?name=$&passcode=$", REQ_NAME, REQ_PASSCODE, "unique_id=$", array_body[print_array]);

		// parse the wiki data and write to the bag of words
		token_t *new_wiki_page_token = tokenize('s', res_body(wiki_page));

		// check title for any extra components:
		token_t *check_title_token = grab_token_by_tag(new_wiki_page_token, "title");
		char *curr_title_value = data_at_token(check_title_token);
		if (!strlen(curr_title_value)) {
			printf("now search\n");
			// check for a child:
			int *title_max_length = malloc(sizeof(int));
			char *new_title = token_read_all_data(check_title_token, title_max_length, NULL, NULL);

			printf("get new title: %s\n", new_title);

			if (strlen(new_title)) // add it!
				update_token_data(check_title_token, new_title, title_max_length);
			else {// bad data
				printf("An error occured with the title of document: %d\n", print_array);
				exit(1);
			}

			free(title_max_length);
			free(new_title);
		}
		printf("check title: %s\n", data_at_token(check_title_token));

		int finish = word_bag(index_writer, title_writer, stopword_trie, new_wiki_page_token);

		if (finish < 0) {
			printf("\nWRITE ERR\n");
			return 1;
		}

		destroy_token(new_wiki_page_token);
		res_destroy(wiki_page);

		free(array_body[print_array]);
	}

	free(array_body);
	free(array_length);

	res_destroy(response);

	destroy_socket(sock_data);
	trie_destroy(stopword_trie);

	return 0;
}