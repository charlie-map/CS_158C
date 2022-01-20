#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stack.h"

typedef struct Token {
	char *tag;
	void *data; // for a singular type (like a char *)

	int max_children, children_index;
	void **children; // for nest children tags
} token_t;

token_t *create_token(char *tag, void *data) {
	token_t *new_token = malloc(sizeof(token_t));

	new_token->tag = tag;
	new_token->data = data;

	new_token->max_children = 8;
	new_token->children_index = 0;
	new_token->children = malloc(sizeof(void *) * new_token->max_children);

	return new_token;
}

int tokenize(stack_t *stack, char *filename);

int main() {
	stack_t *stack = stack_create();

	tokenize(stack, "miniXML.txt");
}

int tokenize(stack_t *stack, char *filename) {
	FILE *file = fopen(filename, "r");

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
	size_t buffsize;

	while(getline(&line_read, &buffsize, file)) {
		// search for tags and data
		int search_line = 0;

		while (line_read[search_line] != '\n') {
			// find open and close tags
			if (line_read[search_line] == '<') {
				reading_tag = 1;

				if (line_read[search_line + 1] == '/') {
					search_line++;
					tag_type = 0;

					token_t *prev_tag = (token_t *) stack_pop(stack);
					char *copy_inner_string = malloc(sizeof(char) * inner_string_tag_max);
					strcpy(copy_inner_string, inner_string_tag);
					prev_tag->data = copy_inner_string;

					continue;
				}

				semi_tag_curr = 0;
				memset(semi_tag, '\0', semi_tag_max);

				tag_type = 1;
			} else if (line_read[search_line] == '>') {

				reading_tag = 0;

				if (!tag_type) {
					token_t *parent = (token_t *) stack_pop(stack);

					printf("pull data %s\n", inner_string_tag);

					inner_string_tag_curr = 0;
					memset(inner_string_tag, '\0', inner_string_tag_max);
				} else {
					token_t *new_token = create_token(semi_tag, NULL);

					stack_push(stack, new_token);
				}
			} else {
				if (reading_tag) {
					semi_tag[semi_tag_curr] = line_read[search_line];

					semi_tag_curr++;

					if (semi_tag_curr == semi_tag_max) { // resize
						semi_tag_max *= 2;

						semi_tag = realloc(semi_tag, sizeof(char) * semi_tag_max);
					}
				} else {
					inner_string_tag[inner_string_tag_curr] = line_read[search_line];

					inner_string_tag_curr++;

					if (inner_string_tag_curr == inner_string_tag_max) { // resize
						inner_string_tag_max *= 2;

						inner_string_tag = realloc(inner_string_tag, sizeof(char) * inner_string_tag_max);
					}
				}
			}

			search_line++;
		}
	}

	free(line_read);

	free(inner_string_tag);
	free(semi_tag);

	return 0;
}