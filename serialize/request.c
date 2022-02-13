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
#include "request.h"

#define MAXLINE 4096

struct Response {
	hashmap *headers;
	char *body;
	int body_len;
};

res *res_create(hashmap *head, char *bo, int bo_len) {
	res *new_res = malloc(sizeof(res));

	new_res->headers = head;
	new_res->body = bo;
	new_res->body_len = bo_len;

	return new_res;
}

char *res_body(res *re) {
	return re->body;
}

int res_body_len(res *re) {
	return re->body_len;
}

hashmap *res_headers(res *re) {
	return re->headers;
}

int res_destroy(res *re) {
	deepdestroy__hashmap(re->headers);
	free(re->body);

	free(re);

	return 0;
}

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
char *build_url(char *request_url, int *req_length, char *query_param, char **attr_values) {
	int req_index = strlen(request_url);
	*req_length = req_index * (strlen(query_param) ? 2 : 1) + 1;
	char *full_request = malloc(sizeof(char) * *req_length);

	strcpy(full_request, request_url);

	int curr_attr_pos = 0;

	for (int check_param = 0; query_param[check_param]; check_param++) {

		if (query_param[check_param] == '$') {
			// read from arg list
			char *arg_value = attr_values[curr_attr_pos];
			int arg_len = strlen(arg_value);

			full_request = resize_array(full_request, req_length, req_index + arg_len, sizeof(char));
			strcpy(full_request + (sizeof(char) * req_index), arg_value);

			req_index += arg_len;
			curr_attr_pos++;
		} else
			full_request[req_index++] = query_param[check_param];

		full_request = resize_array(full_request, req_length, req_index, sizeof(char));

		full_request[req_index] = '\0';
	}

	*req_length = req_index;

	return full_request;
}

char *create_header(char *HOST, char *PORT, char *request_url, int *url_length, char *post_data) {
	int host_length = strlen(HOST);
	char *build_host = malloc(sizeof(char) * (host_length + strlen(PORT) + 2));
	memset(build_host, '\0', host_length + strlen(PORT) + 2);

	strcpy(build_host, HOST);
	build_host[host_length] = ':';
	strcat(build_host, PORT);

	// calculate header length
	int post_data_len = post_data ? strlen(post_data) : 0;

	// calculate how many digits the post_data_len is:
	int content_number = 1, content_number_calculator = post_data_len;
	if (post_data_len >= 100) { content_number += 2; content_number_calculator /= 100; }
	if (post_data_len >= 10) { content_number += 1; content_number_calculator /= 10; }

	*url_length = 69 + (post_data ? 24 + post_data_len : 0) + *url_length + strlen(build_host) + content_number;

	char *header = malloc(sizeof(char) * (*url_length + 1));

	sprintf(header, "%s %s HTTP/1.1\r\nHost: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n%s",
		post_data ? "POST" : "GET", request_url, build_host, (post_data ?
		"application/x-www-form-urlencoded" : "text/plain"), post_data_len, post_data ? post_data : "");

	free(build_host);

	return header;
}

char **handle_array(char *res, int *max_len) {	
	int arr_index = 0;
	*max_len = 8;
	char **arr = malloc(sizeof(char *) * *max_len);

	int max_res_length = strlen(res);

	// create a stack that tells the "level of depth" within 
	// certain values within the array, so a comma in a string
	// will not count as being a separator in the array:
	// ["10, 2", 6]
	stack_tv2 *abstract_depth = stack_create();

	for (int curr_res_pos = 0; curr_res_pos < max_res_length; curr_res_pos++) {
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

hashmap *read_headers(char *header_str, int *header_end) {
	int past_lines = 0;

	hashmap *header_map = make__hashmap(0, NULL, destroyCharKey);

	// jump past HTTP: status line
	while ((int) header_str[past_lines] != 10)
		past_lines++;

	past_lines += 1;

	// while the newline doesn't start with a newline
	// (double newline is end of header)
	while ((int) header_str[past_lines] != 10) {
		int *head_max = malloc(sizeof(int)), head_index = 0;
		*head_max = 8;
		char *head_tag = malloc(sizeof(char) * *head_max);

		int *attr_max = malloc(sizeof(int)), attr_index = 0;
		*attr_max = 8;
		char *attr_tag = malloc(sizeof(char) * *attr_max);

		// head head tag
		while ((int) header_str[past_lines + head_index] != 58) {

			head_tag[head_index++] = header_str[past_lines + head_index];

			head_tag = resize_array(head_tag, head_max, head_index, sizeof(char));
			head_tag[head_index] = '\0';
		}

		past_lines += head_index + 2;

		// read attr tag
		while ((int) header_str[past_lines + attr_index + 1] != 10) {
			attr_tag[attr_index++] = header_str[past_lines + attr_index];

			attr_tag = resize_array(attr_tag, attr_max, attr_index, sizeof(char));
			attr_tag[attr_index] = '\0';
		}

		// check for a carraige return (\r). This means the newline character is one further along
		past_lines += attr_index + ((int) header_str[past_lines + attr_index + 2] == 13 ? 3 : 2);

		insert__hashmap(header_map, head_tag, attr_tag, "", compareCharKey, destroyCharKey);

		free(head_max);
		free(attr_max);
	}

	*header_end = past_lines + 1;
	return header_map;
}

// takes request url and will build the full url:
res *send_req_helper(socket_t *socket, char *request_url, int *url_length, char *type, ...) {
	char *data = NULL;
	if (strcmp(type, "POST") == 0) { // look for next param
		va_list post_data;
		va_start(post_data, type);

		data = va_arg(post_data, char *);
	}

	// build the request into the header:
	char *header = create_header(socket->HOST, socket->PORT, request_url, url_length, data);

	// send get request
	int bytes_sent = 0, total_bytes = sizeof(char) * *url_length;
	while ((bytes_sent = send(socket->sock, header, *url_length, 0)) < total_bytes) {
		if (bytes_sent == -1) {
			fprintf(stderr, "send data err: %s\n", gai_strerror(bytes_sent));
			exit(1);
		}
	}

	free(header);

	int curr_bytes_recv = 0, prev_bytes_recv = 0;
	size_t header_len = sizeof(char) * MAXLINE;
	char *header_read = malloc(header_len);
	memset(header_read, '\0', header_len);

	curr_bytes_recv += recv(socket->sock, header_read, header_len, 0);

	if (curr_bytes_recv == 0) {// missing data -- socket closed
		free(header_read);
		return NULL;
	}

	// read header
	int *header_end = malloc(sizeof(int));
	hashmap *headers = read_headers(header_read, header_end);
	int content_length = atoi(get__hashmap(headers, "Content-Length", 0));

	size_t full_req_len = sizeof(char) * content_length;
	char *buffer = malloc(full_req_len + sizeof(char));
	memset(buffer, '\0', full_req_len + sizeof(char));
	for (int copy_after_head = *header_end; copy_after_head * sizeof(char) < curr_bytes_recv; copy_after_head++) {
		buffer[copy_after_head - *header_end] = header_read[copy_after_head];
	}

	int buffer_bytes = curr_bytes_recv - *header_end * sizeof(char);
	free(header_read);
	free(header_end);

	while (buffer_bytes < full_req_len) {
		int new_bytes = recv(socket->sock, buffer + buffer_bytes, full_req_len, 0);

		if (new_bytes == -1) {
			continue;
		}

		buffer_bytes += new_bytes;
	}

	buffer[full_req_len] = '\0';

	// create response structure
	return res_create(headers, buffer, full_req_len + 1);
}

socket_t *get_socket(char *HOST, char *PORT) {
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
			exit(1);
		}

		if (connect(sock, p->ai_addr, p->ai_addrlen) == -1) {
			close(sock);
			perror("client: connect");
			exit(1);
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		exit(1);
	}

	socket_t *data_return = malloc(sizeof(socket_t));
	data_return->sock = sock;
	data_return->servinfo = servinfo;
	data_return->HOST = HOST;
	data_return->PORT = PORT;

	return data_return;
}

int destroy_socket(socket_t *socket_data) {
	freeaddrinfo((struct addrinfo *) socket_data->servinfo);

	free(socket_data);

	return 0;
}

int spec_char_sum(char *request_structure, char search_char) {
	int sum = 0;
	// go through the char * and count the occurences of search_char
	for (int check_count = 0; request_structure[check_count]; check_count++) {
		if (request_structure[check_count] == search_char)
			sum++;
	}

	return sum;
}

/* param should contain any additional components of the request:
	param has two components (currently).
	- "-q" for adding queries, this should take in the following:
		- first, a query structure that looks something like:
		"?name=$&passcode=$", where name and passcode are the names of
		query parameters
		- then a char * for each $ within the query
	- "-b" for adding body, this has the exact same structure as the previous, however,
		does not start with a "?":
		"name=$&passcode=$"
		- then a char * for each $ within the body
	- if no params are wanted, put "" in for param
*/
res *send_req(socket_t *sock, char *sub_url, char *type, char *param, ...) {
	// parse param:
	va_list read_multi_param;
	va_start(read_multi_param, param);

	int *url_length = malloc(sizeof(int));
	char *new_url;

	int *data_length = malloc(sizeof(int));
	char *new_data = NULL;

	for (int check_param = 0; param[check_param]; check_param++) {
		if (param[check_param] != '-')
			continue;

		if (param[check_param + 1] != 'q' && param[check_param + 1] != 'b')
			continue;

		// grab the immediately next parameter which shows the structure of the 
		// query:
		char *query_structure = va_arg(read_multi_param, char *);

		// count the amount of "$" there are:
		int $_sum = spec_char_sum(query_structure, '$');

		// per number of $, create a char ** to hold each one, which will be
		// sent to the request builder:
		char **tag_attrs = malloc(sizeof(char *) * $_sum);
		for (int grab_tag_attr = 0; grab_tag_attr < $_sum; grab_tag_attr++) {
			tag_attrs[grab_tag_attr] = va_arg(read_multi_param, char *);
		}

		// now choose for body ('b') or query ('q')
		if (param[check_param + 1] == 'q') {
			new_url = build_url(sub_url, url_length, query_structure, tag_attrs);
		} else {
			new_data = build_url("", data_length, query_structure, tag_attrs);
		}

		free(tag_attrs);
	}

	free(data_length);

	if (*url_length == 0)
		return NULL;

	// if GET, sending data doesn't matter
	res *response = send_req_helper(sock, new_url, url_length, type, new_data);

	free(new_url);
	free(url_length);

	if (new_data)
		free(new_data);

	return response;
}