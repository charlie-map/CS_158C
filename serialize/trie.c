#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "hashmap.h"
#include "trie.h"

// default next for simple_payload
void *default_next(void *payload) {
	if (!((char *) payload)[1])
		return NULL;

	return payload + sizeof(char);
}

int default_comparer(void *p1, void *p2) {
	return ((char *) p1)[0] == ((char *) p2)[0];
}

// default converter for simple payload
char simple_convert(void *payload) {
	return ((char *) payload)[0];
}

char *singleton_maker(void *payload) {
	char *singleton = malloc(sizeof(char) * 2);

	singleton[0] = ((char *) payload)[0];
	singleton[1] = '\0';

	return singleton;
}

int default_delete(void *payload) {
	free((char *) payload);

	return 0;
}

typedef struct TrieNode {
	void *payload;
	int (*destroy_payload)(void *);

	int thru_weight;
	int end_weight;

	hashmap *children;
} node_t;

void node_destroy(void *void_node) {
	node_t *node = (node_t *) void_node;

	if (node->children)
		deepdestroy__hashmap(node->children);

	if (node->destroy_payload)
		node->destroy_payload(node->payload);

	free(node);

	return;
}

node_t *node_construct(void *payload, int (*delete)(void *)) {
	node_t *new_node = malloc(sizeof(node_t));

	new_node->payload = payload;
	new_node->destroy_payload = delete;

	new_node->thru_weight = 0;
	new_node->end_weight = 0;

	new_node->children = make__hashmap(0, NULL, node_destroy);

	return new_node;
}

struct Trie {
	int payload_type; // 1 for char, 0 for void *

	int (*comparer)(void *, void *);
	void *(*next)(void *);

	int (*delete)(void *);

	hashmap *root_node;
};

/*
	trie_create develops a new header for a trie. The header connects into
	a root node that contains the actual root node of the trie

	trie_create takes in the parameter: param
	- param will include the structure of any included values such as (per 1.0):
		'c': compare function, should take in two void * (or char) values and return
		either 1 for the first value is greater, negative one if less, and 0
		for if the values are equal. If not given, the assumed weight default
		function will use char *'s as per default trie behavior
		'p': payload type, defines what kind of data the trie holds: either
		char (if given 'c') or void * (if given 'v')
		'n': for an insertion (or search), this paramater takes a function
		of type void (*)(void *) and gives the next value in the data type (
		linked list, char *, etc.) -- will set to default (char *) if not inputted
		'd': to delete values post insertion. For a char *, just go through and
		cleans stored data
*/
trie_t *trie_create(char *param, ...) {
	trie_t *new_trie = malloc(sizeof(trie_t));

	new_trie->payload_type = 1;
	new_trie->root_node = make__hashmap(0, NULL, node_destroy);

	new_trie->next = default_next;
	new_trie->comparer = default_comparer;
	new_trie->delete = default_delete;

	va_list param_detail;
	va_start(param_detail, param);

	// look at p_tag first
	int find_p = 0;
	while (param[find_p] && param[find_p + 1] && (param[find_p] != '-' || param[find_p + 1] != 'p')) {
		find_p++;
	}

	if (param[find_p] && param[find_p + 1] && param[find_p] == '-' && param[find_p + 1]) {
		if (!param[find_p + 2]) // look for value
			return NULL; // ERROR

		new_trie->payload_type = param[find_p + 2] == 'c';
	} else
		new_trie->payload_type = 1;

	// check for other parameters
	for (find_p = 0; param[find_p + 1]; find_p++) {
		if (param[find_p] != '-')
			continue;

		if (param[find_p + 1] == 'n') {
			printf("add next");
			new_trie->next = va_arg(param_detail, void *(*)(void *));
		} else if (param[find_p + 1] == 'c') {
			printf("add compare");
			new_trie->comparer = va_arg(param_detail, int (*)(void *, void *));
		} else if (param[find_p + 1] == 'd') {
			printf("add delete");
			new_trie->delete = va_arg(param_detail, int (*)(void *));
		}
	}

	// return updated new_trie
	return new_trie;
}

int trie_insert_helper(hashmap *curr_node, trie_t *trie_meta_data, void *value) {
	char *build_alloc_char = trie_meta_data->payload_type ? singleton_maker(value) : NULL;
	node_t *sub_node = get__hashmap(curr_node, build_alloc_char ? build_alloc_char : value);

	void *get_next_value = trie_meta_data->next(value);

	if (get_next_value && sub_node) {
		if (build_alloc_char)
			free(build_alloc_char);

		sub_node->thru_weight++;
	} else {
		if (!sub_node) {
			sub_node = node_construct(build_alloc_char ? build_alloc_char : value, trie_meta_data->delete);
			insert__hashmap(curr_node, build_alloc_char ? build_alloc_char : value, sub_node, "", trie_meta_data->comparer, NULL);
		} else
			free(build_alloc_char);

		sub_node->end_weight++;
		if (!get_next_value)
			return 0;
	}

	return trie_insert_helper(sub_node->children, trie_meta_data, get_next_value);
}

// the value that comes after trie depends on weight_option
// either void * for weight_option = 0 or char for weight_option = 1
int trie_insert(trie_t *trie, void *p_value) {
	return trie_insert_helper(trie->root_node, trie, p_value);
}

int trie_search_helper(hashmap *curr_node, trie_t *trie_meta_data, void *value) {
	char *build_alloc_char = trie_meta_data->payload_type ? singleton_maker(value) : NULL;
	node_t *sub_node = get__hashmap(curr_node, build_alloc_char ? build_alloc_char : value);

	void *get_next_value = trie_meta_data->next(value);

	if (build_alloc_char)
		free(build_alloc_char);

	if (get_next_value && sub_node)
		return trie_search_helper(sub_node->children, trie_meta_data, get_next_value);
	else if (sub_node && sub_node->end_weight)
		return sub_node->end_weight;

	return 0;
}

int trie_search(trie_t *trie, void *p_value) {
	if (!trie->root_node)
		return 0;

	return trie_search_helper(trie->root_node, trie, p_value);
}

int trie_destroy(trie_t *trie) {
	deepdestroy__hashmap(trie->root_node);

	free(trie);

	return 0;
}