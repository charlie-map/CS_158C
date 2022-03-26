#ifndef __DOCUMENT_VECTOR_L__
#define __DOCUMENT_VECTOR_L__

#include "../utils/hashmap.h"

typedef struct DocumentVector {
	char *id, *title;
	float mag, sqrt_mag;
	hashmap *map;
} document_vector_t;

document_vector_t *create_document_vector(char *id, char *title, float mag);

void destroy_hashmap_float(void *v);
void destroy_hashmap_body(document_vector_t *body_hash);

#endif /* __DOCUMENT_VECTOR_L__ */