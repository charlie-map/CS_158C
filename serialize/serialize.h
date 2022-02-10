#ifndef __SERIALIZE_L__
#define __SERIALIZE_L__

#include "trie.h"
#include "token.h"
#include "hashmap.h"

typedef struct IDF_Track idf_t;
void hashmap_destroy_idf(void *p);

int word_bag(FILE *index_fp, FILE *title_fp, trie_t *stopword_trie, token_t *full_page, hashmap *idf);

void destroy_hashmap_val(void *ptr);

#endif