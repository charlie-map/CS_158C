#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Token {
	char *tag;
	void *data; // for a singular type (like a char *)

	int max_attr_tag, attr_tag_index;
	char **attribute; // holds any attributes for this tag
	// the attribute name is always an even position and
	// the attribute itself is odd

	int max_children, children_index;
	struct Token **children; // for nest children tags

	struct Token *parent;
} token_t;

token_t *create_token(char *tag, void *data, token_t *parent) {
	token_t *new_token = malloc(sizeof(token_t));

	new_token->tag = tag;
	new_token->data = data;

	new_token->max_attr_tag = 8;
	new_token->attr_tag_index = 0;
	new_token->attribute = malloc(sizeof(char *) * new_token->max_attr_tag);

	new_token->max_children = 8;
	new_token->children_index = 0;
	new_token->children = malloc(sizeof(token_t *) * new_token->max_children);

	new_token->parent = parent;

	return new_token;
}

int add_token_data(token_t *token, void *data) {
	token->data = data;

	return 0;
}

int add_token_attribute(token_t *token, char *tag, char *attribute) {
	token->attribute[token->attr_tag_index++] = tag;
	token->attribute[token->attr_tag_index] = attribute;

	// check for resize
	if (token->attr_tag_index == token->max_attr_tag) {
		token->max_attr_tag *= 2;

		token->attribute = realloc(token->attribute, sizeof(char *) * token->max_attr_tag);
	}

	return 0;
}

int add_token_children(token_t *token, void *child) {
	token_t token_parent = *token;

	token_parent.children[token_parent.children_index] = child;

	token_parent.children_index++;

	// check for resize
	if (token_parent.children_index == token_parent.max_children) {
		token_parent.max_children *= 2;

		token_parent.children = realloc(token_parent.children, sizeof(token_t*) * token_parent.max_children);
	}

	return 0;
}

void *grab_token_children(token_t *parent) {
	return parent->children[parent->children_index];
}

void *grab_token_parent(token_t *curr_token) {
	return curr_token->parent;
}

token_t *tokenize(char *filename);

int main() {
	token_t *xml_tree = tokenize("miniXML.txt");

	return 0;
}

int read_main_tag(char **main_tag, char *curr_line, int search_tag) {
	*main_tag = malloc(sizeof(char) * 8);
	int max_main_tag = 8, main_tag_index = 0;

	while (curr_line[search_token] != '>' &&
		   curr_line[search_token] != '\n' &&
		   curr_line[search_token] != ' ') {

		(*main_tag)[main_tag_index] = curr_line[search_token];

		search_token++;
		main_tag_index++;

		if (main_tag_index == max_main_tag) {
			max_main_tag *= 2;

			*main_tag = realloc(*main_tag, sizeof(char) * max_main_tag);
		}
	}

	return search_tag;
}

// builds a new tree and adds it as a child of parent_tree
int read_tag(token_t *parent_tree, FILE *file, char **curr_line, size_t buffer_size, int search_token) {
	char *main_tag;
	search_token = read_main_tag(&main_tag, *curr_line, search_token);

	token_t *new_tree = create_token(main_tag, NULL, parent_tree);

	char *attr_tag_name = malloc(sizeof(char) * 8);
	int max_attr_tag_name = 8, attr_tag_name_index = 0;
	char *attr_tag_value = malloc(sizeof(char) * 8);
	int max_attr_tag_value = 8, attr_tag_value_index = 0;

	while ((*curr_line)[search_token] != '>') {
		if (curr_line[search_token] == '\n') {
			search_token = 0;

			getline(curr_line, &buffsize, file);
		}



		search_token++;
	}

	return search_token;
}

int tokenizeMETA(FILE *file, token_t *curr_tree, char *curr_line, size_t buffer_size, int search_token) {

	while(getline(&curr_line, &buffsize, file)) {

		while (curr_line[search_token] != '\n' && curr_line[search_token] != '\0') {
			if (curr_line[search_token] == '<') {
				printf("searching tag %s\n", curr_line);

				search_token = read_tag(curr_tree, &curr_line, search_token);
			}

			search_token++;
		}
	}

	return 0;
}

token_t *tokenize(char *filename) {
	FILE *file = fopen(filename, "r");

	token_t *curr_tree = create_token("root", NULL, NULL);

	char *line_read = malloc(sizeof(char));
	size_t buffsize = 0;

	tokenizeMETA(file, curr_tree, line_read, buffsize, -1);
}