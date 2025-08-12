/*
    Implementation for socket and connect abstractions for a server
*/

#define _GNU_SOURCE

#include "connect.h"

void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

int server_setup () {

    struct addrinfo hints, *servinfo, *p;
	struct sigaction sa;
    int sockfd;
	int rv;
	int yes = 1;

    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;          // Specifying for IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;        // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: listening for connections...\n");

	return sockfd;
}

int accept_connection (int sockfd) {

	socklen_t sin_size;
	struct sockaddr_storage their_addr; // connector's address info
	int new_fd;

	// Accept connection
	sin_size = sizeof their_addr;
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
				&sin_size);

	if (new_fd == -1) {
		perror("accept");
		return -1;
	}

	print_ip ("server: got connection from", their_addr.ss_family, (struct sockaddr *)&their_addr, false);

	return new_fd;
}

void handle_client_connection(int sockfd, int client_fd, const char* message) {
	if (!fork()) { // this is the child process
		close(sockfd); // child doesn't need the listener
		
		// Send the message to the client
		if (send(client_fd, message, strlen(message), 0) == -1) {
			perror("send");
		}
		
		close(client_fd);
		exit(0);
	}
}