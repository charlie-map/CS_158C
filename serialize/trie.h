#ifndef __TRIE_T__
#define __TRIE_T__

typedef struct Trie trie_t;

trie_t *trie_create(char *param, ...);

int trie_insert(trie_t *trie, void *p_value);
int trie_search(trie_t *trie, void *p_value);

int trie_destroy(trie_t *trie);

#endif