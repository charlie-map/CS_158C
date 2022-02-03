#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

// all socket related packages
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>

#include "stack.h"

#define HOST "cutewiki.charlie.city"
#define PORT "8822"
#define MAXLINE 4096

#define REQ_NAME "permission_data_pull"
#define REQ_PASSCODE "d6bc639b-8235-4c0d-82ff-707f9d47a4ca"

void *resize_array(void *arr, int *max_len, int curr_index, size_t singleton_size) {
	while (curr_index >= *max_len) {
		*max_len *= 2;

		arr = realloc(arr, singleton_size * *max_len);
	}
	
	return arr;
}

void sigchld_handler(int s) {
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa) {

	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

// param will be the formatter for an query paramaters:
// ?name=$&other_value=$
// expects an empty string if no query paramaters are wanted
char *build_request(char *request_url, int *req_length, char *query_param, ...) {
	va_list param_pull;
	va_start(param_pull, query_param);

	int req_index = strlen(request_url);
	*req_length = req_index * (strlen(query_param) ? 2 : 1) + 1;
	char *full_request = malloc(sizeof(char) * *req_length);

	strcpy(full_request, request_url);

	for (int check_param = 0; query_param[check_param]; check_param++) {

		if (query_param[check_param] == '$') {
			// read from arg list
			char *arg_value = va_arg(param_pull, char *);
			int arg_len = strlen(arg_value);

			full_request = resize_array(full_request, req_length, req_index + arg_len, sizeof(char));
			strcpy(full_request + (sizeof(char) * req_index), arg_value);

			req_index += arg_len;
		} else
			full_request[req_index++] = query_param[check_param];

		full_request = resize_array(full_request, req_length, req_index, sizeof(char));

		full_request[req_index] = '\0';
	}

	*req_length = req_index;

	return full_request;
}

char *create_header(char *request_url, int *url_length) {
	int host_length = strlen(HOST);
	char *build_host = malloc(sizeof(char) * (host_length + strlen(PORT) + 2));
	memset(build_host, '\0', host_length + strlen(PORT) + 2);

	strcpy(build_host, HOST);
	build_host[host_length] = ':';
	strcat(build_host + (sizeof(char) * (host_length + 1)), PORT);

	// calculate header length
	*url_length = 47 + *url_length + strlen(build_host);
	char *header = malloc(sizeof(char) * *url_length);
	char *copy_header = malloc(sizeof(char) * (*url_length + 1));

	sprintf(copy_header, "GET %s HTTP/1.1\nHost: %s\nContent-Type: text/plain\n\n", request_url, build_host);

	for (int copy_value = 0; copy_value < *url_length; copy_value++) {
		header[copy_value] = copy_header[copy_value];
	}

	free(build_host);
	free(copy_header);

	return header;
}

char **handle_array(char *res, int *max_len, int curr_res_pos) {
	int arr_index = 0;
	*max_len = 8;
	char **arr = malloc(sizeof(char *) * *max_len);

	int max_res_length = strlen(res);

	// create a stack that tells the "level of depth" within 
	// certain values within the array, so a comma in a string
	// will not count as being a separator in the array:
	// ["10, 2", 6]
	stack_tv2 *abstract_depth = stack_create();

	for (curr_res_pos; curr_res_pos < max_res_length; curr_res_pos++) {
		int *str_len = malloc(sizeof(int)), str_index = 0;
		*str_len = 8;
		char *str = malloc(sizeof(char) * *str_len);

		str[str_index] = '\0';

		// seek until ',' or ']' AND stack is empty
		while ((res[curr_res_pos] != ',' && res[curr_res_pos] != ']') || stack_size(abstract_depth)) {
			if (res[curr_res_pos] == '[' && !stack_size(abstract_depth)) {
				curr_res_pos++;
				continue;
			}

			if (res[curr_res_pos] == '"' && stack_size(abstract_depth) && res[curr_res_pos] == ((char *) stack_peek(abstract_depth))[0])
				stack_pop(abstract_depth);
			else if (res[curr_res_pos] == '"')
				stack_push(abstract_depth, "\"");
			else {
				str[str_index++] = res[curr_res_pos];

				str = (char *) resize_array(str, str_len, str_index, sizeof(char));
				str[str_index] = '\0';
			}
			curr_res_pos++;
		}

		free(str_len);

		arr[arr_index++] = str;
		arr = (char **) resize_array(arr, max_len, arr_index, sizeof(char *));
	}

	stack_destroy(abstract_depth);
	*max_len = arr_index;

	return arr;
}

// takes request url and will build the full url:
char *send_req(int socket, char *request_url, int *url_length, char *type, ...) {
	// build the request into the header:
	char *header = create_header(request_url, url_length);

	// send get request
	int bytes_sent = 0, total_bytes = sizeof(char) * *url_length;
	while ((bytes_sent = send(socket, header, *url_length, 0)) < total_bytes) {
		if (bytes_sent == -1) {
			fprintf(stderr, "send data err: %s\n", gai_strerror(bytes_sent));
			exit(1);
		}
	}

	free(header);

	int bytes_recv = -1;
	char *buffer = malloc(sizeof(char) * MAXLINE);
	memset(buffer, '\0', sizeof(char) * MAXLINE);
	int buffer_len = MAXLINE;

	while (bytes_recv = recv(socket, buffer, buffer_len, 0)) {
		if (bytes_recv == -1)
			continue;

		break;
	}

	// find start of data in buffer:
	int buffer_data_len = strlen(buffer);
	int curr_res_pos;
	for (curr_res_pos = 0; curr_res_pos < buffer_data_len; curr_res_pos++) {
		if ((int) buffer[curr_res_pos] == 10 && (int) buffer[curr_res_pos + 2] == 10) {
			curr_res_pos += 3;
			break;
		}
	}

	// create string that holds return value:
	char *return_string = malloc(sizeof(char) * (buffer_data_len - curr_res_pos + 1));
	strcpy(return_string, buffer + sizeof(char) * curr_res_pos);

	free(buffer);

	return return_string;
}

int main() {
	// connection stuff
	//int *sock_fd = malloc(sizeof(int)); // listen on sock_fd
	int sock;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	struct sigaction sa;

	int status;

	memset(&hints, 0, sizeof(hints)); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;	  // dont' care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;  // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;	  // fill in my IP for me

	if ((status = getaddrinfo(HOST, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	// loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sock = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(sock);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

	// send get request to remote server
	int *url_length = malloc(sizeof(int));
	char *new_url = build_request("/pull_page_names", url_length, "?name=$&passcode=$", REQ_NAME, REQ_PASSCODE);
	int *response_max_len = malloc(sizeof(int));
	char *response = send_req(sock, new_url, url_length, "GET");

	printf("All check: %s\n", response);

	char **arr = handle_array(response, response_max_len, 0);

	for (int check_res = 0; check_res < *response_max_len; check_res++) {
		printf("check response: %s\n", arr[check_res]);

		free(arr[check_res]);
	}

	free(url_length);
	free(new_url);
	free(response_max_len);
	free(response);

	free(arr);

	freeaddrinfo(servinfo);

	return 0;
}