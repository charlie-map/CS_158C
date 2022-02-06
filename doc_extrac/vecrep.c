#include <stdio.h>
#include <stdlib.h>

#include "request.h"
#include "hashmap.h"

#define HOST "cutewiki.charlie.city"
#define PORT "8822"

#define REQ_NAME "permission_data_pull"
#define REQ_PASSCODE "d6bc639b-8235-4c0d-82ff-707f9d47a4ca"

int main() {
	socket_t *sock_data = get_socket(HOST, PORT);

	destroy_socket(sock_data);

	return 0;
}