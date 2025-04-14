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
