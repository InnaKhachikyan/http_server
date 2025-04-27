#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/wait.h>
#include <ctype.h>
#include "userpass_map.h"

#define PORT 8080
#define READ_CHUNK 256
#define POLL_TIME 100
#define MAX_HEADER_SIZE (8 * 1024) // I looked through the standards, its 8KB-16KB
#define MAX_HEADERS 50 // most http requests send under 20 headers, 50 is chosen for safety

struct http_request {
	char *method;
	char *path;
	char *version;
	char *header_name[MAX_HEADERS];
	char *header_value[MAX_HEADERS];
	int header_count;
	char *body; //if there is anything after the \r\n\r\n
};

int create_listening_socket(int port) {
	int listen_fd;
	int opt = 1;
	struct sockaddr_in server_addr;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// not to get the error "address already in use"
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		close(listen_fd);
		exit(EXIT_FAILURE);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY; //listens on all available interfaces not only 127.0...
	server_addr.sin_port = htons(port);

	if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind");
		close(listen_fd);
		exit(EXIT_FAILURE);
	}

	if (listen(listen_fd, SOMAXCONN) < 0) { //SOMAXCONN: the max num of backlog allowed by OS
		perror("listen");
		close(listen_fd);
		exit(EXIT_FAILURE);
	}

	return listen_fd;
}

int read_http_request(int client_fd, char **out_buf, size_t *out_len) {
	size_t length = 0;
	size_t buffer_size = READ_CHUNK;
	char *buffer = (char*)(malloc(buffer_size));
	if(!buffer) {
		printf("Memory for buffer not allocated\n");
		exit(EXIT_FAILURE);
	}

	while(1) {

		if(buffer_size > MAX_HEADER_SIZE) {
			fprintf(stderr, "Too large header\n");
			free(buffer);
			buffer = NULL;
			return -1;
		}

		if(length + READ_CHUNK + 1 > buffer_size) {
			buffer_size *= 2;
			char *tmp = realloc(buffer, buffer_size);
			if(!buffer) {
				printf("Realloc failed\n");
				free(buffer);
				buffer = NULL;
				exit(EXIT_FAILURE);
			}
			buffer = tmp;
		}

		ssize_t bytes_received = recv(client_fd, buffer + length, READ_CHUNK, 0);
		if(bytes_received < 0) {
			printf("Receive failed\n");
			free(buffer);
			buffer = NULL;
			exit(EXIT_FAILURE);
		}
		if(bytes_received == 0)  { //means peer closed the connection
			break;
		}

		length += bytes_received;
		buffer[length] = '\0';
		if(strstr(buffer, "\r\n\r\n") != NULL) {
			break;
		}
	}
	*out_buf = buffer;
	*out_len = length;
	return 0;
}

// we need to trim the whitespace from start/end

char *trim(char *s) {
	while(*s && isspace((unsigned char)*s)) {
		s++;
	}
	if(*s == '\0') {
		return s;
	}
	char *end = s + strlen(s) - 1;
	while(end > s && isspace((unsigned char)*end)) {
		*end-- = '\0';
	}
	return s;
}

// parsing raw http into req; returns 0 on success

int parse_http_request(const char *raw, struct http_request *req) {
	char *copy = strdup(raw);
	if(!copy) {
		return -1;
	}

	char *line_saveptr, *save_ptr, *line;

	line = strtok_r(copy, "\r\n", &line_saveptr);
	if(!line) {
		free(copy);
		copy = NULL;
		return -1;
	}

	char *met = strtok_r(line, " ", &save_ptr);
	char *pth = strtok_r(NULL, " ", &save_ptr);
	char *ver = strtok_r(NULL, " ", &save_ptr);
	if(!met || !pth || !ver) {
		free(copy);
		copy = NULL;
		return -1;
	}

	req->method = strdup(met);
	req->path = strdup(pth);
	req->version = strdup(ver);

	req->header_count = 0;
	while((line = strtok_r(NULL, "\r\n", &line_saveptr)) && *line) {
		if(req->header_count >= MAX_HEADERS) {
			break;
		}

		char *colon = strchr(line, ':');
		if(!colon) {
			continue;
		}
		*colon = '\0';

		char *name = trim(line);
		char *value = trim(colon + 1);

		req->header_name[req->header_count] = strdup(name);
		req->header_value[req->header_count] = strdup(value);
		req->header_count++;
	}

	char *body_start = strstr(raw, "\r\n\r\n");
	if(body_start != NULL) {
		req->body = strdup(body_start + 4);
		if(req->body == NULL) {
			perror("strdup failure");
		}
	}
	else {
		req->body = NULL;
	}
	free(copy);
	copy = NULL;
	return 0;
}

void handle_client_request(int client_fd) {
	char *request = NULL;
	size_t request_length;

	if(read_http_request(client_fd, &request, &request_length) == 0) {
		struct http_request req = {0};
		if(parse_http_request(request, &req) == 0) {
			printf("Request:\nMethod: %s\nPath: %s\nVersion: %s\n", req.method, req.path, req.version);
			for(int i = 0; i < req.header_count; i++) {
				printf("%s: %s\n", req.header_name[i], req.header_value[i]);
			}
			
			if(req.body) {
				printf("Body: %s\n", req.body);
			}
		
	

// basic auth and response logic needs to be implemented yet

//in the end free all alocated memory
			free(req.method);
			req.method = NULL;
			free(req.path);
			req.path = NULL;
			free(req.version);
			req.version = NULL;
			for(int i = 0; i < req.header_count; i++) {
			free(req.header_name[i]);
			req.header_name[i] = NULL;
			free(req.header_value[i]);
			req.header_value[i] = NULL;
			}
			free(req.body);
			req.body = NULL;
		}
		else {
			fprintf(stderr, "Parsing failed\n");
		}
	}
	free(request);
	request = NULL;
	close(client_fd);
}


void fork_for_client(int client_fd, int listen_fd) {
	pid_t pid = fork();
	if(pid < 0) {
		perror("FORK FAILED\n");
		close(client_fd);
		return;
	}
	else if(pid == 0) { //then we are in the child process
		close(listen_fd); // because child doesn't need that socket
		handle_client_request(client_fd);
		exit(EXIT_SUCCESS);
	}
	else {
		close(client_fd); //parent doesn't need that socket after it forked
	}
}

void poll_for_connections(int listen_fd) {
	struct pollfd fds[1];
	fds[0].fd = listen_fd;
	fds[0].events = POLLIN;

	while(1) {
		int retPoll = poll(fds, 1, POLL_TIME);
		if(retPoll < 0) {
			perror("POLL FAILED\n");
			exit(1);
		}

		int status;
		pid_t pid;

		while((pid = waitpid(-1, &status, WNOHANG)) > 0 ) {
			printf("Child %d exited %d\n", pid, WEXITSTATUS(status));
		}

		if(retPoll == 0) {
			continue;
		}

		if(fds[0].revents & POLLIN) {
			struct sockaddr_in client_addr;
			socklen_t addr_len = sizeof(client_addr);
			int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
			if(client_fd < 0) {
				perror("ACCEPT FAILED\n");
				continue;
			}

			fork_for_client(client_fd, listen_fd);
		}
	}
}

int main() {

	int listen_fd = create_listening_socket(PORT);
	printf("Listening on port %d\n", PORT);

	poll_for_connections(listen_fd);

	close(listen_fd);
	init_user_map();

    add_user("Alice",   "0123456");
    add_user("Bob",     "987654");
    add_user("Charlie", "pass123");
    add_user("David",   "qwerty");
    add_user("Eve",     "evepass");
    add_user("Frank",   "fr4nk!");
    add_user("Grace",   "gracepwd");
    add_user("Heidi",   "heidipwd");
    add_user("Ivan",    "ivan123");
    add_user("Judy",    "judypass");

    printf("Total users added: %zu\n", user_count);
    for (size_t i = 0; i < user_count; i++) {
        printf("User %zu: %s / %s\n", i+1, entries[i].username, entries[i].password);
    }

    free_user_map();
    return 0;

}

