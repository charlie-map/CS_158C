#ifndef __KD_TREE_L__
#define __KD_TREE_L__

typedef struct KD_Tree kdtree_t;

kdtree_t *kdtree_create(int (*weight)(void *, void *), void *(*member_extract)(void *, void *), void *dimension, void *(*next_d)(void *));

int kdtree_load(kdtree_t *k_t, void ***members, int member_length);
void *kdtree_insert(kdtree_t *k_t, void *payload);

void *kdtree_min(kdtree_t *k_t, void *D);
void *kdtree_delete(kdtree_t *k_t, void *k_node, ...);

int kdtree_destroy(kdtree_t *k_t);

int default_weight(void *p1, void *p2);
void *default_member_extract(void *member, void *dimension);
void *default_next_d(void *dimension);

#endif