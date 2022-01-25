#ifndef __TOKEN_L__
#define __TOKEN_L__

typedef struct Token token_t;

token_t *tokenize(char *filename);

token_t **token_children(token_t *parent);
token_t *grab_token_parent(token_t *curr_token);

char **token_get_tag_data(token_t *search_token, char *tag_name, int *max_tag);

int destroy_token(token_t *curr_token);

#endif