#ifndef __TOKEN_L__
#define __TOKEN_L__

typedef struct Token token_t;

token_t *tokenize(char *filename);

int destroy_token(token_t *curr_token);

#endif