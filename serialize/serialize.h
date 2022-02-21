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

int word_bag(mutex_t index_fp, mutex_t title_fp, trie_t *stopword_trie, token_t *full_page, mutex_t idf, char **ID);

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