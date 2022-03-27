#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "document-vector.h"

void destroy_hashmap_float(void *v) {
	free((float *) v);

	return;
}

document_vector_t *create_document_vector(char *id, char *title, float mag) {
	document_vector_t *hm = (document_vector_t *) malloc(sizeof(document_vector_t));

	hm->id = id;
	hm->title = title;

	hm->mag = mag;
	hm->sqrt_mag = sqrt(mag);

	hm->map = make__hashmap(0, NULL, destroy_hashmap_float);

	return hm;
}

void destroy_hashmap_body(document_vector_t *body_hash) {
	free(body_hash->id);
	free(body_hash->title);

	deepdestroy__hashmap(body_hash->map);

	free(body_hash);
	return;
}