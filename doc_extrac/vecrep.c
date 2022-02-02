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

#define HOST "localhost"
#define PORT "9876"
#define MAXLINE 4096

#define REQ_NAME "permission_data_pull"
#define REQ_PASSCODE "d6bc639b-8235-4c0d-82ff-707f9d47a4ca"

void *resize_array(void *arr, int *max_len, int curr_index) {
	while (curr_index >= *max_len) {
		*max_len *= 2;

		arr = realloc(arr, sizeof(arr[0]) * *max_len);
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

			full_request = resize_array(full_request, req_length, req_index + arg_len);
			strcpy(full_request + (sizeof(char) * req_index), arg_value);

			req_index += arg_len;
		} else
			full_request[req_index++] = query_param[check_param];

		full_request = resize_array(full_request, req_length, req_index);

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
	*url_length = 24 + *url_length + strlen(build_host);
	char *header = malloc(sizeof(char) * *url_length);
	
	sprintf(header, "GET %s HTTP/1.1\nHost: %s\n\n\n", request_url, build_host);

	return header;
}

char **handle_array(char *res, int curr_res_pos) {
	int *max_len = malloc(sizeof(int)), arr_index = 0;
	*max_len = 8;
	char **arr = malloc(sizeof(char) * *max_len);

	int max_res_length = strlen(res);

	// create a stack that tells the "level of depth" within 
	// certain values within the array, so a comma in a string
	// will not count as being a separator in the array:
	// ["10, 2", 6]
	stack_tv2 *abstract_depth = stack_create();

	printf("test %d to %d\n", curr_res_pos, max_res_length);

	for (curr_res_pos; curr_res_pos < max_res_length; curr_res_pos++) {
		int *str_len = malloc(sizeof(int)), str_index = 0;
		*str_len = 8;
		char *str = malloc(sizeof(char) * *str_len);

		// seek until ',' or ']' AND stack is empty
		while ((res[curr_res_pos] != ',' && res[curr_res_pos] != ']') || stack_size(abstract_depth)) {
			if (res[curr_res_pos] == '"' && res[curr_res_pos] == ((char *) stack_peek(abstract_depth))[0])
				stack_pop(abstract_depth);
			else if (res[curr_res_pos] == '"');
				stack_push(abstract_depth, "\"");

			str[str_index++] = res[curr_res_pos];
			curr_res_pos++;

			str = resize_array(str, str_len, str_index);
		}

		free(str_len);

		arr[arr_index++] = str;
		arr = resize_array(arr, max_len, arr_index);
	}

	for (int check = 0; check < arr_index; check++) {
		printf("Test: %s\n", arr[check]);
	}

	return arr;
}

// takes request url and will build the full url:
char **get(int socket, char *request_url, int *url_length, char ** (*handle_response)(char *, int)) {
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

	int bytes_recv = -1;
	char *buffer = malloc(sizeof(char) * MAXLINE);
	int buffer_len = MAXLINE;

	while (bytes_recv = recv(socket, buffer, buffer_len, 0)) {
		if (bytes_recv == -1)
			continue;

		break;
	}

	// find start of data in buffer:
	int curr_res_pos;
	for (curr_res_pos = 0; curr_res_pos < buffer_len; curr_res_pos++) {
		if (buffer[curr_res_pos] == '\n' && buffer[curr_res_pos + 2] == '\n')
			break;
	}

	char **test = handle_array(buffer, curr_res_pos);

	return test;
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

	sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	if ((status = connect(sock, servinfo->ai_addr, servinfo->ai_addrlen)) != 0) {
		fprintf(stderr, "connect error: %s\n", gai_strerror(status));
		exit(1);
	}

	// send get request to remote server
	int *url_length = malloc(sizeof(int));
	char *new_url = build_request("/test", url_length, ""); //build_request("/test", url_length, "?name=$&passcode=$", REQ_NAME, REQ_PASSCODE);
	printf("Get full length %d request: %s\n", *url_length, new_url);
	get(sock, new_url, url_length, handle_array);

	freeaddrinfo(servinfo);

	free(url_length);
	free(new_url);

	return 0;
}