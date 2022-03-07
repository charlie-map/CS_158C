#ifndef __DESERIALIZE_L__
#define __DESERIALIZE_L__

#include "../utils/hashmap.h"

typedef struct HashmapBody {
	char *id, *title;
	float mag, sqrt_mag;
	hashmap *map;
} hashmap_body_t;

hashmap_body_t *create_hashmap_body(char *id, char *title, float mag);
void destroy_hashmap_body(hashmap_body_t *body_hash);

hashmap *deserialize_title(char *title_reader);

char **deserialize(char *index_reader, hashmap *docs, int *max_words);

#endif /* __DESERIALIZE_L__ */