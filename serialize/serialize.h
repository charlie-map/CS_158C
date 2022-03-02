#ifndef __SERIALIZE_L__
#define __SERIALIZE_L__

#include "trie.h"
#include "token.h"
#include "hashmap.h"

typedef struct IDF_Track idf_t;
void hashmap_destroy_idf(void *p);

// struct for mutex locking
typedef struct MutexLocker {
	pthread_mutex_t mutex;
	void *runner;
} mutex_t;
mutex_t newMutexLocker(void *payload);

typedef struct TermFreqRep {
	int max_full_rep, full_rep_index;
	char *full_rep; // DOC_ID,FREQ|DOC_ID,FREQ|DOC_ID,FREQ

	char *curr_doc_id; // to know if we should reset curr_freq
	int curr_term_freq; // the current document to calculate

	int doc_freq;
} tf_t;
void destroy_tf_t(void *tf);
int word_bag(hashmap *term_freq, mutex_t *title_fp, trie_t *stopword_trie, token_t *full_page, char **ID);

typedef struct HashmapBody {
	char *id;
	float mag, sqrt_mag;
	hashmap *map;
} hashmap_body_t;

void destroy_hashmap_body(hashmap_body_t *body_hash);
hashmap_body_t **word_bag_idf(FILE *index_reader, hashmap *idf, int *word_bag_len, int word_bag_len_max, int dtf_drop_threshold);

void destroy_hashmap_val(void *ptr);
void destroy_hashmap_float(void *v);

#endif