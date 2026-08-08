#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdio.h>
#include "sock.h"

int yvo_create_socket_ipv4_tcp(unsigned int ip, unsigned short port) {
	int sockfd;

	if(((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)) {
		printf("Error while creating IPv4 TCP socket.\n");	
		return -1;
	}

	struct sockaddr_in sockaddr;

	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.s_addr = htonl(ip);

	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
	return sockfd;	
}
