#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "serialize.h"
#include "../stemmer.h"

struct IDF_Track {
	int prev_idf_index; // last document to be added (for checking dupes)
	int document_freq; // occurrences of documents that contain term
};

void hashmap_destroy_idf(void *p) {
	free((idf_t *) p);

	return;
}

// create index structure
int delimeter_check(char curr_char, char *delims) {
	for (int check_delim = 0; delims[check_delim]; check_delim++) {
		if (delims[check_delim] == curr_char)
			return 1;
	}

	return 0;
}

/*
	goes through a char * to create an array of char * that are split
	based on the delimeter character. If the string was:

	char *example = "Hi, I am happy, as of now, thanks";

	then using split_string(example, ','); would return:

	["Hi", "I am happy", "as of now", "thanks"]

	-- UPDATE: 
	char *param:
		place directly after arr_len,
		-- uses a pattern to signify more parameters
	int **minor_length:
		stores the length of each position in the returned char **
		use: "-l" to access push in minor_length
		-- only used if passed, otherwise null
		-- int ** will have same length as char **
	int (*is_delim)(char delim, char *delimeters):
		gives the options to instead have multi delimeters
		use: "-d" to have is_delim and char *delimeters passed in
*/
char **split_string(char *full_string, char delimeter, int *arr_len, char *extra, ...) {
	va_list param;
	va_start(param, extra);

	int **minor_length = NULL;
	int (*is_delim)(char, char *) = NULL;
	char *multi_delims = NULL;

	for (int check_extra = 0; extra[check_extra]; check_extra++) {
		if (extra[check_extra] != '-')
			continue;

		if (extra[check_extra + 1] == 'l')
			minor_length = va_arg(param, int **);
		else if (extra[check_extra + 1] == 'd') {
			is_delim = va_arg(param, int (*)(char, char *));
			multi_delims = va_arg(param, char *);
		}
	}

	int arr_index = 0;
	*arr_len = 8;
	char **arr = malloc(sizeof(char *) * *arr_len);

	int *max_curr_sub_word = malloc(sizeof(int)), curr_sub_word_index = 0;
	*max_curr_sub_word = 8;
	arr[arr_index] = malloc(sizeof(char) * *max_curr_sub_word);

	for (int read_string = 0; full_string[read_string]; read_string++) {
		if ((is_delim && is_delim(full_string[read_string], multi_delims)) || full_string[read_string] == delimeter) { // next phrase
			// check that current word has some length
			if (arr[arr_index][0] == '\0')
				continue;

			// quickly copy curr_sub_word_index into minor_length if minor_length is defined:
			if (minor_length)
				(*minor_length)[arr_index] = curr_sub_word_index;

			arr_index++;

			if (arr_index == *arr_len) {
				*arr_len *= 2;
				arr = realloc(arr, sizeof(char *) * *arr_len);

				if (minor_length)
					*minor_length = realloc(*minor_length, sizeof(int) * *arr_len);
			}

			curr_sub_word_index = 0;
			*max_curr_sub_word = 8;
			arr[arr_index] = malloc(sizeof(char) * *max_curr_sub_word);
			arr[arr_index][0] = '\0';

			continue;
		}

		// if not in range, skip:
		if (((int) full_string[read_string] < 65 || (int) full_string[read_string] > 90) &&
			((int) full_string[read_string] < 97 || (int) full_string[read_string] > 122))
			continue;

		// if a capital letter, lowercase
		if ((int) full_string[read_string] <= 90)
			full_string[read_string] = (char) ((int) full_string[read_string] + 32);

		arr[arr_index][curr_sub_word_index++] = full_string[read_string];

		if (curr_sub_word_index == *max_curr_sub_word) {
			*max_curr_sub_word *= 2;
			arr[arr_index] = realloc(arr[arr_index], sizeof(char *) * *max_curr_sub_word);
		}

		arr[arr_index][curr_sub_word_index] = '\0';
	}

	if (arr[arr_index][0] == '\0') { // free position
		free(arr[arr_index]);

		arr_index--;
	}

	*arr_len = arr_index + 1;

	free(max_curr_sub_word);

	return arr;
}

int destroy_split_string(char **split_string, int *split_string_len) {
	for (int delete_sub = 0; delete_sub < *split_string_len; delete_sub++) {
		free(split_string[delete_sub]);
	}

	free(split_string);

	return 0;
}

void destroy_hashmap_val(void *ptr) {
	free((int *) ptr);

	return;
}

void *is_block(void *hmap, char *tag) {
	return get__hashmap((hashmap *) hmap, tag);
}

int word_bag(FILE *index_fp, FILE *title_fp, trie_t *stopword_trie, token_t *full_page, hashmap *idf_hash) {
	int total_bag_size = 0;

	// create title page:
	// get ID
	int *ID_len = malloc(sizeof(int));
	char *ID = token_read_all_data(grab_token_by_tag(full_page, "id"), ID_len, NULL, NULL);

	total_bag_size += *ID_len + 1;

	// get title
	int *title_len = malloc(sizeof(int));
	char *title = token_read_all_data(grab_token_by_tag(full_page, "title"), title_len, NULL, NULL);

	// write to title_fp
	fputs(ID, title_fp);
	fputs(":", title_fp);
	fputs(title, title_fp);
	fputs("\n", title_fp);

	free(ID_len);
	free(title_len);
	free(title);

	// grab full page data
	int *page_data_len = malloc(sizeof(int)), *word_number_max = malloc(sizeof(int));
	token_t *page_token = grab_token_by_tag(full_page, "text");

	// setup a hashmap that will check for blocked tokens:
	hashmap *block_tag_check = make__hashmap(0, NULL, destroy_hashmap_val);

	// insert any blocked tags (currently just <style>)
	int *style_value = malloc(sizeof(int));
	*style_value = 1;
	int *a_tag_value = malloc(sizeof(int));
	*a_tag_value = 1;
	insert__hashmap(block_tag_check, "style", style_value, "-d");
	insert__hashmap(block_tag_check, "a", a_tag_value, "-d");
	char *token_page_data = token_read_all_data(page_token, page_data_len, block_tag_check, is_block);

	// create an int array so we can know the length of each char *
	int *phrase_len = malloc(sizeof(int) * 8);
	char **full_page_data = split_string(token_page_data, ' ', word_number_max, "-l", &phrase_len);

	deepdestroy__hashmap(block_tag_check);

	free(page_data_len);
	free(token_page_data);

	// create hashmap representation:
	hashmap *word_freq_hash = make__hashmap(0, NULL, NULL);

	int sum_of_squares = 0; // calculate sum of squares

	// loop through full_page_data and for each word:
		// check if the word is already in the hashmap, if it is:
			// add to the frequency for that word
		// if it isn't:
			// insert into hashmap at word with frequency = 0
	for (int add_hash = 0; add_hash < *word_number_max; add_hash++) {
		// check if word is in the stopword trie:
		if (trie_search(stopword_trie, full_page_data[add_hash])) {
			free(full_page_data[add_hash]);
			continue; // skip
		}

		full_page_data[add_hash][stem(full_page_data[add_hash], 0, strlen(full_page_data[add_hash]) - 1) + 1] = 0;

		// get from word_freq_hash:
		int *hashmap_freq = get__hashmap(word_freq_hash, full_page_data[add_hash]);
		idf_t *idf = get__hashmap(idf_hash, full_page_data[add_hash]);

		if (hashmap_freq) {
			free(full_page_data[add_hash]);
			(*hashmap_freq)++;
			continue;
		}

		total_bag_size += phrase_len[add_hash];

		hashmap_freq = malloc(sizeof(int));
		*hashmap_freq = 1;

		insert__hashmap(word_freq_hash, full_page_data[add_hash], hashmap_freq, "", compareCharKey, NULL);

		// check prev_idf_index to make sure it doesn't match current index
		if (idf && idf->prev_idf_index == add_hash) // skip (duplicate)
			continue;
		else if (idf) { // add to current frequency
			idf->document_freq++;
			idf->prev_idf_index = add_hash;
		} else {
			idf = malloc(sizeof(idf_t));
			idf->document_freq = 1;
			idf->prev_idf_index = add_hash;
			insert__hashmap(idf_hash, full_page_data[add_hash], idf, "", compareCharKey, NULL);
		}
	}

	free(word_number_max);
	free(phrase_len);

	int *key_len = malloc(sizeof(int));
	char **keys = (char **) keys__hashmap(word_freq_hash, key_len);

	// setup index file:
	fputs(ID, index_fp);

	for (int count_sums = 0; count_sums < *key_len; count_sums++) {
		int key_freq = *(int *) get__hashmap(word_freq_hash, keys[count_sums]);
		sum_of_squares += key_freq * key_freq;
	}
	char *sum_square_char = malloc(sizeof(char) * 13);
	memset(sum_square_char, '\0', sizeof(char) * 13);

	sprintf(sum_square_char, " %d ", sum_square_char);
	fputs(sum_square_char, index_fp);

	free(sum_square_char);

	// loop through keys and input word:freq pairs
	for (int write_key = 0; write_key < *key_len; write_key++) {
		int *key_freq = (int *) get__hashmap(word_freq_hash, keys[write_key]);

		char *key_freq_str = malloc(sizeof(char) * 13);
		memset(key_freq_str, '\0', sizeof(char) * 13);

		sprintf(key_freq_str, ":%d ", *(int *) key_freq);

		// add length of key_freq_str to bag_size:
		total_bag_size += strlen(key_freq_str);

		fputs(keys[write_key], index_fp);
		fputs(key_freq_str, index_fp);

		free(key_freq_str);
	}

	fputs("\n", index_fp);
	total_bag_size++;

	free(ID);

	free(keys);
	free(key_len);

	deepdestroy__hashmap(word_freq_hash);
	free(full_page_data);

	return total_bag_size;
}

// NOTE: index_reader and index_outputter should be diferent files!
int word_bag_idf(FILE *index_reader, FILE *index_outputter, hashmap *idf, int *word_bag_len, int word_bag_len_max) {
	// for each doc in word_bag_len:
	for (int doc = 0; doc < 1; doc++) {
		char *word_bag;

		// read the contents into word_bag
		printf("reading %d doc: %d\n", doc, word_bag_len[doc]);
		fread(word_bag, sizeof(char), word_bag_len[doc], index_reader);

		// splice word_bag by multi_delimeters:
		// use " " and ":" as delimeters
		int *word_bag_words_max = malloc(sizeof(int));
		char **word_bag_words = split_string(word_bag, 0, word_bag_words_max, "-d", delimeter_check, " :");

		for (int check_doc = 0; check_doc < *word_bag_words_max; check_doc++) {
			printf("%s", word_bag_words[check_doc]);
		}
	}

	return 0;
}