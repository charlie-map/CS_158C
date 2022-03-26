#ifndef __SOCKET_L__
#define __SOCKET_L__

#include "hashmap.h"
#include "../serialize/serialize.h"

typedef struct Response res;
char *res_body(res *re);
int res_body_len(res *re);
hashmap *res_headers(res *re);
int res_destroy(res *re);

typedef struct SocketData {
	int sock;
	void *servinfo;
	char *HOST, *PORT;
} socket_t;

struct MutexPtr {
	pthread_mutex_t mutex;
};

socket_t *get_socket(char *HOST, char *PORT);
int destroy_socket(socket_t *socket_data);

res *send_req(socket_t *sock, char *sub_url, char *type, char *param, ...);

char *create_header(char *HOST, char *PORT, char *request_url, int *url_length, char *post_data);

hashmap *read_headers(char *header_str, int *header_end);

// res is the response body, max_len is the length of the
// char ** returned. So an input of:
// ["10", "586", 20], would result in:
// char ** = ["10", "586", "20"], max_len = 3
char **handle_array(char *res, int *max_len);

#endif