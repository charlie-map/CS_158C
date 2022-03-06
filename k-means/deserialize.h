#ifndef __DESERIALIZE_L__
#define __DESERIALIZE_L__

#include "../utils/hashmap.c"

typedef struct HashmapBody {
	char *id, *title;
	float mag, sqrt_mag;
	hashmap *map;
} hashmap_body_t;

hashmap_body_t *create_hashmap_body(char *id, char *title, float mag);
void destroy_hashmap_body(hashmap_body_t *body_hash);

hashmap *deserialize_title(char *title_reader);

hashmap_body_t **deserialize_bag(char *index_reader, int dtf_drop_threshold);

#endif /* __DESERIALIZE_L__ */