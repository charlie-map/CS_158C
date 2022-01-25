#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "token.h"

/*
	serialize will try to search through the void * to pull out informaiton
	for serialization. this is challenging to define as a single "general"
	process. however, using the char *param and function overloading the concept
	can theoretically be used (with a function header that connects to necessary
	functions). char *param will take in quite a few inputs:

	NOTES:
		- order does NOT matter
		- "i" is the required minimum inputs for the indexing to work, others are optional

	- "i": tells the program what the name (char *) of the output file
		for the generated index is (so all data can be written directly into the
		files)
	- "t": tells the program what the name (char *) of the title
		file is
		- if "t" is used, IMMEDIATELY after the (char *) that holds the title output filename
			should be the pointer to the function that gets the title of whatever
	- "o": defines the order the output file held within "i" should have
		- "w: i" is an example where "w" is the current word and "i" (in the local context)
			is the "id"

	- an example of the above could be:
		"ito", "invertedIndex.txt", "titleIndex.txt", "w: i"
*/
// int serializeMETA(void *tree, char *param, ...) {
// 	// setup any initial requirements
// 	va_list 
// }

int main() {
	token_t *tree_root = tokenize("miniXML.txt");

	int *total_tags = malloc(sizeof(int));
	*total_tags = 0;
	char **tag = token_get_tag_data(tree_root, "id", total_tags);

	for (int check_tag = 0; check_tag < *total_tags; check_tag++) {
		printf("check tag: %s -- %d\n", tag[check_tag], strlen(tag[check_tag]));
	}

	destroy_token(tree_root);

	return 0;
}