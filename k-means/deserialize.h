#ifndef __DESERIALIZE_L__
#define __DESERIALIZE_L__

typedef struct HashmapBody {
	char *id;
	float mag, sqrt_mag;
	hashmap *map;
} hashmap_body_t;

void destroy_hashmap_body(hashmap_body_t *body_hash);
hashmap_body_t **deserialize(FILE *index_reader, int dtf_drop_threshold);

#endif /* __DESERIALIZE_L__ */