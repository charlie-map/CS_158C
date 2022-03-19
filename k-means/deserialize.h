#ifndef __DESERIALIZE_L__
#define __DESERIALIZE_L__

#include "k-means.h"
#include "../utils/hashmap.h"

hashmap_body_t *create_hashmap_body(char *id, char *title, float mag);
void destroy_hashmap_body(hashmap_body_t *body_hash);

hashmap *deserialize_title(char *title_reader);

char **deserialize(char *index_reader, hashmap *docs, int *max_words);

cluster_t **deserialize_cluster(char *filename, int k, hashmap *doc_map, char **word_bag, int *word_bag_len);

#endif /* __DESERIALIZE_L__ */