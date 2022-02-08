#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"

void *resize_arraylist(void *array, int *max_size, int current_index, size_t singleton_size) {
	if (current_index == *max_size) {
		*max_size *= 2;

		array = realloc(array, singleton_size * *max_size);
	}

	return array;
}

struct Token {
	char *tag;

	int data_index, *max_data;
	char *data; // for a singular type (like a char *)

	int *max_attr_tag, attr_tag_index;
	char **attribute; // holds any attributes for this tag
	// the attribute name is always an even position and
	// the attribute itself is odd

	int *max_children, children_index;
	struct Token **children; // for nest children tags

	struct Token *parent;
};

token_t *create_token(char *tag, token_t *parent) {
	token_t *new_token = malloc(sizeof(token_t));

	new_token->tag = tag;
	new_token->data = malloc(sizeof(char) * 8);
	new_token->max_data = malloc(sizeof(int));
	*new_token->max_data = 8;
	new_token->data_index = 0;

	new_token->max_attr_tag = malloc(sizeof(int));
	*new_token->max_attr_tag = 8;
	new_token->attr_tag_index = 0;
	new_token->attribute = malloc(sizeof(char *) * *new_token->max_attr_tag);

	new_token->max_children = malloc(sizeof(int));
	*new_token->max_children = 8;
	new_token->children_index = 0;
	new_token->children = malloc(sizeof(token_t *) * *new_token->max_children);

	new_token->parent = parent;

	return new_token;
}

int add_token_rolling_data(token_t *token, char data) {
	token->data[token->data_index] = data;

	token->data_index++;

	token->data = resize_arraylist(token->data, token->max_data, token->data_index, sizeof(char));
	token->data[token->data_index] = '\0';

	return 0;
}

int add_token_attribute(token_t *token, char *tag, char *attribute) {
	token->attribute[token->attr_tag_index++] = tag;
	token->attribute[token->attr_tag_index++] = attribute;

	// check for resize
	token->attribute = resize_arraylist(token->attribute, token->max_attr_tag, token->attr_tag_index, sizeof(char *));

	return 0;
}

int add_token_children(token_t *token_parent, token_t *child) {
	token_parent->children[token_parent->children_index] = child;

	token_parent->children_index++;

	// check for resize
	// if (token_parent->children_index == *token_parent->max_children) {
	// 	*token_parent->max_children *= 2;

	// 	token_parent->children = realloc(token_parent->children, sizeof(token_t *) * *token_parent->max_children);
	// }
	token_parent->children = resize_arraylist(token_parent->children, token_parent->max_children, token_parent->children_index, sizeof(token_t *));

	return 0;
}

token_t **token_children(token_t *parent) {
	return parent->children;
}

token_t *grab_token_children(token_t *parent) {
	if (parent->children_index == 0)
		return NULL;

	printf("get at: %s\n", (parent->children[parent->children_index - 1])->tag);
	return parent->children[parent->children_index - 1];
}

token_t *grab_token_parent(token_t *curr_token) {
	return curr_token->parent;
}

token_t *grab_token_by_tag(token_t *start_token, char *tag_name) {
	// search for first occurence of token with tag name == tag_name
	// if it doesn't exists, return NULL
	for (int check_children = 0; check_children < start_token->children_index; check_children++) {
		// compare tag:
		if (strcmp(start_token->children[check_children]->tag, tag_name) == 0)
			return start_token->children[check_children];
	}

	// otherwise check children
	for (int bfs_children = 0; bfs_children < start_token->children_index; bfs_children++) {
		token_t *check_children_token = grab_token_by_tag(start_token->children[bfs_children], tag_name);

		if (check_children_token)
			return check_children_token;
	}

	return NULL;
}

int grab_tokens_by_tag_helper(token_t **specific_token_builder, int *spec_token_max, int spec_token_index, token_t *start_token, char *tag_name) {
	// check current tokens children:
	for (int check_children = 0; check_children < start_token->children_index; check_children++) {
		if (strcmp(start_token->children[check_children]->tag, tag_name) == 0) {
			specific_token_builder[spec_token_index++] = start_token->children[check_children];

			specific_token_builder = (token_t **) resize_arraylist(specific_token_builder, spec_token_max, spec_token_index, sizeof(token_t *));
		}

		// recur
		spec_token_index = grab_tokens_by_tag_helper(specific_token_builder, spec_token_max, spec_token_index, start_token->children[check_children], tag_name);
	}

	return spec_token_index;
}

// This is not fast!!
token_t **grab_tokens_by_tag(token_t *start_token, char *tags_name, int *spec_token_max) {
	int spec_token_index = 0;
	*spec_token_max = 8;
	token_t **specific_token_builder = malloc(sizeof(token_t *) * *spec_token_max);

	spec_token_index = grab_tokens_by_tag_helper(specific_token_builder, spec_token_max, spec_token_index, start_token, tags_name);

	*spec_token_max = spec_token_index;

	return specific_token_builder;
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

		*found_tag = (char **) resize_arraylist(*found_tag, max_tag, *tag_index, sizeof(char *));
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

int token_read_all_data_helper(token_t *search_token, char **full_data, int *data_max, int data_index) {
	// search through each child token and add to full_data:
	for (int add_from_child = 0; add_from_child < search_token->children_index; add_from_child++) {
		// calculate length of the data:
		int data_len = search_token->children[add_from_child]->data_index;

		data_index += data_len;
		*full_data = resize_arraylist(*full_data, data_max, data_index, sizeof(char));

		strcpy(*full_data, search_token->children[add_from_child]->data);

		// get children
		data_index = token_read_all_data_helper(search_token->children[add_from_child], full_data, data_max, data_index);
	}

	return data_index;
}

// go through the entire sub tree and create a char ** of all data values
char *token_read_all_data(token_t *search_token, int *data_max) {
	int data_index = search_token->data_index;
	*data_max = data_index * 2;
	char **full_data = malloc(sizeof(char *));
	*full_data = malloc(sizeof(char) * *data_max);

	// read data from curr token:
	strcpy(*full_data, search_token->data);

	data_index = token_read_all_data_helper(search_token, full_data, data_max, data_index);

	*data_max = data_index;

	char *pull_data = *full_data;
	free(full_data);
	return pull_data;
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

		*main_tag = resize_arraylist(*main_tag, &max_main_tag, main_tag_index, sizeof(char));

		(*main_tag)[main_tag_index] = '\0';
	}

	return search_tag;
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

char *read_close_tag(FILE *file, char **curr_line, int search_close) {
	size_t buffer_size = 0;
	int *max_close_tag = malloc(sizeof(int)), close_tag_index = 0;
	*max_close_tag = 8;
	char *close_tag = malloc(sizeof(char) * 8);

	while((*curr_line)[search_close] != '>') {
		if ((*curr_line)[search_close] == '\n') {
			getdelim(curr_line, &buffer_size, 10, file);

			search_close = 0;
		}

		close_tag[close_tag_index++] = (*curr_line)[search_close];
		close_tag = resize_arraylist(close_tag, max_close_tag, close_tag_index, sizeof(char));
		close_tag[close_tag_index] = '\0';

		search_close++;
	}

	free(max_close_tag);

	return close_tag;
}

int read_newline(char **curr_line, size_t *buffer_size, FILE *fp, char *str_read) {
	if (fp)
		return getdelim(curr_line, buffer_size, 10, fp);

	// otherwise search through str_read for a newline
	int str_read_index = -1;

	while (*str_read) {
		if (str_read_index == -1)
			str_read_index = 0;

		if ((int) *str_read == 10)
			break;

		(*curr_line)[str_read_index] = *str_read;

		str_read += sizeof(char);
		str_read_index++;

		// check for resize
		if ((str_read_index) * sizeof(char) == *buffer_size) {
			// increase
			*buffer_size *= 2;
			*curr_line = realloc(*curr_line, *buffer_size);
		}

		(*curr_line)[str_read_index] = '\0';
	}

	if (str_read_index == -1)
		return str_read_index;

	(*curr_line)[str_read_index] = '\n';
	(*curr_line)[str_read_index + 1] = '\0';

	return str_read_index + 1;
}

int find_end_comment(FILE *file, char *str_read, char **curr_line, size_t *buffer_size, int search_token) {
	printf("\nCOMMENT %d:\n", search_token);
	while (1) {
		if (search_token > 1 && ((*curr_line)[search_token - 2] == '-' && (*curr_line)[search_token - 1] == '-' && (*curr_line)[search_token] == '>'))
			break;

		printf("%c", (*curr_line)[search_token]);
		if ((*curr_line)[search_token] == '\n') {
			search_token = 0;

			read_newline(curr_line, buffer_size, file, str_read);
		}

		search_token++;
	}

	printf("finish %d\n", search_token);

	return search_token;
}

// builds a new tree and adds it as a child of parent_tree
int read_tag(token_t *parent_tree, FILE *file, char *str_read, char **curr_line, size_t *buffer_size, int search_token) {
	// check for comment:
	if ((*curr_line)[search_token + 1] == '!' && (*curr_line)[search_token + 2] == '-' && (*curr_line)[search_token + 3] == '-')
		return find_end_comment(file, str_read, curr_line, buffer_size, search_token) * -1;

	char *main_tag;
	search_token = read_main_tag(&main_tag, *curr_line, search_token);

	token_t *new_tree = create_token(main_tag, parent_tree);

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

			read_newline(curr_line, buffer_size, file, str_read);
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

			attr_tag_name = resize_arraylist(attr_tag_name, &max_attr_tag_name, attr_tag_name_index, sizeof(char));
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

			attr_tag_value = resize_arraylist(attr_tag_value, &max_attr_tag_value, attr_tag_value_index, sizeof(char));
			attr_tag_value[attr_tag_value_index] = '\0';
		}

		search_token++;
	}

	free(attr_tag_name);
	free(attr_tag_value);

	add_token_children(parent_tree, new_tree);
	return search_token + 1;
}

int tokenizeMETA(FILE *file, char *str_read, token_t *curr_tree) {
	int search_token = 0;

	size_t *buffer_size = malloc(sizeof(size_t));
	*buffer_size = sizeof(char) * 123;
	char **buffer_reader = malloc(sizeof(char *));
	*buffer_reader = malloc(*buffer_size);

	int read_len = 0;
	int line = 0;

	while((read_len = read_newline(buffer_reader, buffer_size, file, str_read)) != -1) {
		printf("LINE #%d\n", line++);
		printf("\nread buffer at tag: %s: %s\n", curr_tree->tag, *buffer_reader);
		// move str_read forward
		str_read += read_len * sizeof(char);

		search_token = 0;
		char *curr_line = *buffer_reader;

		while (curr_line[search_token] != '\n' && curr_line[search_token] != '\0') {
			if (curr_line[search_token] == '<') {
				printf("search for open: %d\n", search_token);
				if (curr_line[search_token + 1] == '/') { // close tag
					// return to parent tree instead

					// check tag name matches curr_tree tag
					char *check_tag = read_close_tag(file, buffer_reader, search_token + 2);
					printf("check cmp: %s vs %s\n", check_tag, curr_tree->tag);
					curr_tree = grab_token_parent(curr_tree);
					printf("\nmove curr tree up-post: %d %s\n", curr_tree->children_index, curr_tree->tag);

					search_token = find_close_tag(file, buffer_reader, search_token) + 1;

					continue;
				}

				search_token = read_tag(curr_tree, file, str_read, buffer_reader, buffer_size, search_token);

				printf("read tag: %d at %s\n", search_token, *buffer_reader);
				// negative means the token went through a comment (aka continue without adding to curr_tree)
				if (search_token >= 0) // update curr_tree to dive into the subtree
					curr_tree = grab_token_children(curr_tree);
				else {
					search_token *= -1;
					return 0;
				}

				continue;
			}

			// otherwise add character to the curr_tree
			add_token_rolling_data(curr_tree, curr_line[search_token]);

			search_token++;
		}
	}

	free(buffer_size);
	free(buffer_reader[0]);
	free(buffer_reader);

	return 0;
}

/*
	reader_type chooses what kind of reading is to be done:
	'f' for file reading
	's' for char * reading
*/
token_t *tokenize(char reader_type, char *filename) {
	FILE *file = NULL;

	if (reader_type == 'f')
		file = fopen(filename, "r");

	char *root_tag = malloc(sizeof(char) * 5);
	strcpy(root_tag, "root");
	token_t *curr_tree = create_token(root_tag, NULL);

	tokenizeMETA(file, filename, curr_tree);

	if (file)
		fclose(file);

	return curr_tree;
}