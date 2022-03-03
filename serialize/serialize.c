#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#include "serialize.h"
#include "../stemmer.h"
#include "../utils/helper.h"

struct IDF_Track {
	char *prev_idf_ID; // last document to be added (for checking dupes)
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

mutex_t newMutexLocker(void *payload) {
	mutex_t new_mutexer = { .runner = payload, .mutex = PTHREAD_MUTEX_INITIALIZER };

	return new_mutexer;
}

void destroy_hashmap_val(void *ptr) {
	free((int *) ptr);

	return;
}

void *is_block(void *hmap, char *tag) {
	return get__hashmap((hashmap *) hmap, tag, 0);
}

tf_t *new_tf_t(char *ID) {
	tf_t *new_tf = malloc(sizeof(tf_t));

	new_tf->max_full_rep = 8; new_tf->full_rep_index = 0;
	new_tf->full_rep = malloc(sizeof(char) * new_tf->max_full_rep);
	memset(new_tf->full_rep, '\0', sizeof(char) * new_tf->max_full_rep);

	new_tf->curr_doc_id = ID;
	new_tf->curr_term_freq = 1;

	new_tf->doc_freq = 1;

	return new_tf;
}

void destroy_tf_t(void *tf) {
	free(((tf_t *) tf)->full_rep);

	free((tf_t *) tf);

	return;
}

int is_m(void *tf, void *extra) {
	tf_t *tt = (tf_t *) tf;

	return strcmp(tt->curr_doc_id, (char *) extra) == 0;
}

/* Update to wordbag:
	Now index_fp, title_fp, and idf_hash need mutex locking,
	so bring mutex attr with them
*/
int word_bag(hashmap *term_freq, mutex_t *title_fp, trie_t *stopword_trie, token_t *full_page, char **ID) {
	int total_bag_size = 0;

	// create title page:
	// get ID
	int *ID_len = malloc(sizeof(int));
	*ID = token_read_all_data(grab_token_by_tag(full_page, "id"), ID_len, NULL, NULL);

	total_bag_size += *ID_len - 1;

	// get title
	int *title_len = malloc(sizeof(int));
	char *title = token_read_all_data(grab_token_by_tag(full_page, "title"), title_len, NULL, NULL);

	// write to title_fp
	pthread_mutex_lock(&(title_fp->mutex));
	fputs(*ID, title_fp->runner);
	fputs(":", title_fp->runner);
	fputs(title, title_fp->runner);
	
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

		phrase_len[add_hash] = stem(full_page_data[add_hash], 0, phrase_len[add_hash] - 1) + 1;
		full_page_data[add_hash][phrase_len[add_hash]] = 0;

		// get from word_freq_hash:
		// get char * from idf and free full_page_data
		char *prev_hash_key = (char *) getKey__hashmap(term_freq, full_page_data[add_hash]);

		if (prev_hash_key) {
			tf_t *hashmap_freq = (tf_t *) get__hashmap(term_freq, full_page_data[add_hash], 0);

			free(full_page_data[add_hash]);

			if (strcmp(*ID, hashmap_freq->curr_doc_id) == 0)
				hashmap_freq->curr_term_freq++;
			else { // reset features
				hashmap_freq->curr_term_freq = 1;
				hashmap_freq->curr_doc_id = *ID;

				hashmap_freq->doc_freq++;
			}

			continue;
		}

		total_bag_size += phrase_len[add_hash];

		tf_t *new_tf = new_tf_t(*ID);

		insert__hashmap(term_freq, full_page_data[add_hash], new_tf, "", compareCharKey, destroyCharKey);
	}

	free(word_number_max);
	free(phrase_len);

	int *key_len = malloc(sizeof(int));
	char **keys = (char **) keys__hashmap(term_freq, key_len, "m", is_m, *ID);

	for (int count_sums = 0; count_sums < *key_len; count_sums++) {
		tf_t *m_val = (tf_t *) get__hashmap(term_freq, keys[count_sums], 0);
		int key_freq = m_val->curr_term_freq;
		sum_of_squares += key_freq * key_freq;

		// update full_rep
		// ID,freq|
		int freq_len = (int) log10(key_freq) + 2;
		int length = *ID_len + freq_len;

		// make sure char has enough space
		while (m_val->full_rep_index + length + 1 > m_val->max_full_rep) {
			m_val->max_full_rep *= 2;

			m_val->full_rep = realloc(m_val->full_rep, sizeof(char) * m_val->max_full_rep);
		}

		sprintf(m_val->full_rep + sizeof(char) * m_val->full_rep_index, "%s,%d|", *ID, key_freq);
		m_val->full_rep_index += length;
		m_val->full_rep[m_val->full_rep_index] = '\0';
	}
	char *sum_square_char = malloc(sizeof(char) * 13);
	memset(sum_square_char, '\0', sizeof(char) * 13);

	sprintf(sum_square_char, " %d\n", sum_of_squares);
	total_bag_size += strlen(sum_square_char);
	fputs(sum_square_char, title_fp->runner);
	pthread_mutex_unlock(&(title_fp->mutex));

	free(sum_square_char);

	free(keys);
	free(key_len);
	free(ID_len);

	free(full_page_data);

	return 0;
}

void destroy_hashmap_float(void *v) {
	free((float *) v);

	return;
}

int compareFloatKey(void *v1, void *v2) {
	return *(float *) v1 < *(float *) v2;
}