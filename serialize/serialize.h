#ifndef __SERIALIZE_L__
#define __SERIALIZE_L__

#include "trie.h"
#include "token.h"

int word_bag(FILE *index_fp, FILE *title_fp, trie_t *stopword_trie, token_t *full_page);

#endif