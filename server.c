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

#define PORT 8080
#define BUFFER_SIZE 1024
#define POLL_TIME 100

int create_listening_socket(int port) {
	int listen_fd;
	int opt = 1;
	struct sockaddr_in server_addr;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
        	perror("socket");
        	exit(EXIT_FAILURE);
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY //listens on all available interfaces not only 127.0...
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

		if(ret == 0) {
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

			fork_for_client(client_fd);
		}
}

void fork_for_client(int client_fd) {
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

void handle_client_request(int client_fd) {
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;

	bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if(bytes_read < 0) {
		perror("BYTES NOT READ\n");
		return;
	}
	else {
		buffer[bytes_read] = '\0';
		printf("Client request: %s\n", buffer);
	}

	close(client_fd);
}
