#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"

void *resize_arraylist(void *array, int *max_size, int current_index) {
	if (current_index == *max_size) {
		*max_size *= 2;

		array = realloc(array, sizeof(array[0]) * *max_size);
	}

	return array;
}

struct Token {
	char *tag;

	int data_index, max_data;
	char *data; // for a singular type (like a char *)

	int max_attr_tag, attr_tag_index;
	char **attribute; // holds any attributes for this tag
	// the attribute name is always an even position and
	// the attribute itself is odd

	int max_children, children_index;
	struct Token **children; // for nest children tags

	struct Token *parent;
};

token_t *create_token(char *tag, token_t *parent) {
	token_t *new_token = malloc(sizeof(token_t));

	new_token->tag = tag;
	new_token->data = malloc(sizeof(char) * 8);
	new_token->max_data = 8;
	new_token->data_index = 0;

	new_token->max_attr_tag = 8;
	new_token->attr_tag_index = 0;
	new_token->attribute = malloc(sizeof(char *) * new_token->max_attr_tag);

	new_token->max_children = 8;
	new_token->children_index = 0;
	new_token->children = malloc(sizeof(token_t *) * new_token->max_children);

	new_token->parent = parent;

	return new_token;
}

int add_token_rolling_data(token_t *token, char data) {
	token->data[token->data_index] = data;

	token->data_index++;

	token->data = resize_arraylist(token->data, &token->max_data, token->data_index);
	token->data[token->data_index] = '\0';

	return 0;
}

int add_token_attribute(token_t *token, char *tag, char *attribute) {
	token->attribute[token->attr_tag_index++] = tag;
	token->attribute[token->attr_tag_index++] = attribute;

	// check for resize
	token->attribute = resize_arraylist(token->attribute, &token->max_attr_tag, token->attr_tag_index);

	return 0;
}

int add_token_children(token_t *token_parent, token_t *child) {
	token_parent->children[token_parent->children_index] = child;

	token_parent->children_index++;

	// check for resize
	token_parent->children = resize_arraylist(token_parent->children, &token_parent->max_children, token_parent->children_index);

	return 0;
}

token_t **token_children(token_t *parent) {
	return parent->children;
}

token_t *grab_token_children(token_t *parent) {
	if (parent->children_index == 0)
		return NULL;

	return parent->children[parent->children_index - 1];
}

token_t *grab_token_parent(token_t *curr_token) {
	return curr_token->parent;
}

int destroy_token(token_t *curr_token) {
	free(curr_token->tag);
	free(curr_token->data);

	// free attributes
	for (int free_attribute = 0; free_attribute < curr_token->attr_tag_index; free_attribute++)
		free(curr_token->attribute[free_attribute]);
	free(curr_token->attribute);

	// free children
	for (int free_child = 0; free_child < curr_token->children_index; free_child++)
		destroy_token(curr_token->children[free_child]);
	free(curr_token->children);

	free(curr_token);

	return 0;
}

int token_tagMETA(token_t *search_token, char *search_tag, char ***found_tag, int *max_tag, int *tag_index) {
	// check if current tree position has correct tag
	if (strcmp(search_token->tag, search_tag) == 0) {
		(*found_tag)[(*tag_index)++] = search_token->data;

		*found_tag = (char **) resize_arraylist(*found_tag, max_tag, *tag_index);
	}

	// check children
	for (int check_child = 0; check_child < search_token->children_index; check_child++) {
		token_tagMETA(search_token->children[check_child], search_tag, found_tag, max_tag, tag_index);
	}

	return 0;
}

char **token_get_tag_data(token_t *search_token, char *tag_name, int *max_tag) {
	// setup char ** array
	*max_tag = 8;
	char **found_tag = malloc(sizeof(char *) * *max_tag);
	int *tag_index = (int *) malloc(sizeof(int));
	*tag_index = 0;

	token_tagMETA(search_token, tag_name, &found_tag, max_tag, tag_index);

	// move value into max tag (which becomes the index for the return value)
	*max_tag = *tag_index;
	free(tag_index);

	return found_tag;
}

int read_main_tag(char **main_tag, char *curr_line, int search_tag) {
	*main_tag = malloc(sizeof(char) * 8);
	int max_main_tag = 8, main_tag_index = 0;

	search_tag++;

	while (curr_line[search_tag] != '>' &&
		   curr_line[search_tag] != '\n' &&
		   curr_line[search_tag] != ' ') {

		(*main_tag)[main_tag_index] = curr_line[search_tag];

		search_tag++;
		main_tag_index++;

		*main_tag = resize_arraylist(*main_tag, &max_main_tag, main_tag_index);

		(*main_tag)[main_tag_index] = '\0';
	}

	return search_tag;
}

// builds a new tree and adds it as a child of parent_tree
int read_tag(token_t *parent_tree, FILE *file, char **curr_line, int search_token) {
	char *main_tag;
	search_token = read_main_tag(&main_tag, *curr_line, search_token);

	token_t *new_tree = create_token(main_tag, parent_tree);

	size_t buffer_size = 0;
	int read_tag = 1; // choose whether to add to attr_tag_name
					  // or attr_tag_value: 1 to add to attr_tag_name
					  // 0 to add to attr_tag_value
	int start_attr_value = 0; // checks if reading the attribute has begun
	/*
		primarily for when reading the attribute value:
		tag="hello"

		only after the first " is seen should reading the value occur
	*/

	char *attr_tag_name = malloc(sizeof(char) * 8);
	int max_attr_tag_name = 8, attr_tag_name_index = 0;
	memset(attr_tag_name, '\0', 8);
	char *attr_tag_value = malloc(sizeof(char) * 8);
	int max_attr_tag_value = 8, attr_tag_value_index = 0;
	memset(attr_tag_value, '\0', 8);

	// searching for attributes
	while ((*curr_line)[search_token] != '>') {
		if ((*curr_line)[search_token] == '\n') {
			search_token = 0;

			getline(curr_line, &buffer_size, file);
		}

		// skip not wanted characters
		if ((*curr_line)[search_token] == ' ' || (*curr_line)[search_token] == '\t') {
			search_token++;
			continue;
		}

		if (read_tag) {
			if ((*curr_line)[search_token] == '=') { // switch to reading value
				read_tag = 0;

				start_attr_value = 0;

				search_token++;
				continue;
			} else if (attr_tag_name_index > 0 && (*curr_line)[search_token - 1] == ' ') {
				// this means there was no attr value, so add this
				// with a NULL value and move into the next one
				char *tag_name = malloc(sizeof(char) * (attr_tag_name_index + 1));
				strcpy(tag_name, attr_tag_name);

				add_token_attribute(new_tree, tag_name, NULL);

				attr_tag_name_index = 0;
				continue;
			}

			attr_tag_name[attr_tag_name_index++] = (*curr_line)[search_token];

			attr_tag_name = resize_arraylist(attr_tag_name, &max_attr_tag_name, attr_tag_name_index);
			attr_tag_name[attr_tag_name_index] = '\0';
		} else {
			if ((*curr_line)[search_token] == '"' || (*curr_line)[search_token] == '\'') {
				if (start_attr_value) {
					// add to the token
					char *tag_name = malloc(sizeof(char) * (attr_tag_name_index + 1));
					strcpy(tag_name, attr_tag_name);

					char *attr = malloc(sizeof(char) * attr_tag_value_index + 1);
					strcpy(attr, attr_tag_value);
					add_token_attribute(new_tree, tag_name, attr);

					attr_tag_name_index = 0;
					attr_tag_value_index = 0;

					read_tag = 1;
					start_attr_value = 0;
				} else
					start_attr_value = 1;

				search_token++;
				continue;
			}

			if (!start_attr_value)
				continue;

			attr_tag_value[attr_tag_value_index++] = (*curr_line)[search_token];

			attr_tag_value = resize_arraylist(attr_tag_value, &max_attr_tag_value, attr_tag_value_index);
			attr_tag_value[attr_tag_value_index] = '\0';
		}

		search_token++;
	}

	free(attr_tag_name);
	free(attr_tag_value);

	add_token_children(parent_tree, new_tree);
	return search_token + 1;
}

int find_close_tag(FILE *file, char **curr_line, int search_close) {
	size_t buffer_size = 0;

	while((*curr_line)[search_close] != '>') {
		if ((*curr_line)[search_close] == '\n') {
			getdelim(curr_line, &buffer_size, 10, file);

			search_close = 0;
		}

		search_close++;
	}

	return search_close;
}

int tokenizeMETA(FILE *file, token_t *curr_tree) {
	int search_token = 0;

	size_t buffer_size = sizeof(char) * 123;
	char *curr_line = malloc(buffer_size);

	while(getdelim(&curr_line, &buffer_size, 10, file) != -1) {
		search_token = 0;

		while (curr_line[search_token] != '\n' && curr_line[search_token] != '\0') {
			if (curr_line[search_token] == '<') {

				if (curr_line[search_token + 1] == '/') { // close tag
					// return to parent tree instead

					curr_tree = grab_token_parent(curr_tree);

					search_token = find_close_tag(file, &curr_line, search_token) + 1;
					continue;
				}

				search_token = read_tag(curr_tree, file, &curr_line, search_token);
			
				// update curr_tree to dive into the subtree
				curr_tree = grab_token_children(curr_tree);
			}

			// otherwise add character to the curr_tree
			add_token_rolling_data(curr_tree, curr_line[search_token]);

			search_token++;
		}
	}

	free(curr_line);
	return 0;
}

token_t *tokenize(char *filename) {
	FILE *file = fopen(filename, "r");

	char *root_tag = malloc(sizeof(char) * 5);
	strcpy(root_tag, "root");
	token_t *curr_tree = create_token(root_tag, NULL);

	tokenizeMETA(file, curr_tree);

	fclose(file);

	return curr_tree;
}