#include <stdio.h>
#include <stdlib.h>

#include "request.h"
#include "hashmap.h"

#define HOST "cutewiki.charlie.city"
#define PORT "8822"

#define REQ_NAME "permission_data_pull"
#define REQ_PASSCODE "d6bc639b-8235-4c0d-82ff-707f9d47a4ca"

int main() {
	// create write stream:
	FILE *writer = fopen("docbags.txt", "w");
	socket_t *sock_data = get_socket(HOST, PORT);

	res *response = send_req(sock_data, "/pull_page_names", "GET", "-q", "?name=$&passcode=$", REQ_NAME, REQ_PASSCODE);

	printf("get response: %s\n", res_body(response));

	// pull array
	int *array_length = malloc(sizeof(int));
	char **array_body = handle_array(res_body(response), array_length);

	printf("\nCurrent wiki IDs:\n");
	for (int print_array = 0; print_array < *array_length; print_array++) {
		res *wiki_page = send_req(sock_data, "/pull_data", "POST", "-q-b", "?name=$&passcode=$", REQ_NAME, REQ_PASSCODE, "unique_id=$", array_body[print_array]);

		int finish = fputs(res_body(wiki_page), writer);

		if (finish < 0) {
			printf("\nWRITE ERR\n");
			return 1;
		}

		res_destroy(wiki_page);
		free(array_body[print_array]);
	}

	free(array_body);
	free(array_length);

	res_destroy(response);

	destroy_socket(sock_data);

	return 0;
}