#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Token {
	char *tag;
	void *data; // for a singular type (like a char *)

	int max_children, children_index;
	struct Token **children; // for nest children tags

	struct Token *parent;
} token_t;

token_t *create_token(char *tag, void *data, token_t *parent) {
	token_t *new_token = malloc(sizeof(token_t));

	new_token->tag = tag;
	new_token->data = data;

	new_token->max_children = 8;
	new_token->children_index = 0;
	new_token->children = malloc(sizeof(void *) * new_token->max_children);

	new_token->parent = parent;

	return new_token;
}

int add_token_data(token_t *token, void *data) {
	token->data = data;

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

token_t *tokenize(char *filename) {
	FILE *file = fopen(filename, "r");

	token_t *curr_tree = create_token("root", NULL, NULL);

	int reading_tag = 0; // check for if we are currently within a tag
	int tag_type = 1; // if we are in the open or closed tag reading
					  // 1 for open tag, 0 for closed tag

	// layout for an arraylist to count up the semi_tag
	int semi_tag_max = 8, semi_tag_curr = 0;
	char *semi_tag = malloc(sizeof(char) * semi_tag_max);

	// layout for an arraylist to count up the data within a tag (char *)
	int inner_string_tag_max = 8, inner_string_tag_curr = 0;
	char *inner_string_tag = malloc(sizeof(char) * inner_string_tag_max);
	memset(inner_string_tag, '\0', inner_string_tag_max);

	char *line_read = malloc(sizeof(char));
	size_t buffsize = 0;

	while(getline(&line_read, &buffsize, file)) {
		printf("start reading: %s", line_read);
		// search for tags and data
		int search_line = 0;

		while (line_read[search_line] != '\n' && line_read[search_line] != '\0') {
			// find open and close tags
			if (line_read[search_line] == '<') {
				reading_tag = 1;

				if (line_read[search_line + 1] == '/') {
					search_line++;
					tag_type = 0;

					curr_tree = curr_tree->parent ? (token_t *) grab_token_parent(curr_tree) : curr_tree;

					continue;
				}

				semi_tag_curr = 0;
				memset(semi_tag, '\0', semi_tag_max);

				tag_type = 1;
			} else if (line_read[search_line] == '>') {

				reading_tag = 0;

				if (!tag_type) {
					// when we are on the close tag, add whatever is
					// in the inner_string_tag into the data
					// for the curr_tree tag
					// copy data to a new allocated string
					// plus space for null terminator
					if (strlen(inner_string_tag)) {
						char *copy_data = malloc(sizeof(char) * strlen(inner_string_tag) + sizeof(char));
						strcpy(copy_data, inner_string_tag);

						add_token_data(curr_tree, copy_data);
					}

					inner_string_tag_curr = 0;
					memset(inner_string_tag, '\0', inner_string_tag_max);
				} else {
					// otherwise take curr_tree (which is this nodes' parent)
					// and add a new token child
					// same as data, copy tag into a newly allocated string
					char *copy_tag = malloc(sizeof(semi_tag));
					strcpy(copy_tag, semi_tag);

					token_t *new_token = create_token(copy_tag, NULL, curr_tree);

					// add new_token as a child of parent
					add_token_children(curr_tree, new_token);

					// re add parent (since it's still part of this tree)
					curr_tree = new_token;
				}
			} else {
				if (reading_tag) {
					semi_tag[semi_tag_curr] = line_read[search_line];

					semi_tag_curr++;

					if (semi_tag_curr == semi_tag_max) { // resize
						semi_tag_max *= 2;

						semi_tag = realloc(semi_tag, sizeof(char) * semi_tag_max);
					}

					semi_tag[semi_tag_curr] = '\0';
				} else {
					inner_string_tag[inner_string_tag_curr] = line_read[search_line];

					inner_string_tag_curr++;

					if (inner_string_tag_curr == inner_string_tag_max) { // resize
						inner_string_tag_max *= 2;

						inner_string_tag = realloc(inner_string_tag, sizeof(char) * inner_string_tag_max);
					}

					inner_string_tag[inner_string_tag_curr] = '\0';
				}
			}

			search_line++;
		}
	}

	free(line_read);

	free(inner_string_tag);
	free(semi_tag);

	return curr_tree;
}